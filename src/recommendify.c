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

  for (m = 0; m < 50; m++) {
    for (n = 23; n < 100; n++) {
      for (i = 0; i < 30; i++) {
        smatrix_lookup(db, n, i, 1);
      }
    }
  }

  return NULL;
}

int main(int argc, char **argv) {
  int n,m, num_threads = 4;
  pthread_t threads[num_threads];

  printf("\nloading\n");
  db = smatrix_open("/var/tmp/reco.db");

  if (db == NULL)
    abort();

  for (n = 0; n < num_threads; n++)
    pthread_create(&threads[n], NULL, test, NULL);

  for (n = 0; n < num_threads; n++)
    pthread_join(threads[n], NULL);

  for (n = 0; n < db->cmap.size; n++) {
    if (db->cmap.data[n].flags == 0) continue;
    for (m = 0; m < db->cmap.data[n].rmap->size; m++) {
      if (db->cmap.data[n].rmap->data[m].key == 0) continue;
      printf("(%u,%u) => %lu\n", db->cmap.data[n].key, db->cmap.data[n].rmap->data[m].key, db->cmap.data[n].rmap->data[m].value);
    }
  }

  smatrix_close(db);
  return 0;
}
