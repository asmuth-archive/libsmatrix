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

  smatrix_vec_t** new_data = malloc(sizeof(void *) * new_size);
  memcpy(new_data, self->data, sizeof(void *) * self->size);
  memset(new_data, 0, sizeof(void *) * (new_size - self->size));

  free(self->data);

  self->data = new_data;
  self->size = new_size;
}

smatrix_vec_t* smatrix_lookup(smatrix_t* self, uint32_t x, uint32_t y, int create) {
  smatrix_vec_t *col = NULL, **row = NULL;

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

    *row = col = malloc(sizeof(smatrix_vec_t));
    col->index = y;
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


  printf("found col (1): %p\n", col);

  // insert column
  if (col == NULL && create) {
    // insert col
  }

  printf("found col (2): %p\n", col);

  return col;
}

void smatrix_free(smatrix_t* self) {
  free(self->data);
  free(self);
}
