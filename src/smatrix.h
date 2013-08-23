#ifndef SMATRIX_H
#define SMATRIX_H

#define SMATRIX_INITIAL_SIZE  100000
#define SMATRIX_GROWTH_FACTOR 2

typedef struct {
  int size;
} smatrix;

smatrix* smatrix_init();
//void smatrix_lookup(smatrix* self);
//void smatrix_insert(smatrix* self);
void smatrix_free(smatrix* self);

#endif
