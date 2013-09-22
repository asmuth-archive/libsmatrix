// This file is part of the "libsmatrix" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdint.h>
#include <pthread.h>
#include <stdio.h>

#ifndef SMATRIX_H
#define SMATRIX_H

#define SMATRIX_RMAP_INITIAL_SIZE 10
#define SMATRIX_GROWTH_FACTOR 2
#define SMATRIX_MAX_ROW_SIZE  4096
#define SMATRIX_MAX_ID 100000000

#define SMATRIX_META_SIZE 512

#define SMATRIX_ROW_FLAG_USED 1
#define SMATRIX_ROW_FLAG_DIRTY 2

#define SMATRIX_RMAP_FLAG_SWAPPED 4

#define SMATRIX_RMAP_MAGIC "\x23\x23\x23\x23\x23\x23\x23\x23"
#define SMATRIX_RMAP_MAGIC_SIZE 8

typedef struct smatrix_vec_s smatrix_vec_t;

struct smatrix_vec_s {
  uint32_t         index;
  uint16_t         value;
  uint8_t          refs;
  uint8_t          flags;
  smatrix_vec_t*   next;
};

typedef struct {
  uint32_t         flags;
  uint32_t         index;
  uint64_t         fpos;
  smatrix_vec_t*   head;
} smatrix_row_t;

// FIXPAUL ptr is always NULL in second first level (mem waste)
typedef struct {
  uint32_t         flags;
  uint32_t         key;
  uint64_t         value;
  void*            next;
} smatrix_rmap_slot_t;

typedef struct {
  uint32_t             flags;
  uint64_t             fpos;
  uint64_t             size;
  uint64_t             used;
  smatrix_rmap_slot_t* data;
  pthread_rwlock_t     lock;
} smatrix_rmap_t;

typedef struct {
  int              fd;
  uint64_t         fpos;
  smatrix_rmap_t   rmap;
  pthread_rwlock_t lock;
  smatrix_vec_t**  data;
  long int         size;
} smatrix_t;

smatrix_t* smatrix_open(const char* fname);
void smatrix_sync(smatrix_t* self);
void smatrix_gc(smatrix_t* self);
void smatrix_close(smatrix_t* self);

void smatrix_update(smatrix_t* self, uint32_t x, uint32_t y);

uint64_t smatrix_rmap_get(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key); // will be removed later

void smatrix_rmap_init(smatrix_t* self, smatrix_rmap_t* rmap, uint64_t size);
smatrix_rmap_slot_t* smatrix_rmap_lookup(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key);
smatrix_rmap_slot_t* smatrix_rmap_insert(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key);
void smatrix_rmap_sync(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_rmap_load(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_rmap_resize(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_swap(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_unswap(smatrix_t* self, smatrix_rmap_t* rmap);

void smatrix_meta_sync(smatrix_t* self);
void smatrix_meta_load(smatrix_t* self);


uint64_t smatrix_falloc(smatrix_t* self, uint64_t bytes);
void smatrix_ffree(smatrix_t* self, uint64_t fpos, uint64_t bytes);

// ----

uint32_t smatrix_get(smatrix_t* self, uint32_t x, uint32_t y);
void smatrix_set(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);
void smatrix_incr(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);
int smatrix_foreach(smatrix_t* self, uint32_t x);
smatrix_vec_t* smatrix_lookup(smatrix_t* self, uint32_t x, uint32_t y, int create);
smatrix_vec_t* smatrix_insert(smatrix_vec_t** row, uint32_t y);
void smatrix_resize(smatrix_t* self, uint32_t min_size);
void smatrix_truncate(smatrix_vec_t** row);
void smatrix_dump(smatrix_t* self);
void smatrix_wrlock(smatrix_t* self);
void smatrix_unlock(smatrix_t* self);
void smatrix_vec_lock(smatrix_vec_t* vec);
void smatrix_vec_unlock(smatrix_vec_t* vec);
void smatrix_vec_incref(smatrix_vec_t* vec);
void smatrix_vec_decref(smatrix_vec_t* vec);

#endif
