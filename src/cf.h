// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdlib.h>
#include <stdint.h>

#include "smatrix.h"

#ifndef CF_H
#define CF_H

#define CF_MAX_PSET_LEN 512

typedef struct {
  uint32_t ids[SMATRIX_MAX_ROW_SIZE];
  float    similarities[SMATRIX_MAX_ROW_SIZE];
  int      len;
  float    quality;
} cf_reco_t;

typedef struct {
  uint32_t ids[CF_MAX_PSET_LEN];
  int      len;
} cf_pset_t;


void cf_add_pset(smatrix_t* smatrix, cf_pset_t* pset);
cf_reco_t* cf_recommend(smatrix_t* smatrix, uint32_t id);
float cf_jaccard(smatrix_t* smatrix, smatrix_vec_t* a, smatrix_vec_t *b);
float cf_cosine(smatrix_t* smatrix, smatrix_vec_t* a, smatrix_vec_t *b);

#endif
