#ifndef SMATRIX_H
#define SMATRIX_H

#define SMATRIX_INITIAL_SIZE  100000
#define SMATRIX_GROWTH_FACTOR 2

typedef struct smatrix_vec_s {
  struct smatrix_vec_s* next;
} smatrix_vec_t;

typedef struct {
  smatrix_vec_t** data;
  int             size;
} smatrix_t;

smatrix_t* smatrix_init();
smatrix_vec_t* smatrix_lookup_row(smatrix_t* self, int index);
smatrix_vec_t* smatrix_lookup(smatrix_t* self, int row_index, int col_index);
smatrix_vec_t* smatrix_lookup_col(smatrix_vec_t* row, int index);
//void smatrix_insert(smatrix* self);
void smatrix_free(smatrix_t* self);

#endif
