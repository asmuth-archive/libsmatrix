// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include "smatrix.h"

smatrix_t* db;

void* test(void* fnord) {
  uint64_t i, n, m;

  for (m = 1; m < 2; m++) {
    for (n = 23; n < 100; n++) {
      for (i = 0; i < 3000; i++) {
        smatrix_incr(db, n * m, i * m * n, 1);
        smatrix_get(db, n * m, i * m * n);
      }

      smatrix_sync(db);
    }

    //smatrix_gc(db);
  }

  return NULL;
}

int main(int argc, char **argv) {
  int n, num_threads = 1;
  pthread_t threads[num_threads];

  db = smatrix_open("/var/tmp/reco.db");

  if (db == NULL)
    abort();

  printf("...\n");
/*
  for (n = 0; n < num_threads; n++)
    pthread_create(&threads[n], NULL, test, NULL);

  for (n = 0; n < num_threads; n++)
    pthread_join(threads[n], NULL);
*/

  smatrix_incr(db, 42, 123, 1);
  smatrix_incr(db, 42, 23, 1);
  smatrix_incr(db, 42, 23, 1);
  smatrix_incr(db, 42, 17, 1);

  uint64_t idx, ret[4096 * 8];
  printf("smatrix_rowlen %lu, %lu\n", smatrix_rowlen(db, 42), smatrix_getrow(db, 42, ret, sizeof(ret)));

  //for(idx = smatrix_getrow(db, 42, ret, sizeof(ret)); idx > 2; idx -= 2) {
  //  if (ret[idx -1])
  //    printf("(%llu,%llu)=>%llu\n", 42, ret[idx - 1], ret[idx - 2]);
  //}

  smatrix_close(db);
  return 0;
}
