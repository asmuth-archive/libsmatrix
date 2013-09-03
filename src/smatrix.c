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

smatrix_t* smatrix_init() {
  smatrix_t* self = malloc(sizeof(smatrix_t));

  self->size = SMATRIX_INITIAL_SIZE;
  self->data = malloc(sizeof(void *) * self->size);

  if (self->data == NULL) {
    free(self);
    return NULL;
  }

  memset(self->data, 0, sizeof(void *) * self->size);
  pthread_rwlock_init(&self->lock, NULL);

  return self;
}

smatrix_vec_t* smatrix_lookup(smatrix_t* self, uint32_t x, uint32_t y, int create) {
  smatrix_vec_t *col = NULL, **row = NULL;

  pthread_rwlock_rdlock(&self->lock);

  if (x > self->size) {
    if (create) {
      smatrix_wrlock(self);

      if (x > self->size)
        smatrix_resize(self, x + 1);

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

  smatrix_vec_incref(*col);
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
  new->next  = next;

  for (; next; row_len++)
    next = next->next;

  if (row_len > SMATRIX_MAX_ROW_SIZE)
    smatrix_truncate(row);

  return new;
}

void smatrix_resize(smatrix_t* self, uint32_t min_size) {
  uint32_t new_size = self->size;

  while (new_size < min_size) {
    new_size = new_size * SMATRIX_GROWTH_FACTOR;
  }

  smatrix_vec_t** new_data = malloc(sizeof(void *) * new_size);
  memcpy(new_data, self->data, sizeof(void *) * self->size);
  memset(new_data, 0, sizeof(void *) * (new_size - self->size));

  free(self->data);

  self->data = new_data;
  self->size = new_size;
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

  free(delete);
}

void smatrix_free(smatrix_t* self) {
  smatrix_vec_t *cur, *tmp;
  uint32_t n;

  for (n = 0; n < self->size; n++) {
    cur = self->data[n];

    while (cur) {
      tmp = cur;
      cur = cur->next;
      free(tmp);
    }
  }

  pthread_rwlock_destroy(&self->lock);

  free(self->data);
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
}

void smatrix_vec_unlock(smatrix_vec_t* vec) {
}

void smatrix_vec_incref(smatrix_vec_t* vec) {
}

void smatrix_vec_decref(smatrix_vec_t* vec) {
}
