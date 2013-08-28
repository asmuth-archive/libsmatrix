// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "conn.h"
#include "http.h"
#include "smatrix.h"
#include "cf.h"

extern smatrix_t* db;

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
      conn_handle(self);
    }
  }
}

void conn_handle(conn_t* self) {
  printf("URL: %s (%s)\n", self->http->uri, self->http->uri_argv[1]);

  printf("http request finished!\n");

  if (self->http->uri_argc > 1 && strncmp(self->http->uri_argv[0], "/query", 6) == 0) {
    *self->http->uri_argv[2] = 0;
    uint32_t n, id = atoi(self->http->uri_argv[1] + 1);
    printf("creating recos for %i\n", id);
    cf_reco_t* recos = cf_recommend(db, id);

    if (recos) {
      for (n = 0; recos->ids[n] && n < SMATRIX_MAX_ROW_SIZE; n++) {
        printf("  ° %i -> %f\n", recos->ids[n], recos->similarities[n]);
      }
    }

    printf("fuquery: %i\n", id);
    return;
  }

  if (strncmp(self->http->uri_argv[0], "/learn", 6) == 0) {
    printf("fulearn\n");
    return;
  }

  char* resp = "HTTP/1.1 404 Not Found\r\nServer: recommendify-v2.0.0\r\nConnection: Keep-Alive\r\nContent-Length: 11\r\n\r\nnot found\r\n";
  conn_write(self, resp, strlen(resp));
}

void conn_write(conn_t* self, char* buf, size_t buf_len) {
  int chunk;

  chunk = write(self->fd, buf, buf_len);

  printf("WRITE %i\n", chunk);
}

void conn_close(conn_t* self) {
  printf("CLOSE %i\n", self->fd);

  http_req_free(self->http);
  free(self);

  pthread_exit(NULL);
}
