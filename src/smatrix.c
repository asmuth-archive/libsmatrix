// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "smatrix.h"

smatrix_t* smatrix_open(const char* fname) {
  smatrix_t* self = malloc(sizeof(smatrix_t));

  if (self == NULL)
    return NULL;

  pthread_mutex_init(&self->wlock, NULL);

  self->rmap.size = SMATRIX_RMAP_INITIAL_SIZE;
  self->rmap.used = 0;
  self->rmap.data = malloc(sizeof(smatrix_row_t) * self->rmap.size);
  self->rmap_size = 0;
  self->rmap_fpos = 23;

  if (self->rmap.data == NULL) {
    free(self);
    return NULL;
  }

  memset(self->rmap.data, 0, sizeof(smatrix_row_t) * self->rmap.size);
  pthread_rwlock_init(&self->rmap.lock, NULL);

  self->file = fopen(fname, "a+b");

  if (self->file == NULL) {
    perror("cannot open file");
    free(self->rmap.data);
    free(self);
    return NULL;
  }

  //pthread_rwlock_init(&self->lock, NULL);

  return self;
}

smatrix_row_t* smatrix_rmap_lookup(smatrix_rmap_t* rmap, uint32_t key, smatrix_row_t* insert) {
  long int n, pos;
  smatrix_row_t* row;

  pthread_rwlock_rdlock(&rmap->lock);
  pos = key % rmap->size;

  for (n = 0; n < rmap->size; n++) {
    if (!rmap->data[pos].ptr)
      break;

    if (rmap->data[pos].key == key) {
      row = rmap->data[pos].ptr;
      goto rmap_unlock;
    }

    pos = (pos + 1) % rmap->size;
  }

  pthread_rwlock_unlock(&rmap->lock);

  if (insert == NULL)
    return NULL;

  pthread_rwlock_wrlock(&rmap->lock);

  if (rmap->used > rmap->size / 2) {
    smatrix_rmap_resize(rmap);
    pthread_rwlock_unlock(&rmap->lock);
    return smatrix_rmap_lookup(rmap, key, insert);
  }

  rmap->used++;
  rmap->data[pos].key = key;
  rmap->data[pos].ptr = insert;
  row = rmap->data[pos].ptr;

rmap_unlock:

  pthread_rwlock_unlock(&rmap->lock);
  return row;
}

void smatrix_rmap_resize(smatrix_rmap_t* rmap) {
  int n;
  smatrix_rmap_t new;
  smatrix_row_t* row;

  pthread_rwlock_init(&new.lock, NULL);

  new.used = 0;
  new.size = rmap->size * 2;
  new.data = malloc(sizeof(smatrix_row_t) * new.size);

  memset(new.data, 0, sizeof(smatrix_row_t) * new.size);

  if (new.data == NULL) {
    printf("RMAP RESIZE FAILED (MALLOC)!!!\n"); // FIXPAUL
    abort();
  }

  for (n = 0; n < rmap->size; n++) {
    if (!rmap->data[n].ptr)
      continue;

    smatrix_rmap_lookup(&new, rmap->data[n].key, rmap->data[n].ptr);
  }

  pthread_rwlock_destroy(&new.lock);

  rmap->data = new.data;
  rmap->size = new.size;
  rmap->used = new.used;
}

void smatrix_rmap_sync(smatrix_t* self) {
  int n, all_dirty = 1;

  pthread_rwlock_rdlock(&self->rmap.lock);
  pthread_mutex_lock(&self->wlock);

  if (self->rmap_size != self->rmap.size) {
    // FIXPAUL: write new rmap size + rmap pos in file header
    all_dirty = 1;
  }

  for (n = 0; n < self->rmap.size; n++) {
    if (!self->rmap.data[n].ptr)
      continue;

    if (!all_dirty)
      continue;

    printf("PERSIST %i->%p @ %li\n", self->rmap.data[n].key, self->rmap.data[n].ptr, self->rmap_fpos + (n * 16));
  }

  pthread_rwlock_unlock(&self->rmap.lock);

  pthread_mutex_unlock(&self->wlock);
}

smatrix_vec_t* smatrix_lookup(smatrix_t* self, uint32_t x, uint32_t y, int create) {
  smatrix_vec_t *col = NULL, **row = NULL;

  pthread_rwlock_rdlock(&self->lock);

  if (x > self->size) {
    if (x > SMATRIX_MAX_ID)
      goto unlock;

    if (create) {
      smatrix_wrlock(self);

      if (x > self->size)
        smatrix_resize(self, x + 1);

      if (x > self->size)
        goto unlock;

      smatrix_unlock(self);
    } else {
      goto unlock;
    }
  }

  row = self->data + x;

  if (*row == NULL) {
    if (!create)
      goto unlock;

    smatrix_wrlock(self);

    if (*row == NULL)
      col = smatrix_insert(row, y);

    smatrix_unlock(self);
  }

  smatrix_vec_lock(*row);

  if (col == NULL) {
    col = *row;

    while (col->index != y) {
      if (col->next == NULL || col->index > y) {
        col = NULL;
        break;
      }

      col = col->next;
    }
  }

  if (col == NULL && create) {
    smatrix_wrlock(self);
    col = smatrix_insert(row, y);
    smatrix_unlock(self);
  }

  smatrix_vec_incref(col);
  smatrix_vec_unlock(*row);

unlock:

  pthread_rwlock_unlock(&self->lock);
  return col;
}

smatrix_vec_t* smatrix_insert(smatrix_vec_t** row, uint32_t y) {
  uint32_t row_len = 1;
  smatrix_vec_t **cur = row, *next, *new;

  for (; *cur && (*cur)->index <= y; row_len++) {
    if ((*cur)->index == y) {
      return *cur;
    } else {
      cur = &((*cur)->next);
    }
  }

  next = *cur;

  *cur = new = malloc(sizeof(smatrix_vec_t));
  new->value = 0;
  new->index = y;
  new->flags = 0;
  new->next  = next;

  for (; next; row_len++)
    next = next->next;

  if (row_len > SMATRIX_MAX_ROW_SIZE)
    smatrix_truncate(row);

  return new;
}

void smatrix_resize(smatrix_t* self, uint32_t min_size) {
  long int new_size = self->size;

  while (new_size < min_size) {
    new_size = new_size * SMATRIX_GROWTH_FACTOR;
  }

  smatrix_vec_t** new_data = malloc(sizeof(void *) * new_size);

  if (new_data == NULL)
    return;

  memcpy(new_data, self->data, sizeof(void *) * self->size);
  memset(new_data, 0, sizeof(void *) * (new_size - self->size));

  free(self->data);

  self->data = new_data;
  self->size = new_size;
}

void smatrix_increment(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value) {
  smatrix_vec_t* vec = smatrix_lookup(self, x, y, 1);

  if (vec == NULL)
    return;

  // FIXPAUL check for overflow, also this is not an atomic increment
  vec->value++;
}

void smatrix_truncate(smatrix_vec_t** row) {
  smatrix_vec_t **cur = row, **found = NULL, *delete;

  while (*cur) {
    if ((*cur)->index != 0 && (*cur)->value != 0) {
      if (found == NULL || (*found)->value > (*cur)->value) {
        found = cur;
      }
    }

    cur = &((*cur)->next);
  }

  delete = *found;
  *found = delete->next;

  smatrix_vec_decref(delete);
}

void smatrix_close(smatrix_t* self) {
  smatrix_vec_t *cur, *tmp;
  uint32_t n;
/*
  for (n = 0; n < self->size; n++) {
    cur = self->data[n];

    while (cur) {
      tmp = cur;
      cur = cur->next;
      free(tmp);
    }
  }

  */

  pthread_mutex_destroy(&self->wlock);
  pthread_rwlock_destroy(&self->rmap.lock);

  free(self->rmap.data);
  free(self);
}

void smatrix_dump(smatrix_t* self) {
  smatrix_vec_t *cur;
  uint32_t n;

  for (n = 0; n < self->size; n++) {
    cur = self->data[n];

    if (!cur) continue;

    printf("%i ===> ", n);
    while (cur) {
      printf(" %i:%i, ", cur->index, cur->value);
      cur = cur->next;
    }
    printf("\n----\n");
  }
}

void smatrix_wrlock(smatrix_t* self) {
  pthread_rwlock_unlock(&self->lock);
  pthread_rwlock_wrlock(&self->lock);
}

void smatrix_unlock(smatrix_t* self) {
  pthread_rwlock_unlock(&self->lock);
  pthread_rwlock_rdlock(&self->lock);
}

void smatrix_vec_lock(smatrix_vec_t* vec) {
  for (;;) {
    __sync_synchronize();
    volatile uint32_t flags = vec->flags;

    if ((flags & 1) == 1)
      continue;

    if (__sync_bool_compare_and_swap(&vec->flags, flags, flags | 1))
      break;
  }
}

void smatrix_vec_unlock(smatrix_vec_t* vec) {
  __sync_synchronize();
  vec->flags &= ~1;
}

void smatrix_vec_incref(smatrix_vec_t* vec) {
}

void smatrix_vec_decref(smatrix_vec_t* vec) {
}
