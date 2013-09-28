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

  for (m = 0; m < 10; m++) {
    for (n = 1; n < 30; n++) {
      for (i = 1; i < 50; i++) {
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

  printf("\nstarting\n");

  for (n = 0; n < num_threads; n++)
    pthread_create(&threads[n], NULL, test, NULL);

  for (n = 0; n < num_threads; n++)
    pthread_join(threads[n], NULL);

  printf("\ndone\n");

  for (n = 0; n < db->cmap.size; n++) {
    if (db->cmap.data[n].flags == 0) continue;
    for (m = 0; m < db->cmap.data[n].rmap->size; m++) {
      if (db->cmap.data[n].rmap->data[m].key == 0) continue;
      printf("(%u,%u) => %lu, ", db->cmap.data[n].key, db->cmap.data[n].rmap->data[m].key, db->cmap.data[n].rmap->data[m].value);
      if (n*m % 5 == 0) printf("\n");
    }
  }

  smatrix_close(db);
  return 0;
}
