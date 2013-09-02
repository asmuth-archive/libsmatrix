// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdlib.h>
#include <stdint.h>
#include "cf.h"
#include "marshal.h"

extern smatrix_t* db;

long int marshal_load_csv(char* data, size_t size) {
  char *end, *start = data;
  long int sess_ind = 0, num = 0;
  uint32_t sess[MARSHAL_MAX_SESS_LEN];

  for (end = start; end < data + size; end++) {
    if (*end != '\n' && *end != ',')
      continue;

    if (sess_ind < MARSHAL_MAX_SESS_LEN - 1) {
      sess[sess_ind] = atoi(start);
      sess_ind++;
    }

    if (*end == '\n') {
      cf_add_session(db, sess, sess_ind + 1);
      sess_ind = 0; num++;
    }

    start = end + 1;
  }

  return num;
}

