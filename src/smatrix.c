#include <stdio.h>
#include <stdlib.h>

#include "smatrix.h"

smatrix_t* smatrix_init() {
  smatrix_t* self = malloc(sizeof(smatrix_t));

  self->size = SMATRIX_INITIAL_SIZE;
  self->data = malloc(sizeof(void *) * self->size);

  if (self->data == NULL) {
    free(self);
    return NULL;
  }

  return self;
}

smatrix_vec_t* smatrix_lookup_row(smatrix_t* self, int index) {
  if (index >= self->size)
    return NULL;

  return self->data[index];
}

smatrix_vec_t* smatrix_lookup_col(smatrix_vec_t* row, int index) {
  // here be dragons

  return row;
}

smatrix_vec_t* smatrix_lookup(smatrix_t* self, int row_index, int col_index) {
  smatrix_vec_t* row = smatrix_lookup_row(self, row_index);

  if (row == NULL)
    return NULL;

  smatrix_vec_t* col = smatrix_lookup_col(row, col_index);

  if (col == NULL)
    return NULL;

  return col;
}

void smatrix_free(smatrix_t* self) {
  free(self->data);
  free(self);
}
