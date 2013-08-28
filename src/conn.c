// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdlib.h>
#include <unistd.h>
#include "conn.h"
#include "http.h"

conn_t* conn_init(int fd) {
  conn_t* self = malloc(sizeof(conn_t));
  self->fd = fd;
  self->buffer_pos = 0;

  self->http = http_req_init();
  pthread_create(&self->thread, NULL, &conn_run, self);

  return self;
}

void* conn_run(void* self_) {
  conn_t *self = (conn_t *) self_;
  int chunk, remaining, ret;

  printf("RUN! %i\n", self->fd);

  for (;;) {
    remaining = CONN_BUF_SIZE - self->buffer_pos;

    if (remaining == 0) {
      printf("buffer overflow :(\n");
      conn_close(self);
    }

    chunk = read(self->fd, self->buffer + self->buffer_pos, remaining);

    if (chunk < 1) {
      conn_close(self);
      return NULL;
    }

    self->buffer_pos += chunk;
    ret = http_read(self->http, self->buffer, self->buffer_pos);

    if (ret == -1) {
      conn_close(self);
      return NULL;
    }

    if (ret > 0) {
      printf("http request finished!\n");
    }
  }
}

void conn_close(conn_t* self) {
  printf("CLOSE %i\n", self->fd);

  http_req_free(self->http);
  free(self);

  pthread_exit(NULL);
}
