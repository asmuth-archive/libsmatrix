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

#define SMATRIX_META_SIZE 512
#define SMATRIX_RMAP_FLAG_LOADED 4
#define SMATRIX_RMAP_MAGIC "\x23\x23\x23\x23\x23\x23\x23\x23"
#define SMATRIX_RMAP_MAGIC_SIZE 8
#define SMATRIX_RMAP_INITIAL_SIZE 10
#define SMATRIX_RMAP_SLOT_SIZE 8
#define SMATRIX_RMAP_HEAD_SIZE 16
#define SMATRIX_CMAP_INITIAL_SIZE 4096
#define SMATRIX_CMAP_SLOT_SIZE 12
#define SMATRIX_CMAP_HEAD_SIZE 16
#define SMATRIX_CMAP_BLOCK_SIZE 8192
#define SMATRIX_CMAP_SLOT_USED 1

typedef struct {
  volatile uint16_t    count;
  volatile uint16_t    mutex;
} smatrix_lock_t;

typedef struct {
  uint32_t             key;
  uint32_t             value;
} smatrix_rmap_slot_t;

typedef struct {
  uint64_t             fpos;
  uint64_t             meta_fpos;
  uint32_t             size;
  uint32_t             used;
  uint32_t             key;
  uint32_t             flags;
  smatrix_rmap_slot_t* data;
  smatrix_lock_t       lock;
} smatrix_rmap_t;

typedef struct {
  uint32_t             flags;
  uint32_t             key;
  smatrix_rmap_t*      rmap;
} smatrix_cmap_slot_t;

typedef struct {
  uint64_t             size;
  uint64_t             used;
  uint64_t             block_fpos;
  uint64_t             block_used;
  uint64_t             block_size;
  smatrix_cmap_slot_t* data;
  smatrix_lock_t       lock;
} smatrix_cmap_t;

typedef struct {
  int                  write;
  smatrix_rmap_t*      rmap;
  smatrix_rmap_slot_t* slot;
} smatrix_ref_t;

typedef struct {
  int                  fd;
  uint64_t             fpos;
  uint64_t             mem;
  smatrix_cmap_t       cmap;
  pthread_mutex_t      lock;
} smatrix_t;


// --- public api
smatrix_t* smatrix_open(const char* fname);
uint32_t smatrix_get(smatrix_t* self, uint32_t x, uint32_t y);
uint32_t smatrix_set(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);
uint32_t smatrix_incr(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);
uint32_t smatrix_decr(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);
uint32_t smatrix_rowlen(smatrix_t* self, uint32_t x);
uint32_t smatrix_getrow(smatrix_t* self, uint32_t x, uint32_t* ret, size_t ret_len);
void smatrix_close(smatrix_t* self);
// ---

void smatrix_fcreate(smatrix_t* self);
void smatrix_fload(smatrix_t* self);

void smatrix_lookup(smatrix_t* self, smatrix_ref_t* ref, uint32_t x, uint32_t y, int write);
void smatrix_decref(smatrix_t* self, smatrix_ref_t* ref);

void* smatrix_malloc(smatrix_t* self, uint64_t bytes);
void smatrix_mfree(smatrix_t* self, uint64_t bytes);
uint64_t smatrix_falloc(smatrix_t* self, uint64_t bytes);
void smatrix_ffree(smatrix_t* self, uint64_t fpos, uint64_t bytes);
void smatrix_write(smatrix_t* self, smatrix_rmap_t* rmap, uint64_t fpos, char* data, uint64_t bytes);

void smatrix_rmap_init(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t size);
smatrix_rmap_slot_t* smatrix_rmap_probe(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key);
smatrix_rmap_slot_t* smatrix_rmap_insert(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key);
void smatrix_rmap_resize(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_rmap_load(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_rmap_write_batch(smatrix_t* self, smatrix_rmap_t* rmap, int full);
void smatrix_rmap_write_slot(smatrix_t* self, smatrix_rmap_t* rmap, smatrix_rmap_slot_t* slot);
void smatrix_rmap_swap(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_rmap_free(smatrix_t* self, smatrix_rmap_t* rmap);

void smatrix_cmap_init(smatrix_t* self);
smatrix_rmap_t* smatrix_cmap_lookup(smatrix_t* self, smatrix_cmap_t* cmap, uint32_t key, int create);
smatrix_cmap_slot_t* smatrix_cmap_probe(smatrix_t* self, smatrix_cmap_t* cmap, uint32_t key);
smatrix_cmap_slot_t* smatrix_cmap_insert(smatrix_t* self, smatrix_cmap_t* cmap, uint32_t key);
void smatrix_cmap_resize(smatrix_t* self, smatrix_cmap_t* cmap);
void smatrix_cmap_free(smatrix_t* self, smatrix_cmap_t* cmap);
uint64_t smatrix_cmap_falloc(smatrix_t* self, smatrix_cmap_t* cmap);
void smatrix_cmap_mkblock(smatrix_t* self, smatrix_cmap_t* cmap);
void smatrix_cmap_write(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_cmap_load(smatrix_t* self, uint64_t head_fpos);

int smatrix_lock_trymutex(smatrix_lock_t* lock);
void smatrix_lock_dropmutex(smatrix_lock_t* lock);
void smatrix_lock_release(smatrix_lock_t* lock);
void smatrix_lock_incref(smatrix_lock_t* lock);
void smatrix_lock_decref(smatrix_lock_t* lock);

// ----


#endif
