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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include "smatrix.h"

smatrix_t* db;

void load_preference_set(uint64_t* set, long int len) {
  long int i, n;

  for (n = 0; n < len; n++) {
    smatrix_update(db, set[n], 0);

    for (i = 0; i < len; i++) {
      if (i != n) {
        smatrix_update(db, set[n], set[i]);
      }
    }
  }
}

void load_csv(char* filename) {
  char *end, *start, *data;
  long int fd = 0, bytes, read, len = 0, max_len = 50;
  uint64_t set[50] = {0};

  fd = open(filename, O_RDONLY, 0);
  assert(fd > 0);

  bytes = lseek(fd, 0, SEEK_END);
  assert(bytes > 0);

  start = data = malloc(bytes);
  assert(data != NULL);

  read = pread(fd, data, bytes, 0);
  assert(read == bytes);

  for (end = start; end < data + bytes; end++) {
    if (!(*end == '\n' || *end == ','))
      continue;

    if (len < max_len - 1) {
      set[len] = atoi(start);
      len++;
    }

    if (*end == '\n') {
      load_preference_set(set, len);
      len = 0;
    }

    start = end + 1;
  }

  close(fd);
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  db = smatrix_open("/var/tmp/reco.db");

  if (db == NULL)
    return 1;

  load_csv(argv[1]);

  smatrix_close(db);
  return 0;
}
