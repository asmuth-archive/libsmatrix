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
  smatrix_vec_t *cur, *vec = smatrix_lookup(smatrix, id, 0, 0);

  if (vec == NULL)
    return;

  printf("RECO for: %i (%i total views):\n", id, vec->value);

  for (; vec; vec = vec->next) {
    cur = smatrix_lookup(smatrix, vec->index, 0, 0);

    if (cur == NULL)
      continue;

    printf("  > %i [cc %i] [sim ...] [total %i]\n", vec->index, vec->value, cur->value);
  }
}
