// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "smatrix.h"

smatrix_t* smatrix_open(const char* fname) {
  smatrix_t* self = malloc(sizeof(smatrix_t));

  if (self == NULL)
    return NULL;

  pthread_mutex_init(&self->wlock, NULL);
  pthread_rwlock_init(&self->rmap.lock, NULL);

// FILE INIT

  self->fd = open(fname, O_RDWR | O_CREAT, 00600);

  if (self->fd == -1) {
    perror("cannot open file");
    free(self->rmap.data);
    free(self);
    return NULL;
  }

  self->fpos = lseek(self->fd, 0, SEEK_END);

  if (self->fpos == 0) {
    printf("NEW FILE!\n");
    smatrix_falloc(self, SMATRIX_META_SIZE);

    self->rmap.size = SMATRIX_RMAP_INITIAL_SIZE;
    self->rmap.used = 0;
    self->rmap_size = 0;
    self->rmap_fpos = 0;
    self->rmap.data = malloc(sizeof(smatrix_rmap_slot_t) * self->rmap.size);
    memset(self->rmap.data, 0, sizeof(smatrix_rmap_slot_t) * self->rmap.size);

    smatrix_rmap_sync(self);
  } else {
    printf("LOAD FILE!\n");
    smatrix_meta_load(self);
    smatrix_rmap_load(self);
  }

  return self;
}

// FIXPAUL: this needs to be atomic (compare and swap!)
uint64_t smatrix_falloc(smatrix_t* self, uint64_t bytes) {
  uint64_t old = self->fpos;
  uint64_t new = old + bytes;

  //printf("TRUNCATE TO %li\n", new);
  if (ftruncate(self->fd, new) == -1) {
    perror("FATAL ERROR: CANNOT TRUNCATE FILE"); // FIXPAUL
    abort();
  }

  self->fpos = new;
  return old;
}


// you need to hold a read lock on rmap to call this function safely
void* smatrix_rmap_get(smatrix_t* self, uint32_t key) {
  void *ret;
  smatrix_rmap_slot_t* slot;

  pthread_rwlock_rdlock(&self->rmap.lock);
  slot = smatrix_rmap_lookup(&self->rmap, key);

  if (slot && slot->key == key) {
    ret = slot->ptr;
  } else {
    ret = NULL;
  }

  pthread_rwlock_unlock(&self->rmap.lock);

  return ret;
}


// you need to hold a write lock on rmap to call this function safely
smatrix_rmap_slot_t* smatrix_rmap_insert(smatrix_rmap_t* rmap, uint32_t key) {
  smatrix_rmap_slot_t* slot;

  if (rmap->used > rmap->size / 2) {
    smatrix_rmap_resize(rmap);
  }

  slot = smatrix_rmap_lookup(rmap, key);

  if (slot == NULL) {
    abort();
  }

  printf("INSERTING::: %i\n", key);

  if (slot->key != key) {
    rmap->used++;
    slot->key = key;
    slot->ptr = 1; // FIXPAUL this is uuuuuugly
  }

  return slot;
}

/*
  if (row == NULL) {
    row = malloc(sizeof(smatrix_row_t)); // FIXPAUL never freed :(
    row->flags = SMATRIX_ROW_FLAG_DIRTY;
    row->index = key;
    row->fpos = smatrix_falloc(self, 666);

    if (smatrix_rmap_lookup(&self->rmap, key, row) != row) {
      free(row);
      return smatrix_rmap_lookup(&self->rmap, key, row);
    }
  }
*/

// you need to hold a read or write lock on rmap to call this function safely
smatrix_rmap_slot_t* smatrix_rmap_lookup(smatrix_rmap_t* rmap, uint32_t key) {
  long int n, pos;

  pos = key % rmap->size;

  for (n = 0; n < rmap->size; n++) {
    if (!rmap->data[pos].ptr)
      break;

    if (rmap->data[pos].key == key)
      break;

    pos = (pos + 1) % rmap->size;
  }

  return &rmap->data[pos];
}

void smatrix_rmap_resize(smatrix_rmap_t* rmap) {
  smatrix_rmap_slot_t* slot;
  smatrix_rmap_t new;
  void* del;
  long int pos;

  printf("resize rmap size %li (%li used)\n", rmap->size, rmap->used);
  printf("RESIZE!!!\n");

  new.used = 0;
  new.size = rmap->size * 2;
  new.data = malloc(sizeof(smatrix_rmap_slot_t) * new.size);
  memset(new.data, 0, sizeof(smatrix_rmap_slot_t) * new.size);

  if (new.data == NULL) {
    printf("RMAP RESIZE FAILED (MALLOC)!!!\n"); // FIXPAUL
    abort();
  }

  for (pos = 0; pos < rmap->size; pos++) {
    if (!rmap->data[pos].ptr)
      continue;

    printf("FIXPAUL: reinsert\n");
    slot = smatrix_rmap_insert(&new, rmap->data[pos].key);

    if (slot == NULL) {
      printf("RMAP RESIZE FAILED (INSERT)!!!\n"); // FIXPAUL
      abort();
    }

    slot->ptr = rmap->data[pos].ptr;
  }

  del = rmap->data;

  rmap->data = new.data;
  rmap->size = new.size;
  rmap->used = new.used;

  free(del);
}

void smatrix_rmap_sync(smatrix_t* self) {
  int n, all_dirty = 0;
  char slot_buf[16];

  pthread_rwlock_rdlock(&self->rmap.lock);
  pthread_mutex_lock(&self->wlock);

  if (self->rmap_size != self->rmap.size) {
    self->rmap_size = self->rmap.size;
    self->rmap_fpos = smatrix_falloc(self, self->rmap_size * 16);

    printf("WRITE NEW RMAP TO fpos @%li\n", self->rmap_fpos);
    smatrix_meta_sync(self);

    all_dirty = 1;
  }

  for (n = 0; n < self->rmap.size; n++) {
    if (!self->rmap.data[n].ptr)
      continue;

    if ((self->rmap.data[n].ptr->flags & SMATRIX_ROW_FLAG_DIRTY) == 0 && !all_dirty)
      continue;

    // FIXPAUL what is byte ordering?
    memset(&slot_buf, 0, 16);
    memcpy(&slot_buf[4], &self->rmap.data[n].key, 4);
    memcpy(&slot_buf[8], &self->rmap.data[n].ptr->fpos, 8);

    printf("PERSIST %i->%p @ %li\n", self->rmap.data[n].key, self->rmap.data[n].ptr, self->rmap_fpos + (n * 16));
    pwrite(self->fd, &slot_buf, 16, self->rmap_fpos + (n * 16));

    // FIXPAUL flag unset needs to be a compare and swap loop
    self->rmap.data[n].ptr->flags &= ~SMATRIX_ROW_FLAG_DIRTY;
  }

  pthread_rwlock_unlock(&self->rmap.lock);
  pthread_mutex_unlock(&self->wlock);
}

void smatrix_rmap_load(smatrix_t* self) {
  int n = 0;
  size_t read, rmap_bytes;

  rmap_bytes = self->rmap_size * 16;
  self->rmap.size = self->rmap_size;
  self->rmap.used = 0;
  self->rmap.data = malloc(sizeof(smatrix_rmap_slot_t) * self->rmap.size);
  memset(self->rmap.data, 0, sizeof(smatrix_rmap_slot_t) * self->rmap.size);

  char* buf = malloc(rmap_bytes);
  read = pread(self->fd, buf, rmap_bytes, self->rmap_fpos);

  if (read != self->rmap.size * 16) {
    printf("CANNOT LOAD RMATRIX\n"); // FIXPAUL
    abort();
  }

  for (n = 0; n < self->rmap.size; n++) {
    uint32_t key  = *((uint32_t *) (buf + n * 16 + 4));
    uint32_t fpos = *((uint64_t *) (buf + n * 16 + 8));

    if (fpos) {
      self->rmap.used++;
      self->rmap.data[n].key = key;
      self->rmap.data[n].ptr = malloc(sizeof(smatrix_row_t));
      self->rmap.data[n].ptr->flags = 0;
      self->rmap.data[n].ptr->index = key;
      self->rmap.data[n].ptr->fpos = fpos;
    }
  }

  free(buf);
}

void smatrix_meta_sync(smatrix_t* self) {
  char buf[SMATRIX_META_SIZE];

  memset(&buf, 0, SMATRIX_META_SIZE);
  memset(&buf, 0x17, 8);

  // FIXPAUL what is byte ordering?
  printf("WRITE FPOS %li\n", self->rmap_fpos);
  memcpy(&buf[8],  &self->rmap_fpos, 8);
  memcpy(&buf[16], &self->rmap_size, 8);

  pwrite(self->fd, &buf, SMATRIX_META_SIZE, 0);
}

void smatrix_meta_load(smatrix_t* self) {
  char buf[SMATRIX_META_SIZE];
  size_t read;

  read = pread(self->fd, &buf, SMATRIX_META_SIZE, 0);

  if (read != SMATRIX_META_SIZE) {
    printf("CANNOT READ FILE HEADER\n"); //FIXPAUL
    abort();
  }

  if (buf[0] != 0x17 || buf[1] != 0x17) {
    printf("INVALID FILE HEADER\n"); //FIXPAUL
    abort();
  }

  // because f**k other endianess, thats why...
  memcpy(&self->rmap_fpos, &buf[8],  8);
  memcpy(&self->rmap_size, &buf[16], 8);
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
