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


  smatrix_cmap_t mycmap;
  smatrix_cmap_init(db, &mycmap, 5);

  printf("cmap_lookup(23, false) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 23, 0));

  printf("cmap_lookup(23, true) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 23, 1));

  printf("cmap_lookup(23, false) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 23, 0));

  printf("cmap_lookup(23, true) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 23, 1));

  printf("cmap_lookup(24, true) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 24, 1));
  printf("cmap_lookup(25, true) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 25, 1));
  printf("cmap_lookup(26, true) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 26, 1));
  printf("cmap_lookup(27, true) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 27, 1));
  printf("cmap_lookup(28, true) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 28, 1));
  printf("cmap_lookup(42, true) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 32, 1));
  printf("cmap_lookup(55, true) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 55, 1));
  printf("cmap_lookup(66, true) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 66, 1));
  printf("cmap_lookup(23, false) => %p\n",
    smatrix_cmap_lookup(db, &mycmap, 23, 0));

  smatrix_cmap_free(db, &mycmap);
  exit(0);

  smatrix_lock_t mylock;
  smatrix_lock_incref(&mylock);
  printf("count: %u\n", mylock.count);
  smatrix_lock_decref(&mylock);
  printf("count: %u\n", mylock.count);
  printf("try mutex\n");
  smatrix_lock_incref(&mylock);
  smatrix_lock_trymutex(&mylock);
  printf("got mutex\n");
  smatrix_lock_release(&mylock);
  printf("released mutex\n");
  smatrix_lock_incref(&mylock);
  printf("count: %u\n", mylock.count);
  smatrix_lock_decref(&mylock);
  printf("count: %u\n", mylock.count);



  smatrix_incr(db, 42, 23, 1);
  smatrix_incr(db, 42, 23, 1);


  uint64_t len,idx, ret[4096 * 8];

  len = smatrix_getrow(db, 42, ret, sizeof(ret));

  for(idx = 0; idx < len; idx++) {
    printf("(%llu,%llu)=>%llu\n", 42, ret[idx * 2], ret[idx * 2 + 1]);
  }

  smatrix_close(db);
  return 0;
}
