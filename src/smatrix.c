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
  memset(self->data, 0, sizeof(void *) * self->size);

  if (self->data == NULL) {
    free(self);
    return NULL;
  }

  return self;
}

void smatrix_resize(smatrix_t* self, uint32_t min_size) {
  uint32_t new_size = self->size;

  while (new_size < min_size) {
    new_size = new_size * SMATRIX_GROWTH_FACTOR;
  }

  // FIXPAUL mutex start
  smatrix_vec_t** new_data = malloc(sizeof(void *) * new_size);
  memcpy(new_data, self->data, sizeof(void *) * self->size);
  memset(new_data, 0, sizeof(void *) * (new_size - self->size));

  free(self->data);

  self->data = new_data;
  self->size = new_size;
  // FIXPAUL mutex end
}

smatrix_vec_t* smatrix_lookup(smatrix_t* self, uint32_t x, uint32_t y, int create) {
  smatrix_vec_t *col = NULL, **row = NULL, *cur;

  if (x > self->size) {
    if (create) {
      smatrix_resize(self, x + 1);
    } else {
      return NULL;
    }
  }

  row = self->data + x;

  if (*row == NULL) {
    if (!create)
      return NULL;

    // FIXPAUL mutex start
    if (*row == NULL) {
      *row = col = malloc(sizeof(smatrix_vec_t));
      col->next = NULL;
      col->index = y;
    }
    // FIXPAUL mutex end
  }

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
    cur = *row;

    // FIXPAUL mutex start
    while (cur->next && cur->next->index < y)
      cur = cur->next;

    col = malloc(sizeof(smatrix_vec_t));
    col->index = y;
    col->next  = cur->next;
    cur->next  = col;
    // FIXPAUL mutex end
  }

  return col;
}

void smatrix_free(smatrix_t* self) {
  smatrix_vec_t *cur, *tmp;
  uint32_t n;
  unsigned long items_freed = 0;

  printf("check... %i\n", self->size);

  for (n = 0; n < self->size; n++) {
    cur = self->data[n];

    while (cur) {
      tmp = cur;
      cur = cur->next;
      items_freed++;
      free(tmp);
    }
  }

  printf("freed %lu items\n", items_freed);

  free(self->data);
  free(self);
}
