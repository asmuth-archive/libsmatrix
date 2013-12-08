// This file is part of the "libsmatrix" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#ifndef SMATRIX_PRIVATE_H
#define SMATRIX_PRIVATE_H

void smatrix_fcreate(smatrix_t* self);
void smatrix_fload(smatrix_t* self);
void smatrix_lookup(smatrix_t* self, smatrix_ref_t* ref, uint32_t x, uint32_t y, int write);
void smatrix_decref(smatrix_t* self, smatrix_ref_t* ref);
void* smatrix_malloc(smatrix_t* self, uint64_t bytes);
void smatrix_mfree(smatrix_t* self, uint64_t bytes);
uint64_t smatrix_falloc(smatrix_t* self, uint64_t bytes);
void smatrix_ffree(smatrix_t* self, uint64_t fpos, uint64_t bytes);
void smatrix_write(smatrix_t* self, uint64_t fpos, char* data, uint64_t bytes);
void smatrix_rmap_init(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t size);
smatrix_rmap_slot_t* smatrix_rmap_probe(smatrix_rmap_t* rmap, uint32_t key);
smatrix_rmap_slot_t* smatrix_rmap_insert(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key);
void smatrix_rmap_resize(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_rmap_load(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_rmap_write_batch(smatrix_t* self, smatrix_rmap_t* rmap, int full);
void smatrix_rmap_write_slot(smatrix_t* self, smatrix_rmap_t* rmap, smatrix_rmap_slot_t* slot);
void smatrix_rmap_swap(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_rmap_free(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_rmap_sync_defer(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_rmap_sync(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_cmap_init(smatrix_t* self);
smatrix_rmap_t* smatrix_cmap_lookup(smatrix_t* self, smatrix_cmap_t* cmap, uint32_t key, int create);
smatrix_cmap_slot_t* smatrix_cmap_probe(smatrix_cmap_t* cmap, uint32_t key);
smatrix_cmap_slot_t* smatrix_cmap_insert(smatrix_t* self, smatrix_cmap_t* cmap, uint32_t key);
void smatrix_cmap_resize(smatrix_t* self, smatrix_cmap_t* cmap);
void smatrix_cmap_free(smatrix_t* self, smatrix_cmap_t* cmap);
uint64_t smatrix_cmap_falloc(smatrix_t* self, smatrix_cmap_t* cmap);
void smatrix_cmap_mkblock(smatrix_t* self, smatrix_cmap_t* cmap);
void smatrix_cmap_write(smatrix_t* self, smatrix_rmap_t* rmap);
void smatrix_cmap_load(smatrix_t* self, uint64_t head_fpos);
void smatrix_lock_getmutex(smatrix_lock_t* lock);
void smatrix_lock_dropmutex(smatrix_lock_t* lock);
void smatrix_lock_release(smatrix_lock_t* lock);
void smatrix_lock_incref(smatrix_lock_t* lock);
void smatrix_lock_decref(smatrix_lock_t* lock);
void smatrix_error(const char* msg);
void smatrix_ioqueue_add(smatrix_t* self, smatrix_rmap_t* rmap);

#endif
