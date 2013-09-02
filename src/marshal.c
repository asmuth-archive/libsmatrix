// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cf.h"
#include "marshal.h"

extern smatrix_t* db;

long int marshal_load_csv(char* data, size_t size) {
  cf_pset_t pset = {0};
  char *end, *start = data;
  long int num = 0;

  for (end = start; end < data + size; end++) {
    if (!(*end == '\n' || *end == ','))
      continue;

    if (pset.len < CF_MAX_PSET_LEN - 1) {
      pset.ids[pset.len] = atoi(start);
      pset.len++;
    }

    if (*end == '\n') {
      cf_add_pset(db, &pset);
      pset.len = 0; num++;
    }

    start = end + 1;
  }

  return num;
}

