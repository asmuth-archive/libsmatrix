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
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include "conn.h"
#include "http.h"
#include "smatrix.h"
#include "cf.h"
#include "marshal.h"

extern smatrix_t* db;

conn_t* conn_init(int fd) {
  conn_t* self = malloc(sizeof(conn_t));
  self->fd = fd;
  self->buffer_pos = 0;
  self->buffer_size = CONN_BUFFER_SIZE_INIT;
  self->buffer = malloc(self->buffer_size);

  self->http = http_req_init();
  pthread_create(&self->thread, NULL, &conn_run, self);

  return self;
}

void* conn_run(void* self_) {
  long int chunk, body_bytes_left, remaining, body_pos, new_buffer_size;
  char* new_buffer;
  conn_t *self = (conn_t *) self_;

keepalive:

  remaining = 0;
  body_pos = 0;
  body_bytes_left = 0;

  while (body_pos == 0 || body_bytes_left > 0) {
    remaining = self->buffer_size - self->buffer_pos;

    if (remaining <= 1) {
      new_buffer_size = self->buffer_size + body_bytes_left;
      new_buffer_size += CONN_BUFFER_SIZE_GROW;

      if (new_buffer_size > CONN_BUFFER_SIZE_MAX) {
        conn_write(self, "413 Request Entity Too Large",
          "maximum buffer size exeeded\n", 28);

        goto close;
      }

      new_buffer = 0;
      new_buffer = realloc(self->buffer, new_buffer_size);

      if (new_buffer == NULL)
        goto close;

      self->buffer = new_buffer;
      self->buffer_size = new_buffer_size;
      remaining = new_buffer_size - self->buffer_pos;
    }

    chunk = read(self->fd, self->buffer + self->buffer_pos, remaining);

    if (chunk < 1)
      goto close;

    self->buffer_pos += chunk;

    if (body_pos == 0)
      body_pos = http_read(self->http,self->buffer, self->buffer_pos);

    body_bytes_left =
      self->http->content_length - self->buffer_pos + body_pos + 1;
  }

  if (body_pos < 0)
    goto close;

  self->body = self->buffer + body_pos + 1;
  self->body_len = self->http->content_length;

  conn_handle(self);

  if (self->http->keepalive) {
    conn_reset(self);
    goto keepalive;
  }

close:

  conn_close(self);
  return NULL;
}

void conn_handle(conn_t* self) {
  switch (self->http->uri_argc) {

    case 2:
      if (strncmp(self->http->uri, "/query", 6) == 0)
        return conn_handle_query(self);

    case 1:
      if (strncmp(self->http->uri, "/index", 6) == 0)
        return conn_handle_index(self);

      if (strncmp(self->http->uri, "/ping", 5) == 0)
        return conn_write(self, "200 OK", "pong\n", 5);

    default:
      conn_write(self, "404 Not Found", "not found\n", 10);

  }
}

#define CONN_RESP_BUF_SIZE 8192

void conn_handle_query(conn_t* self) {
  uint32_t n, id;
  char buf[CONN_RESP_BUF_SIZE];
  size_t buf_len;

  if (self->http->method != 1)
    return conn_write(self, "400 Bad Request",
      "please use HTTP GET\n", 20);

  id = atoi(self->http->uri_argv[1] + 1);
  cf_reco_t* recos = cf_recommend(db, id);

  if (recos == NULL) {
    conn_write(self, "200 OK", "no recos found\n", 15);
    return;
  }

  buf_len = snprintf(buf, CONN_RESP_BUF_SIZE,
    "quality,%f\n", recos->quality);

  for (n = 0; recos->ids[n] && n < SMATRIX_MAX_ROW_SIZE; n++) {
    buf_len += snprintf(buf + buf_len, CONN_RESP_BUF_SIZE - buf_len,
      "reco,%i,%f\n", recos->ids[n], recos->similarities[n]);
  }

  conn_write(self, "201 Found", buf, buf_len);
}

void conn_handle_index(conn_t* self) {
  char resp[4096];
  long int sessions_imported;

  if (self->http->method != 2)
    return conn_write(self, "400 Bad Request",
      "please use HTTP POST\n", 21);

  if (self->http->content_length <= 0)
    return conn_write(self, "411 Length Required",
      "please set the Content-Length header\n", 37);

  sessions_imported = marshal_load_csv(self->body, self->body_len);

  sprintf(resp, "imported %li sessions\n", sessions_imported);
  conn_write(self, "200 Created", resp, strlen(resp));
}

void conn_write(conn_t* self, const char* status, char* body, size_t body_len) {
  struct iovec handle[2];
  char   headers[CONN_BUFFER_SIZE_HEADERS];
  size_t headers_len, bytes_written;

  headers_len = snprintf(headers, CONN_BUFFER_SIZE_HEADERS,
    "HTTP/1.1 %s\r\n" \
    "Server: recommendify-v2.0.0\r\n" \
    "Connection: Keep-Alive\r\n"\
    "Content-Type: text/plain\r\n"\
    "Content-Length: %lu\r\n\r\n",
    status, body_len
  );

  handle[0].iov_base = headers;
  handle[0].iov_len  = headers_len;
  handle[1].iov_base = body;
  handle[1].iov_len  = body_len;

  bytes_written = writev(self->fd,
    (const struct iovec *) &handle, 2);

  if (bytes_written != body_len + headers_len) {
    printf("ERROR WRITING?\n"); // FIXPAUL
  }
}

void conn_reset(conn_t* self) {
  self->buffer_pos = 0;
  http_req_reset(self->http);
}

void conn_close(conn_t* self) {
  close(self->fd);
  http_req_free(self->http);
  free(self->buffer);
  free(self);

  pthread_exit(NULL);
}
