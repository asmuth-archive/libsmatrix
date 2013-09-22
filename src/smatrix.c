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
//  + implement smatrix_get
//  + aquire lock on file to prevent concurrent access
//  + falloc lock
//  + constant-ify all the magic numbers
//  + convert endianess when loading/saving to disk
//  + proper error handling / return codes for smatrix_open
//  + resize headers a sane sizes
//  + smarter smatrix_gc
//  + file free list

smatrix_t* smatrix_open(const char* fname) {
  smatrix_t* self = malloc(sizeof(smatrix_t));

  if (self == NULL)
    return NULL;

  self->mem = 0;
  self->fd  = open(fname, O_RDWR | O_CREAT, 00600);

  if (self->fd == -1) {
    perror("cannot open file");
    free(self->rmap.data);
    free(self);
    return NULL;
  }

  self->fpos = lseek(self->fd, 0, SEEK_END);

  if (self->fpos == 0) {
    printf("NEW FILE!\n");
    smatrix_falloc(self, SMATRIX_META_SIZE);
    smatrix_rmap_init(self, &self->rmap, SMATRIX_RMAP_INITIAL_SIZE);
    self->rmap.data = smatrix_malloc(self, sizeof(smatrix_rmap_slot_t) * SMATRIX_RMAP_INITIAL_SIZE);

    if (!self->rmap.data) {
      printf("can't load first level index. mem limit too small?\n");
      exit(1); // fixpaul proper error handling
    }

    memset(self->rmap.data, 0, sizeof(smatrix_rmap_slot_t) * SMATRIX_RMAP_INITIAL_SIZE);
    smatrix_rmap_sync(self, &self->rmap);
    smatrix_meta_sync(self);
  } else {
    printf("LOAD FILE!\n");
    smatrix_meta_load(self);
    smatrix_rmap_load(self, &self->rmap);
    smatrix_unswap(self, &self->rmap);

    if ((self->rmap.flags & SMATRIX_RMAP_FLAG_SWAPPED) != 0) {
      printf("can't load first level index. mem limit too small?\n");
      exit(1); // fixpaul proper error handling
    }

    // FIXPAUL: put this into some method ---
    uint64_t pos;
    smatrix_rmap_t* rmap = &self->rmap;

    for (pos = 0; pos < rmap->size; pos++) {
      if ((rmap->data[pos].flags & SMATRIX_ROW_FLAG_USED) != 0) {
        rmap->data[pos].next = smatrix_malloc(self, sizeof(smatrix_rmap_t));

        if (!rmap->data[pos].next) {
          printf("can't load first level index. mem limit too small?\n");
          exit(1); // fixpaul proper error handling
        }

        ((smatrix_rmap_t *) rmap->data[pos].next)->fpos = rmap->data[pos].value;
        smatrix_rmap_load(self, rmap->data[pos].next);
        //smatrix_unswap(self, rmap->data[pos].next);
      }
    }
    // ---
  }

  return self;
}

void smatrix_close(smatrix_t* self) {
  smatrix_sync(self);
  smatrix_gc(self);

  uint64_t pos;
  smatrix_rmap_t* rmap = &self->rmap;

  for (pos = 0; pos < rmap->size; pos++) {
    if ((rmap->data[pos].flags & SMATRIX_ROW_FLAG_USED) != 0) {
      smatrix_mfree(self, sizeof(smatrix_rmap_t));
      pthread_rwlock_destroy(&((smatrix_rmap_t *) rmap->data[pos].next)->lock);
      free(rmap->data[pos].next);
    }
  }

  smatrix_mfree(self, sizeof(smatrix_rmap_slot_t) * rmap->size);
  free(rmap->data);

  printf("in used at exit: %lu\n", self->mem);

  pthread_rwlock_destroy(&rmap->lock);
  close(self->fd);
  free(self);
}

// FIXPAUL: this needs to be atomic (compare and swap!) or locked
uint64_t smatrix_falloc(smatrix_t* self, uint64_t bytes) {
  uint64_t old = self->fpos;
  uint64_t new = old + bytes;

  //printf("TRUNCATE TO %li\n", new);
  if (ftruncate(self->fd, new) == -1) {
    perror("FATAL ERROR: CANNOT TRUNCATE FILE"); // FIXPAUL
    abort();
  }

  self->fpos = new;
  return old;
}

void* smatrix_malloc(smatrix_t* self, size_t bytes) {
  // FIXPAUL must be compare and swap
  //if (self->mem > 10005840) {
  //  return NULL;
  //}

  self->mem += bytes;

  void* ptr = malloc(bytes);
  return ptr;
}

void smatrix_mfree(smatrix_t* self, size_t bytes) {
  // FIXPAUL must be compare and swap
  self->mem -= bytes;
}

// FIXPAUL: this needs to be atomic (compare and swap!) or locked
void smatrix_ffree(smatrix_t* self, uint64_t fpos, uint64_t bytes) {
  //printf("FREED %llu bytes @ %llu\n", bytes, fpos);
}

// FIXPAUL: this should lock
// FIXPAUL implement this with free lists...
void smatrix_sync(smatrix_t* self) {
  uint64_t pos;

  pthread_rwlock_rdlock(&self->rmap.lock);

  for (pos = 0; pos < self->rmap.size; pos++) {
    if ((self->rmap.data[pos].flags & SMATRIX_ROW_FLAG_USED) != 0) {
      pthread_rwlock_rdlock(&((smatrix_rmap_t *) self->rmap.data[pos].next)->lock);
      if ((((smatrix_rmap_t *) self->rmap.data[pos].next)->flags & SMATRIX_RMAP_FLAG_SWAPPED) == 0) {
        smatrix_rmap_sync(self, (smatrix_rmap_t *) self->rmap.data[pos].next);
      }
      pthread_rwlock_unlock(&((smatrix_rmap_t *) self->rmap.data[pos].next)->lock);
    }
  }

  smatrix_rmap_sync(self, &self->rmap);
  smatrix_meta_sync(self);

  pthread_rwlock_unlock(&self->rmap.lock);
}

// no locks must be held by the caller of this method
void smatrix_gc(smatrix_t* self) {
  uint64_t pos;

  pthread_rwlock_rdlock(&self->rmap.lock);

  for (pos = 0; pos < self->rmap.size; pos++) {
    if ((self->rmap.data[pos].flags & SMATRIX_ROW_FLAG_USED) != 0) {
      if ((((smatrix_rmap_t *) self->rmap.data[pos].next)->flags & SMATRIX_RMAP_FLAG_SWAPPED) == 0) {
        pthread_rwlock_wrlock(&((smatrix_rmap_t *) self->rmap.data[pos].next)->lock);
        if ((((smatrix_rmap_t *) self->rmap.data[pos].next)->flags & SMATRIX_RMAP_FLAG_SWAPPED) == 0) {
          smatrix_swap(self, (smatrix_rmap_t *) self->rmap.data[pos].next);
        }
        pthread_rwlock_unlock(&((smatrix_rmap_t *) self->rmap.data[pos].next)->lock);
      }
    }
  }

  pthread_rwlock_unlock(&self->rmap.lock);
}

void smatrix_rmap_init(smatrix_t* self, smatrix_rmap_t* rmap, uint64_t size) {
  rmap->size = size;
  rmap->used = 0;

  if (!rmap->fpos) {
    rmap->fpos = smatrix_falloc(self, size * 16 + 16);
  }

  pthread_rwlock_init(&rmap->lock, NULL);
}

void smatrix_update(smatrix_t* self, uint32_t x, uint32_t y) {
  smatrix_rmap_t *rmap;
  smatrix_rmap_slot_t *slot, *xslot = NULL, *yslot = NULL;
  uint64_t old_fpos, new_fpos;

  while (!xslot) {
    pthread_rwlock_rdlock(&self->rmap.lock);
    slot = smatrix_rmap_lookup(self, &self->rmap, x);

    if (slot && slot->key == x && (slot->flags & SMATRIX_ROW_FLAG_USED) != 0) {
      xslot = slot;
    } else {
      // of course, un- and then re-locking introduces a race, this is handeled
      // in smatrix_rmap_insert (it returns the existing row if one found)
      pthread_rwlock_unlock(&self->rmap.lock);
      pthread_rwlock_wrlock(&self->rmap.lock);
      xslot = smatrix_rmap_insert(self, &self->rmap, x);

      if (xslot == NULL) {
        pthread_rwlock_unlock(&self->rmap.lock);
        smatrix_gc(self);
        return smatrix_update(self, x,  y);
      }

      if (!xslot->next && !xslot->value) {
        xslot->next = smatrix_malloc(self, sizeof(smatrix_rmap_t));

        if (xslot->next == NULL) {
          xslot->flags = 0;
          pthread_rwlock_unlock(&self->rmap.lock);
          smatrix_gc(self);
          return smatrix_update(self, x,  y);
        }

        ((smatrix_rmap_t *) xslot->next)->data = smatrix_malloc(self, sizeof(smatrix_rmap_slot_t) * SMATRIX_RMAP_INITIAL_SIZE);

        if (((smatrix_rmap_t *) xslot->next)->data == NULL) {
          xslot->flags = 0;
          pthread_rwlock_unlock(&self->rmap.lock);
          smatrix_gc(self);
          return smatrix_update(self, x,  y);
        }

        smatrix_rmap_init(self, xslot->next, SMATRIX_RMAP_INITIAL_SIZE);
        memset(((smatrix_rmap_t *) xslot->next)->data, 0, sizeof(smatrix_rmap_slot_t) * SMATRIX_RMAP_INITIAL_SIZE);
      }
    }

    if (xslot) {
      // FIXPAUL flag set needs to be a compare and swap loop as we might only hold a read lock
      xslot->flags |= SMATRIX_ROW_FLAG_DIRTY;

      old_fpos = xslot->value;
      rmap     = (smatrix_rmap_t *) xslot->next;
      assert(rmap != NULL);
      pthread_rwlock_wrlock(&rmap->lock);
    }

    pthread_rwlock_unlock(&self->rmap.lock);
  }

  if ((rmap->flags & SMATRIX_RMAP_FLAG_SWAPPED) != 0) {
    smatrix_unswap(self, rmap);

    if ((rmap->flags & SMATRIX_RMAP_FLAG_SWAPPED) != 0) {
      pthread_rwlock_unlock(&rmap->lock);
      smatrix_gc(self);
      return smatrix_update(self, x,  y);
    }
  }

  yslot = smatrix_rmap_insert(self, rmap, y);

  if (yslot == NULL) {
    pthread_rwlock_unlock(&rmap->lock);
    smatrix_gc(self);
    return smatrix_update(self, x,  y);
  }

  assert(yslot != NULL);
  assert(yslot->key == y);

  //printf("####### UPDATING (%lu,%lu) => %llu\n", x, y, yslot->value++); // FIXPAUL
  yslot->flags |= SMATRIX_ROW_FLAG_DIRTY;

  new_fpos = rmap->fpos;
  pthread_rwlock_unlock(&rmap->lock);

  if (old_fpos != new_fpos) {
    pthread_rwlock_wrlock(&self->rmap.lock);
    xslot = smatrix_rmap_lookup(self, &self->rmap, x);
    assert(xslot != NULL);
    xslot->value = new_fpos;
    xslot->flags |= SMATRIX_ROW_FLAG_DIRTY;
    pthread_rwlock_unlock(&self->rmap.lock);
  }
}

// you need to hold a write lock on rmap to call this function safely
smatrix_rmap_slot_t* smatrix_rmap_insert(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key) {
  smatrix_rmap_slot_t* slot;

  if (rmap->used > rmap->size / 2) {
    smatrix_rmap_resize(self, rmap);

    if (rmap->used > rmap->size / 2) {
      return NULL;
    }
  }

  slot = smatrix_rmap_lookup(self, rmap, key);
  assert(slot != NULL);

  if ((slot->flags & SMATRIX_ROW_FLAG_USED) == 0 || slot->key != key) {
    rmap->used++;
    slot->key   = key;
    slot->value = 0;
    slot->flags = SMATRIX_ROW_FLAG_USED | SMATRIX_ROW_FLAG_DIRTY;
    slot->next  = NULL;
  }

  return slot;
}

// you need to hold a read or write lock on rmap to call this function safely
smatrix_rmap_slot_t* smatrix_rmap_lookup(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key) {
  uint64_t n, pos;

  pos = key % rmap->size;

  // linear probing
  for (n = 0; n < rmap->size; n++) {
    if ((rmap->data[pos].flags & SMATRIX_ROW_FLAG_USED) == 0)
      break;

    if (rmap->data[pos].key == key)
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

  size_t old_bytes_disk = 16 * rmap->size + 16;
  size_t old_bytes_mem = sizeof(smatrix_rmap_slot_t) * rmap->size;
  size_t new_bytes_disk = 16 * new_size + 16;
  size_t new_bytes_mem = sizeof(smatrix_rmap_slot_t) * new_size;

  new.used = 0;
  new.size = new_size;
  new.data = smatrix_malloc(self, new_bytes_mem);

  if (new.data == NULL) {
    return;
  }

  memset(new.data, 0, new_bytes_mem);

  for (pos = 0; pos < rmap->size; pos++) {
    if ((rmap->data[pos].flags & SMATRIX_ROW_FLAG_USED) == 0)
      continue;

    slot = smatrix_rmap_insert(self, &new, rmap->data[pos].key);

    if (slot == NULL) {
      smatrix_mfree(self, new_bytes_mem);
      free(new.data);
      return;
    }

    slot->value = rmap->data[pos].value;
    slot->next  = rmap->data[pos].next;
  }

  old_data = rmap->data;
  old_fpos = rmap->fpos;

  rmap->fpos = smatrix_falloc(self, new_bytes_disk);
  rmap->data = new.data;
  rmap->size = new.size;
  rmap->used = new.used;

  smatrix_ffree(self, old_fpos, old_bytes_disk);
  smatrix_mfree(self, old_bytes_mem);

  free(old_data);
}

// the caller of this must hold a read lock on rmap
// FIXPAUL: this is doing waaaay to many pwrite syscalls for a large, dirty rmap...
// FIXPAUL: also, the meta info needs to be written only on the first write
void smatrix_rmap_sync(smatrix_t* self, smatrix_rmap_t* rmap) {
  uint64_t pos = 0, fpos;
  char slot_buf[16] = {0};

  fpos = rmap->fpos;

  // FIXPAUL: what is byte ordering?
  memset(&slot_buf[0], 0x23,          8);
  memcpy(&slot_buf[8], &rmap->size,   8);
  pwrite(self->fd, &slot_buf, 16, fpos); // FIXPAUL write needs to be checked

  for (pos = 0; pos < rmap->size; pos++) {
    fpos += 16;

    // FIXPAUL this should be one if statement ;)
    if ((rmap->data[pos].flags & SMATRIX_ROW_FLAG_USED) == 0)
      continue;

    if ((rmap->data[pos].flags & SMATRIX_ROW_FLAG_DIRTY) == 0)
      continue;

    // FIXPAUL what is byte ordering?
    memset(&slot_buf[0], 0,                      4);
    memcpy(&slot_buf[4], &rmap->data[pos].key,   4);
    memcpy(&slot_buf[8], &rmap->data[pos].value, 8);

    pwrite(self->fd, &slot_buf, 16, fpos); // FIXPAUL write needs to be checked

    // FIXPAUL flag unset needs to be a compare and swap loop as we only hold a read lock
    rmap->data[pos].flags &= ~SMATRIX_ROW_FLAG_DIRTY;
  }
}

void smatrix_rmap_load(smatrix_t* self, smatrix_rmap_t* rmap) {
  char meta_buf[16] = {0};

  if (pread(self->fd, &meta_buf, 16, rmap->fpos) != 16) {
    printf("CANNOT LOAD RMATRIX\n"); // FIXPAUL
    abort();
  }

  if (memcmp(&meta_buf, &SMATRIX_RMAP_MAGIC, SMATRIX_RMAP_MAGIC_SIZE)) {
    printf("FILE IS CORRUPT\n"); // FIXPAUL
    abort();
  }

  // FIXPAUL what is big endian?
  memcpy(&rmap->size, &meta_buf[8], 8);
  smatrix_rmap_init(self, rmap, rmap->size);
  rmap->flags = SMATRIX_RMAP_FLAG_SWAPPED;
}

// caller must hold a write lock on rmap
void smatrix_swap(smatrix_t* self, smatrix_rmap_t* rmap) {
  smatrix_rmap_sync(self, rmap);
  rmap->flags |= SMATRIX_RMAP_FLAG_SWAPPED;
  smatrix_mfree(self, sizeof(smatrix_rmap_slot_t) * rmap->size);
  free(rmap->data);
}

// caller must hold a write lock on rmap
void smatrix_unswap(smatrix_t* self, smatrix_rmap_t* rmap) {
  size_t data_size = sizeof(smatrix_rmap_slot_t) * rmap->size;
  rmap->data = smatrix_malloc(self, data_size);

  if (rmap->data == NULL) {
    return;
  }

  memset(rmap->data, 0, data_size);

  size_t read_bytes, rmap_bytes;
  uint64_t pos;
  rmap_bytes = rmap->size * 16;
  char* buf = malloc(rmap_bytes);

  if (buf == NULL) {
    smatrix_mfree(self, data_size);
    free(rmap->data);
    return;
  }

  read_bytes = pread(self->fd, buf, rmap_bytes, rmap->fpos + 16);

  if (read_bytes != rmap_bytes) {
    printf("CANNOT LOAD RMATRIX\n"); // FIXPAUL
    abort();
  }

  // byte ordering FIXPAUL
  for (pos = 0; pos < rmap->size; pos++) {
    memcpy(&rmap->data[pos].value, buf + pos * 16 + 8, 8);

    if (rmap->data[pos].value) {
      memcpy(&rmap->data[pos].key, buf + pos * 16 + 4, 4);
      rmap->used++;
      rmap->data[pos].flags = SMATRIX_ROW_FLAG_USED;
    }
  }

  rmap->flags &= ~SMATRIX_RMAP_FLAG_SWAPPED;
  free(buf);
}

void smatrix_meta_sync(smatrix_t* self) {
  char buf[SMATRIX_META_SIZE];

  memset(&buf, 0, SMATRIX_META_SIZE);
  memset(&buf, 0x17, 8);

  // FIXPAUL what is byte ordering?
  memcpy(&buf[8],  &self->rmap.fpos, 8);

  pwrite(self->fd, &buf, SMATRIX_META_SIZE, 0);
}

void smatrix_meta_load(smatrix_t* self) {
  char buf[SMATRIX_META_SIZE];
  size_t read;

  read = pread(self->fd, &buf, SMATRIX_META_SIZE, 0);

  if (read != SMATRIX_META_SIZE) {
    printf("CANNOT READ FILE HEADER\n"); //FIXPAUL
    abort();
  }

  if (buf[0] != 0x17 || buf[1] != 0x17) {
    printf("INVALID FILE HEADER\n"); //FIXPAUL
    abort();
  }

  // FIXPAUL because f**k other endianess, thats why...
  memcpy(&self->rmap.fpos, &buf[8],  8);
}

/*

void smatrix_wrlock(smatrix_t* self) {
  pthread_rwlock_unlock(&self->lock);
  pthread_rwlock_wrlock(&self->lock);
}

void smatrix_unlock(smatrix_t* self) {
  pthread_rwlock_unlock(&self->lock);
  pthread_rwlock_rdlock(&self->lock);
}

void smatrix_vec_lock(smatrix_vec_t* vec) {
  for (;;) {
    __sync_synchronize();
    volatile uint32_t flags = vec->flags;

    if ((flags & 1) == 1)
      continue;

    if (__sync_bool_compare_and_swap(&vec->flags, flags, flags | 1))
      break;
  }
}

void smatrix_vec_unlock(smatrix_vec_t* vec) {
  __sync_synchronize();
  vec->flags &= ~1;
}

void smatrix_vec_incref(smatrix_vec_t* vec) {
}

void smatrix_vec_decref(smatrix_vec_t* vec) {
}

*/
