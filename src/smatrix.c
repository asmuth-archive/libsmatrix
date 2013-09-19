// This file is part of the "recommendify" project
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

#include "smatrix.h"

smatrix_t* smatrix_open(const char* fname) {
  smatrix_t* self = malloc(sizeof(smatrix_t));

  if (self == NULL)
    return NULL;

// FILE INIT

  self->fd = open(fname, O_RDWR | O_CREAT, 00600);

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
    smatrix_meta_sync(self);
  } else {
    printf("LOAD FILE!\n");
    smatrix_meta_load(self);
    smatrix_rmap_load(self, &self->rmap);
  }

  return self;
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

// FIXPAUL: this needs to be atomic (compare and swap!) or locked
void smatrix_ffree(smatrix_t* self, uint64_t fpos, uint64_t bytes) {
  printf("FREED %lu bytes @ %lu\n", fpos, bytes);

  // FIXPAUL DEBUG ONLY ;)
  char* fnord = malloc(bytes);
  memset(fnord, 0x42, bytes);
  pwrite(self->fd, fnord, bytes, fpos);
  free(fnord);
}

void smatrix_sync(smatrix_t* self) {
  smatrix_rmap_sync(self, &self->rmap);
  smatrix_meta_sync(self);
}

void smatrix_rmap_init(smatrix_t* self, smatrix_rmap_t* rmap, uint64_t size) {
  size_t data_size = sizeof(smatrix_rmap_slot_t) * size;

  rmap->size = size;
  rmap->used = 0;
  rmap->data = malloc(data_size);

  if (rmap->data == NULL) {
    printf("MALLOC DATA FAILED IN RMAP_INIT\n"); //FIXPAUL
    abort();
  }

  if (!rmap->fpos) {
    printf("FALLLLLOC %li\n", size * 16 + 16);
    rmap->fpos = smatrix_falloc(self, size * 16 + 16);
  }

  memset(rmap->data, 0, data_size);
  pthread_rwlock_init(&rmap->lock, NULL);
}

// this method will be removed later
uint64_t smatrix_rmap_get(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key) {
  uint64_t ret = 0;
  smatrix_rmap_slot_t* slot;

  pthread_rwlock_rdlock(&self->rmap.lock);
  slot = smatrix_rmap_lookup(self, &self->rmap, key);

  if (slot && slot->key == key) {
    ret = slot->value++; // ;)
  }

  pthread_rwlock_unlock(&self->rmap.lock);

  return ret;
}


// you need to hold a write lock on rmap to call this function safely
smatrix_rmap_slot_t* smatrix_rmap_insert(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key) {
  smatrix_rmap_slot_t* slot;

  if (rmap->used > rmap->size / 2) {
    smatrix_rmap_resize(self, rmap);
  }

  slot = smatrix_rmap_lookup(self, rmap, key);

  if (slot == NULL) {
    abort();
  }

  printf("INSERTING::: %i\n", key);

  if (slot->key != key) {
    rmap->used++;
    slot->key   = key;
    slot->flags = SMATRIX_ROW_FLAG_USED | SMATRIX_ROW_FLAG_DIRTY;
  }

  return slot;
}

/*
  if (row == NULL) {
    row = malloc(sizeof(smatrix_row_t)); // FIXPAUL never freed :(
    row->flags = SMATRIX_ROW_FLAG_DIRTY;
    row->index = key;
    row->fpos = smatrix_falloc(self, 666);

    if (smatrix_rmap_lookup(&self->rmap, key, row) != row) {
      free(row);
      return smatrix_rmap_lookup(&self->rmap, key, row);
    }
  }
*/

// you need to hold a read or write lock on rmap to call this function safely
smatrix_rmap_slot_t* smatrix_rmap_lookup(smatrix_t* self, smatrix_rmap_t* rmap, uint32_t key) {
  long int n, pos;

  pos = key % rmap->size;

  for (n = 0; n < rmap->size; n++) {
    if (rmap->data[pos].flags & SMATRIX_ROW_FLAG_USED == 0)
      break;

    if (rmap->data[pos].key == key)
      break;

    pos = (pos + 1) % rmap->size;
  }

  return &rmap->data[pos];
}

void smatrix_rmap_resize(smatrix_t* self, smatrix_rmap_t* rmap) {
  smatrix_rmap_slot_t* slot;
  smatrix_rmap_t new;
  void* old_data;
  long int pos;

  uint64_t old_fpos, old_size, new_size = rmap->size * 2;

  size_t old_bytes = sizeof(smatrix_rmap_slot_t) * rmap->size;
  size_t new_bytes = sizeof(smatrix_rmap_slot_t) * new_size;

  printf("RESIZE!!!\n");
  // FIXPAUL: big problem, this doesnt re-falloc

  new.used = 0;
  new.size = new_size;
  new.data = malloc(new_bytes);

  if (new.data == NULL) {
    printf("RMAP RESIZE FAILED (MALLOC)!!!\n"); // FIXPAUL
    abort();
  }

  memset(new.data, 0, new_bytes);

  for (pos = 0; pos < rmap->size; pos++) {
    if (!rmap->data[pos].value)
      continue;

    slot = smatrix_rmap_insert(self, &new, rmap->data[pos].key);

    if (slot == NULL) {
      printf("RMAP RESIZE FAILED (INSERT)!!!\n"); // FIXPAUL
      abort();
    }

    slot->value = rmap->data[pos].value;
    slot->ptr   = rmap->data[pos].ptr;
  }

  old_data = rmap->data;
  old_fpos = rmap->fpos;

  rmap->fpos = smatrix_falloc(self, new_bytes + 16);
  rmap->data = new.data;
  rmap->size = new.size;
  rmap->used = new.used;

  smatrix_ffree(self, old_fpos, old_bytes + 16);
  free(old_data);
}

// the caller of this must hold a read lock on rmap
// FIXPAUL: this is doing waaaay to many pwrite syscalls for a large, dirty rmap...
// FIXPAUL: also, the meta info needs to be written only on the first write
void smatrix_rmap_sync(smatrix_t* self, smatrix_rmap_t* rmap) {
  long int pos = 0, fpos;
  char slot_buf[16] = {0};

  fpos = rmap->fpos;

  // FIXPAUL: what is byte ordering?
  memset(&slot_buf[0], 0x23,          8);
  memcpy(&slot_buf[8], &rmap->size,   8);
  pwrite(self->fd, &slot_buf, 16, fpos); // FIXPAUL write needs to be checked

  for (pos = 0; pos < rmap->size; pos++) {
    fpos += 16;

    if (!rmap->data[pos].value)
      continue;

    if (0 && rmap->data[pos].flags & SMATRIX_ROW_FLAG_DIRTY) // FIXPAUL
      continue;

    // FIXPAUL what is byte ordering?
    memset(&slot_buf[0], 0,                      4);
    memcpy(&slot_buf[4], &rmap->data[pos].key,   4);
    memcpy(&slot_buf[8], &rmap->data[pos].value, 8);

    printf("PERSIST %i->%lu @ %li\n", rmap->data[pos].key, rmap->data[pos].value, fpos);
    pwrite(self->fd, &slot_buf, 16, fpos); // FIXPAUL write needs to be checked

    // FIXPAUL flag unset needs to be a compare and swap loop as we only hold a read lock
    rmap->data[pos].flags &= ~SMATRIX_ROW_FLAG_DIRTY;
  }
}

void smatrix_rmap_load(smatrix_t* self, smatrix_rmap_t* rmap) {
  char meta_buf[16] = {0};
  size_t read, rmap_bytes;

  uint64_t pos;

  printf("READ AT %li\n", rmap->fpos);
  read = pread(self->fd, &meta_buf, 16, rmap->fpos);

  if (read != 16) {
    printf("CANNOT LOAD RMATRIX\n"); // FIXPAUL
    abort();
  }

  if (memcmp(&meta_buf, &SMATRIX_RMAP_MAGIC, SMATRIX_RMAP_MAGIC_SIZE)) {
    printf("FILE IS CORRUPT\n"); // FIXPAUL
    abort();
  }

  // FIXPAUL what is big endian?
  memcpy(&rmap->size, &meta_buf[8], 8);

  printf("RMAP SIZE is %i\n", rmap->size);

  smatrix_rmap_init(self, rmap, rmap->size);

  rmap_bytes = rmap->size * 16;
  char* buf = malloc(rmap_bytes);

  if (buf == NULL) {
    printf("MALLOC FAILED!\n");
    abort();
  }

  read = pread(self->fd, buf, rmap_bytes, rmap->fpos + 16);

  if (read != rmap_bytes) {
    printf("CANNOT LOAD RMATRIX\n"); // FIXPAUL
    abort();
  }

  // byte ordering FIXPAUL
  for (pos = 0; pos < rmap->size; pos++) {
    memcpy(&rmap->data[pos].value, buf + pos * 16 + 8, 8);

    if (rmap->data[pos].value) {
      memcpy(&rmap->data[pos].key, buf + pos * 16 + 4, 4);
      self->rmap.used++;
      self->rmap.data[pos].flags = SMATRIX_ROW_FLAG_USED;
    }
    //printf("LOAD %i\n", rmap->data[pos].key);
  }

  free(buf);
}

void smatrix_meta_sync(smatrix_t* self) {
  char buf[SMATRIX_META_SIZE];

  memset(&buf, 0, SMATRIX_META_SIZE);
  memset(&buf, 0x17, 8);

  // FIXPAUL what is byte ordering?
  printf("WRITE FPOS %li\n", self->rmap.fpos);
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

smatrix_vec_t* smatrix_lookup(smatrix_t* self, uint32_t x, uint32_t y, int create) {
  smatrix_vec_t *col = NULL, **row = NULL;

  pthread_rwlock_rdlock(&self->lock);

  if (x > self->size) {
    if (x > SMATRIX_MAX_ID)
      goto unlock;

    if (create) {
      smatrix_wrlock(self);

      if (x > self->size)
        smatrix_resize(self, x + 1);

      if (x > self->size)
        goto unlock;

      smatrix_unlock(self);
    } else {
      goto unlock;
    }
  }

  row = self->data + x;

  if (*row == NULL) {
    if (!create)
      goto unlock;

    smatrix_wrlock(self);

    if (*row == NULL)
      col = smatrix_insert(row, y);

    smatrix_unlock(self);
  }

  smatrix_vec_lock(*row);

  if (col == NULL) {
    col = *row;

    while (col->index != y) {
      if (col->next == NULL || col->index > y) {
        col = NULL;
        break;
      }

      col = col->next;
    }
  }

  if (col == NULL && create) {
    smatrix_wrlock(self);
    col = smatrix_insert(row, y);
    smatrix_unlock(self);
  }

  smatrix_vec_incref(col);
  smatrix_vec_unlock(*row);

unlock:

  pthread_rwlock_unlock(&self->lock);
  return col;
}

smatrix_vec_t* smatrix_insert(smatrix_vec_t** row, uint32_t y) {
  uint32_t row_len = 1;
  smatrix_vec_t **cur = row, *next, *new;

  for (; *cur && (*cur)->index <= y; row_len++) {
    if ((*cur)->index == y) {
      return *cur;
    } else {
      cur = &((*cur)->next);
    }
  }

  next = *cur;

  *cur = new = malloc(sizeof(smatrix_vec_t));
  new->value = 666; // FIXPAUL
  new->index = y;
  new->flags = 0;
  new->next  = next;

  for (; next; row_len++)
    next = next->next;

  if (row_len > SMATRIX_MAX_ROW_SIZE)
    smatrix_truncate(row);

  return new;
}

void smatrix_resize(smatrix_t* self, uint32_t min_size) {
  long int new_size = self->size;

  while (new_size < min_size) {
    new_size = new_size * SMATRIX_GROWTH_FACTOR;
  }

  smatrix_vec_t** new_data = malloc(sizeof(void *) * new_size);

  if (new_data == NULL)
    return;

  memcpy(new_data, self->data, sizeof(void *) * self->size);
  memset(new_data, 0, sizeof(void *) * (new_size - self->size));

  free(self->data);

  self->data = new_data;
  self->size = new_size;
}

void smatrix_increment(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value) {
  smatrix_vec_t* vec = smatrix_lookup(self, x, y, 1);

  if (vec == NULL)
    return;

  // FIXPAUL check for overflow, also this is not an atomic increment
  vec->value++;
}

void smatrix_truncate(smatrix_vec_t** row) {
  smatrix_vec_t **cur = row, **found = NULL, *delete;

  while (*cur) {
    if ((*cur)->index != 0 && (*cur)->value != 0) {
      if (found == NULL || (*found)->value > (*cur)->value) {
        found = cur;
      }
    }

    cur = &((*cur)->next);
  }

  delete = *found;
  *found = delete->next;

  smatrix_vec_decref(delete);
}

void smatrix_close(smatrix_t* self) {
  smatrix_vec_t *cur, *tmp;
  uint32_t n;
/*
  for (n = 0; n < self->size; n++) {
    cur = self->data[n];

    while (cur) {
      tmp = cur;
      cur = cur->next;
      free(tmp);
    }
  }

  */

  free(self);
}

void smatrix_dump(smatrix_t* self) {
  smatrix_vec_t *cur;
  uint32_t n;

  for (n = 0; n < self->size; n++) {
    cur = self->data[n];

    if (!cur) continue;

    printf("%i ===> ", n);
    while (cur) {
      printf(" %i:%i, ", cur->index, cur->value);
      cur = cur->next;
    }
    printf("\n----\n");
  }
}

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
