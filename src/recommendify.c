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
    for (n = 23; n < 10000; n++) {
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
  int n, num_threads = 8;
  pthread_t threads[num_threads];

  db = smatrix_open("/var/tmp/reco.db");

  if (db == NULL)
    abort();

  for (n = 0; n < num_threads; n++)
    pthread_create(&threads[n], NULL, test, NULL);

  for (n = 0; n < num_threads; n++)
    pthread_join(threads[n], NULL);

  smatrix_close(db);
  return 0;
}
