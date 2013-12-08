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
#include <inttypes.h>

#include "smatrix.h"
#include "smatrix_private.h"

// TODO
//  + async write
//  + free rmaps after write :)
//  + rmap row dirty flags
//  + ftruncate in larger blocks
//  + lru based GC
//  + aquire lock on file to prevent concurrent access
//  + check correct endianess on file open
//  + proper error handling / return codes for smatrix_open
//  + file free list

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

  self->lock.count = 0;
  self->lock.mutex = 0;

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

  if (self->fd) {
    close(self->fd);
  }

  free(self);
}

uint64_t smatrix_falloc(smatrix_t* self, uint64_t bytes) {
  smatrix_lock_getmutex(&self->lock);

  uint64_t old = self->fpos;
  uint64_t new = old + bytes;

  if (ftruncate(self->fd, new) == -1) {
    smatrix_error("truncate() failed");
  }

  self->fpos = new;

  smatrix_lock_release(&self->lock);
  return old;
}

void* smatrix_malloc(smatrix_t* self, uint64_t bytes) {
  __sync_add_and_fetch(&self->mem, bytes);

  void* ptr = malloc(bytes);

  if (ptr == NULL) {
    smatrix_error("malloc() failed");
    abort();
  }

  return ptr;
}

void smatrix_mfree(smatrix_t* self, uint64_t bytes) {
  __sync_sub_and_fetch(&self->mem, bytes);
}

void smatrix_ffree(smatrix_t* self, uint64_t fpos, uint64_t bytes) {
  (void) self;
  (void) fpos;
  (void) bytes;
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

  rmap = smatrix_cmap_lookup(self, &self->cmap, x, write);

  if (rmap == NULL) {
    return;
  }

  if (write) {
    smatrix_lock_decref(&rmap->lock);
    smatrix_lock_getmutex(&rmap->lock);
    mutex = 1;
  }

  if (rmap->size == 0) {
    if (!mutex) {
      smatrix_lock_decref(&rmap->lock);
      smatrix_lock_getmutex(&rmap->lock);
      mutex = 1;
    }

    if (rmap->size == 0) {
      smatrix_rmap_load(self, rmap);
    }
  }

  ref->rmap = rmap;

  if (mutex && !write) {
    smatrix_lock_dropmutex(&rmap->lock);
  }

  slot = smatrix_rmap_probe(rmap, y);

  if (slot != NULL && slot->key == y) {
    ref->slot = slot;
  } else if (write) {
    ref->slot = smatrix_rmap_insert(self, rmap, y);
  }
}

void smatrix_decref(smatrix_t* self, smatrix_ref_t* ref) {
  if (!ref->rmap) {
    return;
  }

  if (ref->write) {
    if (self->fd) {
      // FIXPAUL: this will sync the whole rmap. if only one slot changed this is a lot of overhead...
      smatrix_rmap_sync_defer(self, ref->rmap);
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
  } else {
    rmap->data = NULL;
  }

  rmap->size       = size;
  rmap->used       = 0;
  rmap->fpos       = 0;
  rmap->flags      = 0;
  rmap->lock.count = 0;
  rmap->lock.mutex = 0;
}


// you need to hold a write lock on rmap to call this function safely
smatrix_rmap_slot_t* smatrix_rmap_insert(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key) {
  smatrix_rmap_slot_t* slot;

  if (rmap->used > rmap->size / 2) {
    smatrix_rmap_resize(self, rmap);
  }

  slot = smatrix_rmap_probe(rmap, key);
  assert(slot != NULL);

  if (!slot->key || slot->key != key) {
    rmap->used++;
    slot->key   = key;
    slot->value = 0;
  }

  return slot;
}

// you need to hold a read or write lock on rmap to call this function safely
smatrix_rmap_slot_t* smatrix_rmap_probe(smatrix_rmap_t* rmap, uint32_t key) {
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
  uint64_t pos, bytes, old_size, new_size;
  smatrix_rmap_slot_t* slot;
  smatrix_rmap_t new;

  old_size = rmap->size;
  new_size = rmap->size * 2;
  bytes    = sizeof(smatrix_rmap_slot_t) * new_size;

  new.size = new_size;
  new.used = 0;
  new.data = smatrix_malloc(self, bytes);
  memset(new.data, 0, bytes);

  for (pos = 0; pos < rmap->size; pos++) {
    if (!rmap->data[pos].key && !rmap->data[pos].value)
      continue;

    slot = smatrix_rmap_insert(self, &new, rmap->data[pos].key);
    slot->value = rmap->data[pos].value;
  }

  smatrix_mfree(self, sizeof(smatrix_rmap_slot_t) * old_size);
  free(rmap->data);

  rmap->data = new.data;
  rmap->size = new.size;
  rmap->used = new.used;

  if (self->fd) {
    rmap->flags |= SMATRIX_RMAP_FLAG_RESIZED;
    smatrix_rmap_sync_defer(self, rmap);
  }
}

void smatrix_rmap_sync_defer(smatrix_t* self, smatrix_rmap_t* rmap) {
  rmap->flags |= SMATRIX_RMAP_FLAG_DIRTY;
  smatrix_rmap_sync(self, rmap);
}

void smatrix_rmap_sync(smatrix_t* self, smatrix_rmap_t* rmap) {
  uint64_t bytes;

  if ((rmap->flags & SMATRIX_RMAP_FLAG_RESIZED) > 0) {
    // FIXPAUL can't ffree without knowing the old size.. just dividing by 2 seems too hacky
    //bytes = SMATRIX_RMAP_SLOT_SIZE * rmap->size + SMATRIX_RMAP_HEAD_SIZE;
    //smatrix_ffree(self, rmap->fpos, bytes);

    rmap->fpos = 0;
  }

  if (rmap->fpos == 0) {
    bytes      = SMATRIX_RMAP_SLOT_SIZE * rmap->size + SMATRIX_RMAP_HEAD_SIZE;
    rmap->fpos = smatrix_falloc(self, bytes);

    smatrix_rmap_write_batch(self, rmap, 1);
    smatrix_cmap_write(self, rmap);
  } else {
    // FIXPAUL write only the actualy dirty slots!
    smatrix_rmap_write_batch(self, rmap, 1);
  }

  rmap->flags &= ~SMATRIX_RMAP_FLAG_DIRTY;
  rmap->flags &= ~SMATRIX_RMAP_FLAG_RESIZED;
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

  smatrix_write(self, rmap->fpos, buf, bytes);
}

void smatrix_rmap_write_slot(smatrix_t* self, smatrix_rmap_t* rmap, smatrix_rmap_slot_t* slot) {
  uint64_t rmap_pos, fpos;
  char* buf = smatrix_malloc(self, SMATRIX_RMAP_SLOT_SIZE);

  rmap_pos = slot - rmap->data;
  fpos     = rmap_pos * SMATRIX_RMAP_SLOT_SIZE;
  fpos     += rmap->fpos + SMATRIX_RMAP_HEAD_SIZE;

  memcpy(buf,     &slot->key,   4);
  memcpy(buf + 4, &slot->value, 4);

  smatrix_write(self, fpos, buf, SMATRIX_RMAP_SLOT_SIZE);
}

// caller must hold writelock on rmap
void smatrix_rmap_load(smatrix_t* self, smatrix_rmap_t* rmap) {
  uint64_t pos, read_bytes, mem_bytes, disk_bytes, rmap_size;
  unsigned char meta_buf[SMATRIX_RMAP_HEAD_SIZE] = {0}, *buf;

  if (rmap->flags & SMATRIX_RMAP_FLAG_LOADED)
    return;

  if (!rmap->size) {
    if (pread(self->fd, &meta_buf, SMATRIX_RMAP_HEAD_SIZE, rmap->fpos) != SMATRIX_RMAP_HEAD_SIZE) {
      smatrix_error("pread() failed (rmap_load). corrupt file?");
    }

    if (memcmp(&meta_buf, &SMATRIX_RMAP_MAGIC, SMATRIX_RMAP_MAGIC_SIZE)) {
      smatrix_error("file is corrupt (rmap_load)");
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
    smatrix_error("read() failed (rmap_load)");
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
  if (rmap->data) {
    smatrix_mfree(self, sizeof(smatrix_rmap_slot_t) * rmap->size);
    free(rmap->data);
  }

  smatrix_mfree(self, sizeof(smatrix_rmap_t));
  free(rmap);
}

void smatrix_fcreate(smatrix_t* self) {
  char buf[SMATRIX_META_SIZE];
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
    smatrix_error("invalid file header\n");
    abort();
  }

  if (buf[0] != 0x17 || buf[1] != 0x17) {
    smatrix_error("invalid file header\n");
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

  smatrix_lock_incref(&cmap->lock);
  slot = smatrix_cmap_probe(cmap, key);

  if (slot && slot->key == key && (slot->flags & SMATRIX_CMAP_SLOT_USED) != 0) {
    rmap = slot->rmap;
    smatrix_lock_incref(&rmap->lock);
    smatrix_lock_decref(&cmap->lock);
    return rmap;
  }

  smatrix_lock_decref(&cmap->lock);

  if (!create) {
    return NULL;
  }

  rmap = smatrix_malloc(self, sizeof(smatrix_rmap_t));
  smatrix_rmap_init(self, rmap, SMATRIX_RMAP_INITIAL_SIZE);
  rmap->key = key;

  smatrix_lock_getmutex(&cmap->lock);
  slot = smatrix_cmap_insert(self, cmap, key);

  if (slot->rmap) {
    smatrix_rmap_free(self, rmap);
    rmap = slot->rmap;

    smatrix_lock_incref(&rmap->lock);
    smatrix_lock_release(&cmap->lock);
  } else {
    slot->rmap = rmap;

    if (self->fd) {
      rmap->meta_fpos = smatrix_cmap_falloc(self, &self->cmap);
    }

    smatrix_lock_incref(&rmap->lock);
    smatrix_lock_release(&cmap->lock);

    if (self->fd) {
      smatrix_rmap_sync_defer(self, rmap);
    }
  }

  return rmap;
}

// caller must hold a read lock on cmap!
smatrix_cmap_slot_t* smatrix_cmap_probe(smatrix_cmap_t* cmap, uint32_t key) {
  unsigned pos = key;
  smatrix_cmap_slot_t* slot;

  slot = cmap->data + (key % cmap->size);

  for (;;) {
    if ((slot->flags & SMATRIX_CMAP_SLOT_USED) == 0) {
      return slot;
    }

    if (slot->key == key) {
      return slot;
    }

    pos++;
    slot = cmap->data + (pos % cmap->size);
  }

  return slot;
}

smatrix_cmap_slot_t* smatrix_cmap_insert(smatrix_t* self, smatrix_cmap_t* cmap, uint32_t key) {
  smatrix_cmap_slot_t* slot;

  if (cmap->used * 4 >= cmap->size * 3) {
    smatrix_cmap_resize(self, cmap);
  }

  slot = smatrix_cmap_probe(cmap, key);
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

  smatrix_write(self, cmap->block_fpos, buf, SMATRIX_CMAP_HEAD_SIZE);
  smatrix_write(self, meta_fpos, meta_buf, 8);
}

void smatrix_cmap_write(smatrix_t* self, smatrix_rmap_t* rmap) {
  char* buf = smatrix_malloc(self, SMATRIX_CMAP_SLOT_SIZE);

  memcpy(buf,     &rmap->key,  4);
  memcpy(buf + 4, &rmap->fpos, 8);

  smatrix_write(self, rmap->meta_fpos, buf, SMATRIX_CMAP_SLOT_SIZE);
}

void smatrix_cmap_load(smatrix_t* self, uint64_t head_fpos) {
  smatrix_rmap_t* rmap;
  unsigned char meta_buf[SMATRIX_CMAP_HEAD_SIZE], *buf;
  ssize_t bytes, pos;
  uint64_t fpos, value;

  for (fpos = head_fpos; fpos;) {
    self->cmap.block_fpos = fpos;

    if (pread(self->fd, &meta_buf, SMATRIX_CMAP_HEAD_SIZE, fpos) != SMATRIX_CMAP_HEAD_SIZE) {
      smatrix_error("pread() failed (cmap_load). corrupt file?");
    }

    fpos += SMATRIX_CMAP_HEAD_SIZE;
    bytes = *((uint64_t *) &meta_buf) * SMATRIX_CMAP_SLOT_SIZE;
    buf   = smatrix_malloc(self, bytes);

    if (pread(self->fd, buf, bytes, fpos) != bytes) {
      smatrix_error("pread() failed (cmap_load). corrupt file?");
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

void smatrix_write(smatrix_t* self, uint64_t fpos, char* data, uint64_t bytes) {
  if (pwrite(self->fd, data, bytes, fpos) != (ssize_t) bytes) {
    smatrix_error("write() failed");
  }

  free(data);
  smatrix_mfree(self, bytes);
}

// the caller of this function must have called smatrix_lock_incref before
// returns 0 for success, 1 for failure
void smatrix_lock_getmutex(smatrix_lock_t* lock) {
  assert(lock->count > 0);

  for (;;) {
    if (__sync_bool_compare_and_swap(&lock->mutex, 0, 1)) {
      break;
    }

    while (lock->mutex != 0) {
      asm("pause");
    }
  }

  while (lock->count > 0) {
    asm("pause");
  }
}

void smatrix_lock_dropmutex(smatrix_lock_t* lock) {
  assert(lock->count == 0);
  asm("lock incw (%0)" : : "c" (&lock->count));
  lock->mutex = 0;
}

void smatrix_lock_release(smatrix_lock_t* lock) {
  lock->mutex = 0;
}

inline void smatrix_lock_incref(smatrix_lock_t* lock) {
  for (;;) {
    asm("lock incw (%0)" : : "c" (&lock->count));

    if (lock->mutex == 0) {
      return;
    }

    asm("lock decw (%0)" : : "c" (&lock->count));

    while (lock->mutex != 0) {
      asm("pause");
    }
  }
}

inline void smatrix_lock_decref(smatrix_lock_t* lock) {
  asm("lock decw (%0)" : : "c" (&lock->count));
}

void smatrix_error(const char* msg) {
  printf("libsmatrix error: %s", msg);
  abort();
}
