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

#include "recommendify.h"
#include "smatrix.h"
#include "cf.h"
#include "conn.h"
#include "version.h"

smatrix_t* db;
volatile int running = 1;
int ssock;

void quit() {
  if (running) {
    running = 0;
    shutdown(ssock, SHUT_RDWR);
  }
}

int main(int argc, char **argv) {
  int fd, opt = 1;
  struct sockaddr_in saddr;

  print_version();

  // FIXPAUL optparsing
  (void) argc; (void) argv;

  signal(SIGQUIT, quit);
  signal(SIGINT, quit);
  signal(SIGPIPE, SIG_IGN);

  db = smatrix_init();

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

  while (running) {
    fd = accept(ssock, NULL, NULL);
    printf("ACCEPT\n");

    if (fd == -1) {
      perror("accept failed");
      continue;
    }

    conn_init(fd);
  }

  close(ssock);
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
