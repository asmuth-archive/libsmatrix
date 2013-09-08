// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdint.h>
#include <pthread.h>

#ifndef SMATRIX_H
#define SMATRIX_H

#define SMATRIX_INITIAL_SIZE  120000000
#define SMATRIX_GROWTH_FACTOR 2
#define SMATRIX_MAX_ROW_SIZE  4096
#define SMATRIX_MAX_ID 100000000

typedef struct smatrix_vec_s smatrix_vec_t;

struct smatrix_vec_s {
  uint32_t         index;
  uint16_t         value;
  uint8_t          refs;
  uint8_t          flags;
  smatrix_vec_t*   next;
};

typedef struct {
  smatrix_vec_t**  data;
  long int         size;
  pthread_rwlock_t lock;
} smatrix_t;

smatrix_t* smatrix_init();
uint32_t smatrix_get(smatrix_t* self, uint32_t x, uint32_t y);
void smatrix_set(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);
void smatrix_incr(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);
int smatrix_foreach(smatrix_t* self, uint32_t x);
void smatrix_sync(smatrix_t* self, uint32_t x);
void smatrix_free(smatrix_t* self);

static smatrix_vec_t* smatrix_lookup(smatrix_t* self, uint32_t x, uint32_t y, int create);
static smatrix_vec_t* smatrix_insert(smatrix_vec_t** row, uint32_t y);
static void smatrix_resize(smatrix_t* self, uint32_t min_size);
static void smatrix_truncate(smatrix_vec_t** row);
static void smatrix_dump(smatrix_t* self);
static void smatrix_wrlock(smatrix_t* self);
static void smatrix_unlock(smatrix_t* self);
static void smatrix_vec_lock(smatrix_vec_t* vec);
static void smatrix_vec_unlock(smatrix_vec_t* vec);
static void smatrix_vec_incref(smatrix_vec_t* vec);
static void smatrix_vec_decref(smatrix_vec_t* vec);

#endif
