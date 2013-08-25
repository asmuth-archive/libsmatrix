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

void smatrix_resize(smatrix_t* self, int min_size) {
  int new_size = self->size;

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

smatrix_vec_t* smatrix_lookup(smatrix_t* self, int row_index, int col_index, int insert) {
  smatrix_vec_t *col = NULL, **row = NULL;

  if (row_index > self->size) {
    if (insert) {
      smatrix_resize(self, row_index + 1);
    } else {
      return NULL;
    }
  }

  row = self->data + col_index;

  if (*row) {
    // lookup col
  } else {
    if (insert) {
    // insert a new row
      printf("create row: %i\n", row_index);
      *row = col = malloc(sizeof(smatrix_vec_t));
    } else {
      return NULL;
    }
  }

  printf("found col: %p\n", col);

  if (col == NULL && insert) {
    // insert col
  }

  return col;
}

void smatrix_free(smatrix_t* self) {
  free(self->data);
  free(self);
}
