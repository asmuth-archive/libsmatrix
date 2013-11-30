// This file is part of the "libsmatrix" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "smatrix.h"

smatrix_t* my_smatrix;

// libsmatrix example: simple CF based recommendation engine
int main(int argc, char **argv) {
  my_smatrix = smatrix_open(NULL);

  // one preference set = list of items in one session
  // e.g. list of viewed items by the same user
  // e.g. list of bought items in the same checkout
  uint32_t input_ids[5] = {12,52,63,76,43};
  import_preference_set(input_ids, 5);

  // generate recommendations (similar items) for item #76
  void neighbors_for_item(76);

  smatrix_close(my_smatrix);
  return 0;
}

// train / add a preference set (list of items in one session)
void import_preference_set(uint32_t* ids, uint32_t num_ids) {
  uint32_t i, n;

  for (n = 0; n < num_ids; n++) {
    smatrix_incr(my_smatrix, ids[n], 0, 1);

    for (i = 0; i < pset->len; i++) {
      if (i != n) {
        smatrix_incr(my_smatrix, ids[n], ids[i], 1);
      }
    }
  }
}

// get recommendations for item with id "item_id"
void neighbors_for_item(uint32_t item_id)
  uint32_t neighbors, *row, total;

  total = smatrix_get(my_smatrix, item_id, 0);
  neighbors = smatrix_getrow(my_smatrix, item_id, row, 8192);

  for (pos = 0; pos < neighbors; pos++) {
    uint32_t cur_id = row[pos * 2];

    printf("found neighbor for item %u: item %u with distance %f\n",
      item_id, cf_cosine(smatrix, cur_id, row[pos * 2 + 1], total));
  }

  free(row);
}

// calculates the cosine vector distance between two items
double cf_cosine(smatrix_t* smatrix, uint32_t b_id, uint32_t cc_count, uint32_t a_total) {
  uint32_t b_total;
  double num, den;

  b_total = smatrix_get(smatrix, b_id, 0);

  if (b_total == 0)
    b_total = 1;

  num = cc_count;
  den = sqrt((double) a_total) * sqrt((double) b_total);

  if (den == 0.0)
    return 0.0;

  if (num > den)
    return 0.0;

  return (num / den);
}

