// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "recommendify.h"
#include "smatrix.h"
#include "cf.h"
#include "version.h"

smatrix_t* db;

int main(int argc, char **argv) {
  print_version();

  db = smatrix_init();

  // FNORD
  printf("> importing /tmp/reco_in.csv...\n");
  int sess_count;
  size_t buf_len;
  char *buf, *cur;
  FILE *f = fopen("/tmp/reco_in.csv", "r");

  while (1) {
    if (getline(&buf, &buf_len, f) == -1)
     break;

    uint32_t sess[500];
    int sess_len = 0;
    char *last = buf;

    for (cur = buf; *cur; cur++) {
      if (*cur == ',' || *cur == '\n') {
        *cur = 0;
        if ((unsigned long) sess_len < (sizeof(sess) / sizeof(uint32_t)) - 1) {
          sess[sess_len] = atoi(last);
          sess_len++;
        }
        last = cur + 1;
      }
    }

    sess_count++;
    if (sess_count == 10000) break;
    cf_add_session(db, sess, sess_len * sizeof(uint32_t));
  }

  fclose(f);
  printf("   * imported %i sessions\n", sess_count);
  // EOFNORD

  /*
  smatrix_vec_t* val = smatrix_lookup(db, 53415246, 22361353, 1);
  val->value += 1;

  smatrix_lookup(db, 53415246, 22361353, 1);
  smatrix_lookup(db, 53415249, 22361353, 1);
  smatrix_lookup(db, 53415249, 22361353, 1);
  smatrix_lookup(db, 53415249, 22361359, 1);
  smatrix_lookup(db, 53415249, 22361359, 1);
  */

  smatrix_free(db);

  return 0;
}

void print_version() {
  printf(
    VERSION_STRING,
    VERSION_MAJOR,
    VERSION_MINOR,
    VERSION_PATCH
  );
}
