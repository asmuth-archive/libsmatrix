// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <pthread.h>
#include "http.h"

#ifndef CONN_H
#define CONN_H

#define CONN_BUFFER_SIZE_MAX 262144000 // 250 MB
#define CONN_BUFFER_SIZE_INIT 4096
#define CONN_BUFFER_SIZE_GROW 4096
#define CONN_BUFFER_SIZE_HEADERS 8192

typedef struct conn_s conn_t;

struct conn_s {
  int         fd;
  long int    buffer_pos;
  long int    buffer_size;
  char*       buffer;
  long int    body_len;
  char*       body;
  http_req_t* http;
  pthread_t   thread;
};

conn_t* conn_init(int fd);
void* conn_run(void* self);
void conn_handle(conn_t* self);
void conn_handle_query(conn_t* self);
void conn_handle_index(conn_t* self);
void conn_write(conn_t* self, const char* status, char* body, size_t body_len);
void conn_reset(conn_t* self);
void conn_close(conn_t* self);

#endif
