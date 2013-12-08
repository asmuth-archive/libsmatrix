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
#define SMATRIX_RMAP_FLAG_DIRTY 8
#define SMATRIX_RMAP_FLAG_RESIZED 16
#define SMATRIX_RMAP_MAGIC "\x23\x23\x23\x23\x23\x23\x23\x23"
#define SMATRIX_RMAP_MAGIC_SIZE 8
#define SMATRIX_RMAP_INITIAL_SIZE 16
#define SMATRIX_RMAP_SLOT_SIZE 8
#define SMATRIX_RMAP_HEAD_SIZE 16
#define SMATRIX_CMAP_INITIAL_SIZE 65536
#define SMATRIX_CMAP_SLOT_SIZE 12
#define SMATRIX_CMAP_HEAD_SIZE 16
#define SMATRIX_CMAP_BLOCK_SIZE 4194304
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

typedef struct smatrix_ref_s smatrix_ref_t;

struct smatrix_ref_s {
  int                  write;
  smatrix_rmap_t*      rmap;
  smatrix_rmap_slot_t* slot;
  smatrix_ref_t*       next;
};

typedef struct {
  int                  fd;
  uint64_t             fpos;
  uint64_t             mem;
  smatrix_ref_t*       ioqueue;
  smatrix_cmap_t       cmap;
  smatrix_lock_t       lock;
} smatrix_t;

smatrix_t* smatrix_open(const char* fname);
uint32_t smatrix_get(smatrix_t* self, uint32_t x, uint32_t y);
uint32_t smatrix_set(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);
uint32_t smatrix_incr(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);
uint32_t smatrix_decr(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);
uint32_t smatrix_rowlen(smatrix_t* self, uint32_t x);
uint32_t smatrix_getrow(smatrix_t* self, uint32_t x, uint32_t* ret, size_t ret_len);
void smatrix_close(smatrix_t* self);

#endif
