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
#define SMATRIX_RMAP_FLAG_DIRTY 8

#define SMATRIX_RMAP_MAGIC "\x23\x23\x23\x23\x23\x23\x23\x23"
#define SMATRIX_RMAP_MAGIC_SIZE 8

#define SMATRIX_OP_SET 1
#define SMATRIX_OP_INCR 2
#define SMATRIX_OP_DECR 4

#define SMATRIX_HEAD_SIZE 16
#define SMATRIX_SLOT_SIZE 16

typedef struct {
  uint32_t             flags;
  uint64_t             fpos;
  uint64_t             size;
  uint64_t             used;
  void*                data;
  pthread_rwlock_t     lock;
} smatrix_rmap_t;

typedef struct {
  int              fd;
  uint64_t         fpos;
  uint64_t         mem;
  smatrix_rmap_t   rmap;
  pthread_mutex_t  lock;
} smatrix_t;

smatrix_t* smatrix_open(const char* fname);
/*
void smatrix_sync(smatrix_t* self);
void smatrix_gc(smatrix_t* self);
void smatrix_mfree(smatrix_t* self, uint64_t bytes);
void smatrix_close(smatrix_t* self);
void smatrix_ffree(smatrix_t* self, uint64_t fpos, uint64_t bytes);
uint64_t smatrix_update(smatrix_t* self, uint32_t x, uint32_t y, uint32_t op, uint64_t opval);
smatrix_rmap_t* smatrix_retrieve(smatrix_t* self, uint32_t x);
smatrix_rmap_slot_t* smatrix_rmap_insert(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key);
void smatrix_rmap_load(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_swap(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_unswap(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_meta_load(smatrix_t* self);
*/
void* smatrix_rmap_lookup(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key);
int smatrix_rmap_resize(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_rmap_init(smatrix_t* self, smatrix_rmap_t* rmap, uint64_t size);
void smatrix_rmap_sync(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_meta_sync(smatrix_t* self);
void* smatrix_malloc(smatrix_t* self, uint64_t bytes);
uint64_t smatrix_falloc(smatrix_t* self, uint64_t bytes);
void smatrix_access(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key, uint32_t value, uint64_t ptr);
int smatrix_rmap_insert(__uint128_t* slot, uint32_t key, uint32_t value, uint64_t ptr);
/*
// ----

uint64_t smatrix_get(smatrix_t* self, uint32_t x, uint32_t y);
uint64_t smatrix_set(smatrix_t* self, uint32_t x, uint32_t y, uint64_t value);
uint64_t smatrix_incr(smatrix_t* self, uint32_t x, uint32_t y, uint64_t value);
uint64_t smatrix_decr(smatrix_t* self, uint32_t x, uint32_t y, uint64_t value);
uint64_t smatrix_rowlen(smatrix_t* self, uint32_t x);
uint64_t smatrix_getrow(smatrix_t* self, uint32_t x, uint64_t* ret, size_t ret_len);
*/

__uint128_t* smatrix_slot(void* data, uint32_t pos);
uint64_t smatrix_slot_ptr(__uint128_t slot);
uint32_t smatrix_slot_val(__uint128_t slot);
uint32_t smatrix_slot_key(__uint128_t slot);

#endif
