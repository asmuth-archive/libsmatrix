// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <pthread.h>

#ifndef CONN_H
#define CONN_H

typedef struct conn_s conn_t;

struct conn_s {
  int       fd;
  pthread_t thread;
};

conn_t* conn_init(int fd);
void conn_run(conn_t* self);

#endif
