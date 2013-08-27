// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdio.h>

#include "cf.h"

void cf_add_session(smatrix_t* smatrix, uint32_t* session, size_t size) {
  long i, n, c = size / sizeof(uint32_t);

  for (n = 0; n < c; n++) {
    smatrix_lookup(smatrix, session[n], 0, 1)->value++;

    for (i = 0; i < c; i++) {
      if (i != n) {
        smatrix_lookup(smatrix, session[n], session[i], 1)->value++;
      }
    }
  }
}

void cf_top_neighbors(smatrix_t* smatrix, uint32_t id, uint32_t num) {
  smatrix_vec_t *cur, *root;
  float similarity;

  root = smatrix_lookup(smatrix, id, 0, 0);

  if (root == NULL)
    return;

  printf("RECO for: %i (%i total views):\n", id, root->value);

  for (cur = root->next; cur; cur = cur->next) {
    similarity = cf_jaccard(smatrix, root, cur);
    printf("  > %i [cc %i] [sim %f]\n", cur->index, cur->value, similarity);
  }
}

float cf_jaccard(smatrix_t* smatrix, smatrix_vec_t* a, smatrix_vec_t *b) {
  uint32_t num, den;
  smatrix_vec_t *b_root = smatrix_lookup(smatrix, b->index, 0, 0);

  if (b_root == NULL)
    return 0.0;

  num = b->value;
  den = a->value + b_root->value - b->value;

  if (den == 0)
    return 0.0;

  return ((float) num / (float) den);
}
