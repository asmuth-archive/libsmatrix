// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdint.h>

#ifndef SMATRIX_H
#define SMATRIX_H

#define SMATRIX_INITIAL_SIZE  1024
#define SMATRIX_GROWTH_FACTOR 2

typedef struct smatrix_vec_s {
  uint32_t index;
  uint32_t value;
  struct smatrix_vec_s* next;
} smatrix_vec_t;

typedef struct {
  smatrix_vec_t** data;
  uint32_t        size;
} smatrix_t;

smatrix_t* smatrix_init();
smatrix_vec_t* smatrix_lookup(smatrix_t* self, uint32_t x, uint32_t y, int create);
void smatrix_free(smatrix_t* self);

#endif
