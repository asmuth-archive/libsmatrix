// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdio.h>

#include "cf.h"
#include "string.h"

// FIXPAUL actualy increment operation is not atomic!
void cf_add_pset(smatrix_t* smatrix, cf_pset_t* pset) {
  long int i, n;

  for (n = 0; n < pset->len; n++) {
    smatrix_lookup(smatrix, pset->ids[n], 0, 1)->value++;

    for (i = 0; i < pset->len; i++) {
      if (i != n) {
        smatrix_lookup(smatrix, pset->ids[n], pset->ids[i], 1)->value++;
      }
    }
  }
}

cf_reco_t* cf_recommend(smatrix_t* smatrix, uint32_t id) {
  int pos;
  smatrix_vec_t *cur, *root;
  cf_reco_t* result;

  root = smatrix_lookup(smatrix, id, 0, 0);

  if (root == NULL)
    return NULL;

  result = malloc(sizeof(cf_reco_t));
  cur = root->next;

  printf("RECOS FOR %i (%i total @ %i)\n", id, root->value, index);
  for (pos = 0; cur; pos++) {
    result->ids[pos] = cur->index;
    result->similarities[pos] = cf_jaccard(smatrix, root, cur);
    result->len++;
    cur = cur->next;
  }

  for (; pos < SMATRIX_MAX_ROW_SIZE; pos++) {
    result->ids[pos] = 0;
    result->similarities[pos] = 0;
  }

  result->quality = root->value;
  return result;
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

  printf("   COMPARE %i: cc %i, total %i @ %i, sim %f\n", b->index, b->value, b_root->value, b_root->index, ((float) num / (float) den));

  return ((float) num / (float) den);
}
