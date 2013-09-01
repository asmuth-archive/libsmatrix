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
#include "conn.h"
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
  return NULL;
}

int main(int argc, char **argv) {
  int fd, ssock, opt = 1;
  struct sockaddr_in saddr;

  print_version();
  db = smatrix_init();

  //tmp_import();

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

    conn_init(fd);
  }

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
