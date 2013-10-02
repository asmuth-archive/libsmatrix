// This file is part of the "libsmatrix" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "smatrix.h"

// TODO
//  + ftruncate in larger blocks
//  + aquire lock on file to prevent concurrent access
//  + check correct endianess on file open
//  + convert smatrix->lock to spinlock, remove pthread dependency
//  + proper error handling / return codes for smatrix_open
//  + file free list
//  + re-implement (a smart) gc

/*

  libsmatrix file format (augmented BNF):
  ---------------------------------------

    FILE              ::= FILE_HEADER         ; header size is 512 bytes
                          FILE_BODY

    FILE_HEADER       ::= <8 Bytes 0x17>      ; uint64_t, magic number
                          CMAP_HEAD_FPOS      ; uint64_t
                          <496 Bytes 0x0>     ; padding to 512 bytes

    FILE_BODY         ::= *( CMAP_BLOCK | RMAP_BLOCK )

    CMAP_BLOCK        ::= CMAP_BLOCK_SIZE     ; uint64_t
                          CMAP_BLOCK_NEXT     ; uint64_t, file offset
                          *( CMAP_ENTRY )     ; 12 bytes each

    CMAP_ENTRY        ::= CMAP_ENTRY_KEY      ; uint32_t
                          CMAP_ENTRY_VALUE    ; uint64_t

    CMAP_ENTRY_KEY    ::= <uint32_t>          ; key / first dimension
    CMAP_ENTRY_VALUE  ::= <uint64_t>          ; file offset of the RMAP_BLOCK
    CMAP_HEAD_FPOS    ::= <uint64_t>          ; file offset of the first CMAP_BLOCK
    CMAP_BLOCK_SIZE   ::= <uint64_t>          ; number of entries in this block
    CMAP_BLOCK_NEXT   ::= <uint64_t>          ; file offset of the next block or 0

    RMAP_BLOCK        ::= <8 Bytes 0x23>      ; uint64_t, magic number
                          RMAP_BLOCK_SIZE     ; uint64_t
                          *( RMAP_SLOT )      ; 8 bytes each

    RMAP_SLOT         ::= RMAP_ENTRY          ; used hashmap slot
                          | RMAP_SLOT_UNUSED  ; unused hashmap slot

    RMAP_ENTRY        ::= RMAP_ENTRY_KEY      ; uint32_t
                          RMAP_ENTRY_VALUE    ; uint32_t

    RMAP_SLOT_UNUSED  ::= <8 Bytes 0x0>       ; empty slot
    RMAP_ENTRY_KEY    ::= <uint32_t>          ; key / second dimension
    RMAP_ENTRY_VALUE  ::= <uint32_t>          ; value
    RMAP_BLOCK_SIZE   ::= <uint64_t>          ; number of slots in this block

*/

smatrix_t* smatrix_open(const char* fname) {
  smatrix_t* self = calloc(1, sizeof(smatrix_t));

  if (self == NULL)
    return NULL;

  pthread_mutex_init(&self->lock, NULL);

  if (!fname) {
    smatrix_cmap_init(self);
    return self;
  }

  self->fd  = open(fname, O_RDWR | O_CREAT, 00600);

  if (self->fd == -1) {
    perror("cannot open file");
    free(self);
    return NULL;
  }

  self->fpos = lseek(self->fd, 0, SEEK_END);

  if (self->fpos == 0) {
    smatrix_fcreate(self);
  } else {
    smatrix_fload(self);
  }

  return self;
}

void smatrix_close(smatrix_t* self) {
  uint64_t pos;

  for (pos = 0; pos < self->cmap.size; pos++) {
    if (self->cmap.data[pos].flags & SMATRIX_CMAP_SLOT_USED) {
      smatrix_rmap_free(self, self->cmap.data[pos].rmap);
    }
  }

  smatrix_cmap_free(self, &self->cmap);
  pthread_mutex_destroy(&self->lock);
  close(self->fd);
  free(self);
}

uint64_t smatrix_falloc(smatrix_t* self, uint64_t bytes) {
  pthread_mutex_lock(&self->lock);

  uint64_t old = self->fpos;
  uint64_t new = old + bytes;

  if (ftruncate(self->fd, new) == -1) {
    perror("FATAL ERROR: CANNOT TRUNCATE FILE"); // FIXPAUL
    abort();
  }

  self->fpos = new;

  pthread_mutex_unlock(&self->lock);
  return old;
}

void* smatrix_malloc(smatrix_t* self, uint64_t bytes) {
  __sync_add_and_fetch(&self->mem, bytes);

  void* ptr = malloc(bytes);

  if (ptr == NULL) {
    printf("malloc failed!\n"); // FIXPAUL
    abort();
  }

  return ptr;
}

void smatrix_mfree(smatrix_t* self, uint64_t bytes) {
  __sync_sub_and_fetch(&self->mem, bytes);
}

void smatrix_ffree(smatrix_t* self, uint64_t fpos, uint64_t bytes) {
  //printf("FREED %llu bytes @ %llu\n", bytes, fpos);
}

uint32_t smatrix_get(smatrix_t* self, uint32_t x, uint32_t y) {
  smatrix_ref_t ref;
  uint32_t retval = 0;

  smatrix_lookup(self, &ref, x, y, 0);

  if (ref.slot)
    retval = ref.slot->value;

  smatrix_decref(self, &ref);
  return retval;
}

// returns a whole row as an array of uint32_t's, odd slots contain indexes, even slots contain
// values. example: [index, value, index, value...]
uint32_t smatrix_getrow(smatrix_t* self, uint32_t x, uint32_t* ret, size_t ret_len) {
  smatrix_ref_t ref;
  uint32_t pos, num = 0;

  smatrix_lookup(self, &ref, x, 0, 0);

  if (ref.rmap) {
    for (pos = 0; pos < ref.rmap->size; pos++) {
      if (!ref.rmap->data[pos].key && !ref.rmap->data[pos].value)
        continue;

      ret[num * 2]     = ref.rmap->data[pos].key;
      ret[num * 2 + 1] = ref.rmap->data[pos].value;

      if ((++num * 2 * sizeof(uint32_t)) >= ret_len)
        break;
    }
  }

  smatrix_decref(self, &ref);
  return num;
}

uint32_t smatrix_rowlen(smatrix_t* self, uint32_t x) {
  smatrix_ref_t ref;
  uint32_t len = 0;

  smatrix_lookup(self, &ref, x, 0, 0);

  if (ref.rmap)
    len = ref.rmap->used;

  smatrix_decref(self, &ref);
  return len;
}

uint32_t smatrix_set(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value) {
  smatrix_ref_t ref;
  uint32_t retval;

  smatrix_lookup(self, &ref, x, y, 1);
  retval = (ref.slot->value = value);
  smatrix_decref(self, &ref);

  return retval;
}

uint32_t smatrix_incr(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value) {
  smatrix_ref_t ref;
  uint32_t retval;

  smatrix_lookup(self, &ref, x, y, 1);
  retval = (ref.slot->value += value);
  smatrix_decref(self, &ref);

  return retval;
}

uint32_t smatrix_decr(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value) {
  smatrix_ref_t ref;
  uint32_t retval;

  smatrix_lookup(self, &ref, x, y, 1);
  retval = (ref.slot->value -= value);
  smatrix_decref(self, &ref);

  return retval;
}

void smatrix_lookup(smatrix_t* self, smatrix_ref_t* ref, uint32_t x, uint32_t y, int write) {
  int mutex = 0;
  smatrix_rmap_t* rmap;
  smatrix_rmap_slot_t* slot;

  ref->rmap = NULL;
  ref->slot = NULL;
  ref->write = write;

  for (;;) {
    rmap = smatrix_cmap_lookup(self, &self->cmap, x, write);

    if (rmap == NULL) {
      return;
    }

    if (rmap->size == 0) {
      if (smatrix_lock_trymutex(&rmap->lock)) {
        smatrix_lock_decref(&rmap->lock);
        continue;
      }

      mutex = 1;
      smatrix_rmap_load(self, rmap);
    }

    if (write && !mutex) {
      if (smatrix_lock_trymutex(&rmap->lock)) {
        smatrix_lock_decref(&rmap->lock);
        continue;
      }
    }

    ref->rmap = rmap;
    break;
  }

  if (mutex && !write) {
    smatrix_lock_dropmutex(&rmap->lock);
  }

  slot = smatrix_rmap_probe(self, rmap, y);

  if (slot != NULL && slot->key == y) {
    ref->slot = slot;
  } else if (write) {
    ref->slot = smatrix_rmap_insert(self, rmap, y);
  }
}

void smatrix_decref(smatrix_t* self, smatrix_ref_t* ref) {
  if (!ref->rmap) {
    return;
  } else if (ref->write) {
    if (self->fd) {
      smatrix_rmap_write_slot(self, ref->rmap, ref->slot);
    }

    smatrix_lock_release(&ref->rmap->lock);
  } else {
    smatrix_lock_decref(&ref->rmap->lock);
  }
}

void smatrix_rmap_init(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t size) {
  if (size > 0) {
    size_t bytes = sizeof(smatrix_rmap_slot_t) * size;

    rmap->data = smatrix_malloc(self, bytes);
    memset(rmap->data, 0, bytes);
  }

  rmap->size = size;
  rmap->used = 0;
  rmap->flags = 0;
  rmap->lock.count = 0;
  rmap->lock.mutex = 0;
}


// you need to hold a write lock on rmap to call this function safely
smatrix_rmap_slot_t* smatrix_rmap_insert(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key) {
  smatrix_rmap_slot_t* slot;

  if (rmap->used > rmap->size / 2) {
    smatrix_rmap_resize(self, rmap);
  }

  slot = smatrix_rmap_probe(self, rmap, key);
  assert(slot != NULL);

  if (!slot->key || slot->key != key) {
    rmap->used++;
    slot->key   = key;
    slot->value = 0;
  }

  return slot;
}

// you need to hold a read or write lock on rmap to call this function safely
smatrix_rmap_slot_t* smatrix_rmap_probe(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key) {
  uint64_t n, pos;

  pos = key % rmap->size;

  // linear probing
  for (n = 0; n < rmap->size; n++) {
    if (rmap->data[pos].key == key)
      break;

    if (!rmap->data[pos].key && !rmap->data[pos].value)
      break;

    pos = (pos + 1) % rmap->size;
  }

  return &rmap->data[pos];
}

// you need to hold a write lock on rmap in order to call this function safely
void smatrix_rmap_resize(smatrix_t* self, smatrix_rmap_t* rmap) {
  smatrix_rmap_slot_t* slot;
  smatrix_rmap_t new;
  void* old_data;

  uint64_t pos, old_fpos, new_size = rmap->size * 2;

  uint64_t old_bytes_disk = SMATRIX_RMAP_SLOT_SIZE * rmap->size + SMATRIX_RMAP_HEAD_SIZE;
  uint64_t old_bytes_mem = sizeof(smatrix_rmap_slot_t) * rmap->size;
  uint64_t new_bytes_disk = SMATRIX_RMAP_SLOT_SIZE * new_size + SMATRIX_RMAP_HEAD_SIZE;
  uint64_t new_bytes_mem = sizeof(smatrix_rmap_slot_t) * new_size;

  new.used = 0;
  new.size = new_size;
  new.data = smatrix_malloc(self, new_bytes_mem);
  memset(new.data, 0, new_bytes_mem);

  for (pos = 0; pos < rmap->size; pos++) {
    if (!rmap->data[pos].key && !rmap->data[pos].value)
      continue;

    slot = smatrix_rmap_insert(self, &new, rmap->data[pos].key);
    slot->value = rmap->data[pos].value;
  }

  old_data   = rmap->data;
  old_fpos   = rmap->fpos;
  rmap->data = new.data;
  rmap->size = new.size;
  rmap->used = new.used;

  if (self->fd) {
    rmap->fpos = smatrix_falloc(self, new_bytes_disk);
    smatrix_ffree(self, old_fpos, old_bytes_disk);
    smatrix_rmap_write_batch(self, rmap, 1);
    smatrix_cmap_write(self, rmap);
  }

  smatrix_mfree(self, old_bytes_mem);
  free(old_data);
}

// the caller of this must hold a read lock on rmap
void smatrix_rmap_write_batch(smatrix_t* self, smatrix_rmap_t* rmap, int full) {
  uint64_t pos = 0, bytes, buf_pos, rmap_size = rmap->size;
  char *buf;

  if (full) {
    bytes  = rmap->size * SMATRIX_RMAP_SLOT_SIZE;
    bytes += SMATRIX_RMAP_HEAD_SIZE;
  } else {
    bytes = SMATRIX_RMAP_HEAD_SIZE;
  }

  buf = smatrix_malloc(self, bytes);

  memset(buf,     0,           bytes);
  memset(buf,     0x23,        8);
  memcpy(buf + 8, &rmap_size,  8);

  if (full) {
    buf_pos = SMATRIX_RMAP_HEAD_SIZE;

    for (pos = 0; pos < rmap->size; pos++) {
      memcpy(buf + buf_pos,     &rmap->data[pos].key,   4);
      memcpy(buf + buf_pos + 4, &rmap->data[pos].value, 4);
      buf_pos += SMATRIX_RMAP_SLOT_SIZE;
    }
  }

  smatrix_write(self, rmap, rmap->fpos, buf, bytes);
}

void smatrix_rmap_write_slot(smatrix_t* self, smatrix_rmap_t* rmap, smatrix_rmap_slot_t* slot) {
  uint64_t rmap_pos, fpos;
  char* buf = smatrix_malloc(self, SMATRIX_RMAP_SLOT_SIZE);

  rmap_pos = slot - rmap->data;
  fpos     = rmap_pos * SMATRIX_RMAP_SLOT_SIZE;
  fpos     += rmap->fpos + SMATRIX_RMAP_HEAD_SIZE;

  memcpy(buf,     &slot->key,   4);
  memcpy(buf + 4, &slot->value, 4);

  smatrix_write(self, rmap, fpos, buf, SMATRIX_RMAP_SLOT_SIZE);
}

// caller must hold writelock on rmap
void smatrix_rmap_load(smatrix_t* self, smatrix_rmap_t* rmap) {
  uint64_t pos, read_bytes, mem_bytes, disk_bytes, rmap_size;
  unsigned char meta_buf[SMATRIX_RMAP_HEAD_SIZE] = {0}, *buf;

  if (rmap->flags & SMATRIX_RMAP_FLAG_LOADED)
    return;

  if (!rmap->size) {
    if (pread(self->fd, &meta_buf, SMATRIX_RMAP_HEAD_SIZE, rmap->fpos) != SMATRIX_RMAP_HEAD_SIZE) {
      printf("CANNOT LOAD RMATRIX -- pread @ %lu\n", rmap->fpos); // FIXPAUL
      abort();
    }

    if (memcmp(&meta_buf, &SMATRIX_RMAP_MAGIC, SMATRIX_RMAP_MAGIC_SIZE)) {
      printf("FILE IS CORRUPT\n"); // FIXPAUL
      abort();
    }

    rmap_size = *((uint64_t *) &meta_buf[8]);
    rmap->size = rmap_size;
    assert(rmap->size > 0);
  }

  mem_bytes  = rmap->size * sizeof(smatrix_rmap_slot_t);
  disk_bytes = rmap->size * SMATRIX_RMAP_SLOT_SIZE;
  rmap->used = 0;
  rmap->data = smatrix_malloc(self, mem_bytes);
  buf        = smatrix_malloc(self, disk_bytes);

  memset(rmap->data, 0, mem_bytes);
  read_bytes = pread(self->fd, buf, disk_bytes, rmap->fpos + SMATRIX_RMAP_HEAD_SIZE);

  if (read_bytes != disk_bytes) {
    printf("CANNOT LOAD RMATRIX -- read wrong number of bytes: %lu vs. %lu @ %lu\n", read_bytes, disk_bytes, rmap->fpos); // FIXPAUL
    abort();
  }

  for (pos = 0; pos < rmap->size; pos++) {
    memcpy(&rmap->data[pos].value, buf + pos * SMATRIX_RMAP_SLOT_SIZE + 4, 4);

    if (rmap->data[pos].value) {
      memcpy(&rmap->data[pos].key, buf + pos * SMATRIX_RMAP_SLOT_SIZE, 4);
      rmap->used++;
    }
  }

  rmap->flags = SMATRIX_RMAP_FLAG_LOADED;
  smatrix_mfree(self, disk_bytes);
  free(buf);
}

// caller must hold a write lock on rmap
void smatrix_rmap_swap(smatrix_t* self, smatrix_rmap_t* rmap) {
  rmap->flags &= ~SMATRIX_RMAP_FLAG_LOADED;
  smatrix_mfree(self, sizeof(smatrix_rmap_slot_t) * rmap->size);
  free(rmap->data);
}

void smatrix_rmap_free(smatrix_t* self, smatrix_rmap_t* rmap) {
  smatrix_mfree(self, sizeof(smatrix_rmap_slot_t) * rmap->size);
  free(rmap->data);
  smatrix_mfree(self, sizeof(smatrix_rmap_t));
  free(rmap);
}

void smatrix_fcreate(smatrix_t* self) {
  char buf[SMATRIX_META_SIZE];

  printf("NEW FILE!\n");
  smatrix_falloc(self, SMATRIX_META_SIZE);

  memset(&buf, 0, SMATRIX_META_SIZE);
  memset(&buf, 0x17, 8);
  pwrite(self->fd, &buf, SMATRIX_META_SIZE, 0);

  smatrix_cmap_init(self);
  smatrix_cmap_mkblock(self, &self->cmap);
}

void smatrix_fload(smatrix_t* self) {
  char buf[SMATRIX_META_SIZE];
  uint64_t read, cmap_head_fpos;

  read = pread(self->fd, &buf, SMATRIX_META_SIZE, 0);

  if (read != SMATRIX_META_SIZE) {
    printf("CANNOT READ FILE HEADER\n"); //FIXPAUL
    abort();
  }

  if (buf[0] != 0x17 || buf[1] != 0x17) {
    printf("INVALID FILE HEADER\n"); //FIXPAUL
    abort();
  }

  memcpy(&cmap_head_fpos, &buf[8],  8);

  smatrix_cmap_init(self);
  smatrix_cmap_load(self, cmap_head_fpos);
}

void smatrix_cmap_init(smatrix_t* self) {
  uint64_t bytes;

  self->cmap.size = SMATRIX_CMAP_INITIAL_SIZE;
  self->cmap.used = 0;
  self->cmap.lock.count = 0;
  self->cmap.lock.mutex = 0;
  self->cmap.block_fpos = 0;
  self->cmap.block_used = 0;
  self->cmap.block_size = 0;

  bytes = sizeof(smatrix_cmap_slot_t) * self->cmap.size;
  self->cmap.data = smatrix_malloc(self, bytes);
  memset(self->cmap.data, 0, bytes);
}

void smatrix_cmap_free(smatrix_t* self, smatrix_cmap_t* cmap) {
  uint64_t bytes = sizeof(smatrix_cmap_slot_t) * cmap->size;
  smatrix_mfree(self, bytes);
  free(cmap->data);
}

// caller must hold no locks on cmap!
smatrix_rmap_t* smatrix_cmap_lookup(smatrix_t* self, smatrix_cmap_t* cmap, uint32_t key, int create) {
  smatrix_cmap_slot_t* slot;
  smatrix_rmap_t* rmap;

  for (;;) {
    smatrix_lock_incref(&cmap->lock);
    slot = smatrix_cmap_probe(self, cmap, key);

    if (slot && slot->key == key && (slot->flags & SMATRIX_CMAP_SLOT_USED) != 0) {
      rmap = slot->rmap;
      smatrix_lock_incref(&rmap->lock);
      smatrix_lock_decref(&cmap->lock);
      return rmap;
    } else {
      if (!create) {
        smatrix_lock_decref(&cmap->lock);
        return NULL;
      }

      if (smatrix_lock_trymutex(&cmap->lock)) {
        smatrix_lock_decref(&cmap->lock);
        // FIXPAUL pause?
        continue;
      }

      slot = smatrix_cmap_insert(self, cmap, key);

      if (slot->rmap) {
        rmap = slot->rmap;
      } else {
        // FIXPAUL move to rmap_create method
        rmap = smatrix_malloc(self, sizeof(smatrix_rmap_t));
        smatrix_rmap_init(self, rmap, SMATRIX_RMAP_INITIAL_SIZE);
        rmap->key = key;

        if (self->fd) {
          rmap->meta_fpos = smatrix_cmap_falloc(self, &self->cmap);
          rmap->fpos = smatrix_falloc(self, rmap->size * SMATRIX_RMAP_SLOT_SIZE + SMATRIX_RMAP_HEAD_SIZE);
          smatrix_cmap_write(self, rmap);
          smatrix_rmap_write_batch(self, rmap, 0);
        }

        slot->rmap = rmap;
      }

      smatrix_lock_incref(&rmap->lock);
      smatrix_lock_release(&cmap->lock);
      return rmap;
    }
  }
}

// caller must hold a read lock on cmap!
smatrix_cmap_slot_t* smatrix_cmap_probe(smatrix_t* self, smatrix_cmap_t* cmap, uint32_t key) {
  uint64_t n, pos;

  pos = key % cmap->size;

  // linear probing
  for (n = 0; n < cmap->size; n++) {
    if ((cmap->data[pos].flags & SMATRIX_CMAP_SLOT_USED) == 0)
      break;

    if (cmap->data[pos].key == key)
      break;

    pos = (pos + 1) % cmap->size;
  }

  return &cmap->data[pos];
}

smatrix_cmap_slot_t* smatrix_cmap_insert(smatrix_t* self, smatrix_cmap_t* cmap, uint32_t key) {
  smatrix_cmap_slot_t* slot;

  if (cmap->used > cmap->size / 2) {
    smatrix_cmap_resize(self, cmap);
  }

  slot = smatrix_cmap_probe(self, cmap, key);
  assert(slot != NULL);

  if ((slot->flags & SMATRIX_CMAP_SLOT_USED) == 0 || slot->key != key) {
    cmap->used++;
    slot->key   = key;
    slot->flags = SMATRIX_CMAP_SLOT_USED;
    slot->rmap  = NULL;
  }

  return slot;
}

void smatrix_cmap_resize(smatrix_t* self, smatrix_cmap_t* cmap) {
  uint64_t new_bytes, pos;
  smatrix_cmap_slot_t *slot;
  smatrix_cmap_t new;

  new.used  = 0;
  new.size  = cmap->size * 2;
  new_bytes = sizeof(smatrix_cmap_slot_t) * new.size;
  new.data  = smatrix_malloc(self, new_bytes);

  smatrix_mfree(self, sizeof(smatrix_cmap_slot_t) * cmap->size);
  memset(new.data, 0, new_bytes);

  for (pos = 0; pos < cmap->size; pos++) {
    if ((cmap->data[pos].flags & SMATRIX_CMAP_SLOT_USED) == 0)
      continue;

    slot = smatrix_cmap_insert(self, &new, cmap->data[pos].key);
    slot->rmap = cmap->data[pos].rmap;
  }

  free(cmap->data);

  cmap->data = new.data;
  cmap->size = new.size;
  cmap->used = new.used;
}

// caller must hold a write lock on cmap
uint64_t smatrix_cmap_falloc(smatrix_t* self, smatrix_cmap_t* cmap) {
  uint64_t fpos;

  if (cmap->block_used >= cmap->block_size) {
    smatrix_cmap_mkblock(self, cmap);
  }

  fpos =  cmap->block_fpos + SMATRIX_CMAP_HEAD_SIZE;
  fpos += cmap->block_used * SMATRIX_CMAP_SLOT_SIZE;

  cmap->block_used++;

  return fpos;
}

void smatrix_cmap_mkblock(smatrix_t* self, smatrix_cmap_t* cmap) {
  uint64_t bytes, meta_fpos;
  char* buf = smatrix_malloc(self, SMATRIX_CMAP_HEAD_SIZE);
  char* meta_buf = smatrix_malloc(self, 8);

  meta_fpos = cmap->block_fpos + 8;

  bytes = SMATRIX_CMAP_BLOCK_SIZE * SMATRIX_CMAP_SLOT_SIZE;
  bytes += SMATRIX_CMAP_HEAD_SIZE;

  cmap->block_fpos = smatrix_falloc(self, bytes);
  cmap->block_used = 0;
  cmap->block_size = SMATRIX_CMAP_BLOCK_SIZE;

  memcpy(meta_buf, &cmap->block_fpos, 8);
  memcpy(buf,      &cmap->block_size, 8);
  memset(buf + 8,  0,                 8);

  smatrix_write(self, NULL, cmap->block_fpos, buf, SMATRIX_CMAP_HEAD_SIZE);
  smatrix_write(self, NULL, meta_fpos, meta_buf, 8);
}

void smatrix_cmap_write(smatrix_t* self, smatrix_rmap_t* rmap) {
  char* buf = smatrix_malloc(self, SMATRIX_CMAP_SLOT_SIZE);

  memcpy(buf,     &rmap->key,  4);
  memcpy(buf + 4, &rmap->fpos, 8);

  smatrix_write(self, NULL, rmap->meta_fpos, buf, SMATRIX_CMAP_SLOT_SIZE);
}

void smatrix_cmap_load(smatrix_t* self, uint64_t head_fpos) {
  smatrix_rmap_t* rmap;
  unsigned char meta_buf[SMATRIX_CMAP_HEAD_SIZE], *buf;
  ssize_t bytes, pos;
  uint64_t fpos, value;

  for (fpos = head_fpos; fpos;) {
    self->cmap.block_fpos = fpos;

    if (pread(self->fd, &meta_buf, SMATRIX_CMAP_HEAD_SIZE, fpos) != SMATRIX_CMAP_HEAD_SIZE) {
      printf("CANNOT LOAD CMAP -- pread @ %lu\n", fpos); // FIXPAUL
      abort();
    }

    fpos += SMATRIX_CMAP_HEAD_SIZE;
    bytes = *((uint64_t *) &meta_buf) * SMATRIX_CMAP_SLOT_SIZE;
    buf   = smatrix_malloc(self, bytes);

    if (pread(self->fd, buf, bytes, fpos) != bytes) {
      printf("CANNOT LOAD CMAP -- pread @ %lu\n", fpos); // FIXPAUL
      abort();
    }

    for (pos = 0; pos < bytes; pos += SMATRIX_CMAP_SLOT_SIZE) {
      value = *((uint64_t *) &buf[pos + 4]);

      if (!value)
        break;

      rmap = smatrix_malloc(self, sizeof(smatrix_rmap_t));
      smatrix_rmap_init(self, rmap, 0);
      rmap->key = *((uint32_t *) &buf[pos]);
      rmap->meta_fpos = fpos + pos;
      rmap->fpos = value;

      smatrix_cmap_insert(self, &self->cmap, rmap->key)->rmap = rmap;
    }

    smatrix_mfree(self, bytes);
    free(buf);
    fpos = *((uint64_t *) &meta_buf[8]);
  }
}


void smatrix_write(smatrix_t* self, smatrix_rmap_t* rmap, uint64_t fpos, char* data, uint64_t bytes) {
  //printf("write %lu bytes @ %lu\n", bytes, fpos);

  if (rmap) {
    // FIXPAUL: increment pending_writes count
  }

  // FIXPAUL queue and return here

  if (pwrite(self->fd, data, bytes, fpos) != bytes) {
    printf("error while writing %lu bytes @ %lu\n", bytes, fpos);
    abort(); // FIXPAUL
  }

  free(data);
  smatrix_mfree(self, bytes);

  if (rmap) {
    // FIXPAUL: decrement pending_writes count
  }
}

// the caller of this function must have called smatrix_lock_incref before
// returns 0 for success, 1 for failure
int smatrix_lock_trymutex(smatrix_lock_t* lock) {
  assert(lock->count > 0);

  if (!__sync_bool_compare_and_swap(&lock->mutex, 0, 1))
    return 1;

  // FIXPAUL use atomic builtin. memory barrier neccessary at all?
  __sync_sub_and_fetch(&lock->count, 1);

  while (lock->count > 0) // FIXPAUL: volatile neccessary?
    __sync_synchronize(); // FIXPAUL cpu burn + write barrier neccessary?

  return 0;
}

void smatrix_lock_dropmutex(smatrix_lock_t* lock) {
  assert(lock->count == 0);

  // FIXPAUL use atomic builtin. memory barrier neccessary at all?
  __sync_add_and_fetch(&lock->count, 1);

  __sync_synchronize();
  lock->mutex = 0;
}

void smatrix_lock_release(smatrix_lock_t* lock) {
  __sync_synchronize();
  lock->mutex = 0;
}

void smatrix_lock_incref(smatrix_lock_t* lock) {
  for (;;) {
    // FIXPAUL handle overflow!
    // FIXPAUL use atomic builtin with correct memory model (full barrier neccessary?)
    __sync_add_and_fetch(&lock->count, 1);

    if (lock->mutex) {
      __sync_sub_and_fetch(&lock->count, 1);

      while (lock->mutex != 0) // FIXPAUL bolatile neccessary?
        __sync_synchronize(); // FIXPAUL issue PAUSE instruction
    } else {
      break;
    }
  }
}

void smatrix_lock_decref(smatrix_lock_t* lock) {
  // FIXPAUL use atomic builtin. memory barrier neccessary at all?
  __sync_sub_and_fetch(&lock->count, 1);
}
