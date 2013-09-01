// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
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
  int chunk, remaining, body_pos;

  for (;;) {
    body_pos = 0;

    while (body_pos == 0) {
      remaining = CONN_BUF_SIZE - self->buffer_pos;

      if (remaining == 0) {
        conn_close(self);
        return NULL;
      }

      chunk = read(self->fd, self->buffer + self->buffer_pos, remaining);

      if (chunk < 1) {
        conn_close(self);
        return NULL;
      }

      self->buffer_pos += chunk;
      body_pos = http_read(self->http, self->buffer, self->buffer_pos);
    }

    if (body_pos < 0) {
      conn_close(self);
      return NULL;
    }

    conn_handle(self);
    conn_reset(self);
  }
}

void conn_handle(conn_t* self) {
  switch (self->http->uri_argc) {

    case 2:
      if (strncmp(self->http->uri_argv[0], "/query", 6) == 0)
        return conn_handle_query(self);

    case 1:
      if (strncmp(self->http->uri_argv[0], "/index", 6) == 0)
        return conn_handle_index(self);

      if (strncmp(self->http->uri_argv[0], "/ping", 5) == 0)
        return conn_write_http(self, "200 OK", "pong\n", 5);

    default:
      conn_write_http(self, "404 Not Found", "not found\n", 10);

  }
}

void conn_handle_query(conn_t* self) {
  if (self->http->method != 1)
    return conn_write_http(self, "400 Bad Request",
      "please use HTTP GET\n", 20);

  *self->http->uri_argv[2] = 0;
  uint32_t n, id = atoi(self->http->uri_argv[1] + 1);
  printf("creating recos for %i\n", id);
  cf_reco_t* recos = cf_recommend(db, id);

  if (recos) {
    for (n = 0; recos->ids[n] && n < SMATRIX_MAX_ROW_SIZE; n++) {
      printf("  ° %i -> %f\n", recos->ids[n], recos->similarities[n]);
    }
  }

  conn_write_http(self, "200 OK", "fnord\n", 6);
}

void conn_handle_index(conn_t* self) {
  if (self->http->method != 2)
    return conn_write_http(self, "400 Bad Request",
      "please use HTTP POST\n", 21);

  printf("INDEX %i\n", self->http->method);
  //char* resp = "HTTP/1.1 200 OK\r\nServer: recommendify-v2.0.0\r\nConnection: Keep-Alive\r\nContent-Length: 6\r\n\r\npong\r\n";
  //conn_write(self, resp, strlen(resp));
}

// FIXPAUL this should be one writev syscall, not two write syscalls
void conn_write_http(conn_t* self, const char* status, char* body, size_t body_len) {
  char headers[4096];

  snprintf(headers, 4096,
    "HTTP/1.1 %s\r\nServer: recommendify-v2.0.0\r\nConnection: Keep-Alive\r\nContent-Length: %i\r\n\r\n",
    status, body_len
  );

  conn_write(self, headers, strlen(headers));
  conn_write(self, body, body_len);
}

void conn_write(conn_t* self, char* buf, size_t buf_len) {
  int chunk;

  chunk = write(self->fd, buf, buf_len);

  printf("WRITE %i\n", chunk);
}

void conn_reset(conn_t* self) {
  self->buffer_pos = 0;
  http_req_reset(self->http);
}

void conn_close(conn_t* self) {
  printf("CLOSE %i\n", self->fd);

  http_req_free(self->http);
  free(self);

  pthread_exit(NULL);
}
