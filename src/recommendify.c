// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include "recommendify.h"
#include "smatrix.h"
#include "cf.h"
#include "version.h"

smatrix_t* db;

void* tmp_import() {
  printf("\n> importing /tmp/reco_in.csv...\n");
  FILE *f = fopen("/tmp/reco_in.csv", "r");
  int sess_count = 0;
  size_t buf_len = 0;
  char *buf = NULL, *cur;

  if (!f) {
    printf("cannot open file\n");
    exit(1);
  }

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
    if (sess_count == 100000) break;
    cf_add_session(db, sess, sess_len * sizeof(uint32_t));
  }

  free(buf);
  fclose(f);
  printf("   * imported %i sessions\n", sess_count);
}

int main(int argc, char **argv) {
  int fd, ssock, opt = 1;
  struct sockaddr_in saddr;

  print_version();
  db = smatrix_init();

  // FNORD
  pthread_t t1, t2, t3, t4, t5, t6, t7, t8;
  pthread_create(&t1, NULL, &tmp_import, NULL);
  //pthread_create(&t2, NULL, &tmp_import, NULL);
  //pthread_create(&t3, NULL, &tmp_import, NULL);
  //pthread_create(&t4, NULL, &tmp_import, NULL);
  pthread_join(t1, NULL);
  //pthread_join(t2, NULL);
  //pthread_join(t3, NULL);
  //pthread_join(t4, NULL);

  //long n, c = size / sizeof(uint32_t);
  //for (n = 0; n < c; n++) {
  //  printf("%i -> %i\n", n, smatrix_lookup(db, n, 0, 0)->value);
  //}

  // EOFNORD
  //cf_top_neighbors(db, 43798630, 10);
  
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  saddr.sin_port = htons(2323);

  ssock = socket(AF_INET, SOCK_STREAM, 0);

  if (ssock == -1) {
    perror("create socket failed!\n");
    return 1;
  }

  if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt(SO_REUSEADDR)");
    return 1;
  }

  if (bind(ssock, (struct sockaddr *) &saddr, sizeof(saddr)) == -1) {
    perror("bind failed");
    return 1;
  }

  if (listen(ssock, 1024) == -1) {
    perror("listen failed");
    return 1;
  }

  printf("\n> listening on 0.0.0.0:2323\n");

  for (;;) {
    fd = accept(ssock, NULL, NULL);

    if (fd == -1) {
      perror("accept failed");
      continue;
    }

    printf("ACCEPT: %i\n", fd);
  }

  //smatrix_dump(db);
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
