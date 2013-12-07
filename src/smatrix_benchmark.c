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
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

#include "smatrix.h"

typedef struct {
  smatrix_t* smx;
  int        threadn;
  int        user1;
} args_t;

smatrix_t* smx_mem;

void* benchmark_incr_mixed(void* args_) {
  args_t* args = (args_t*) args_;
  int i, n, r, o;

  o = 42 + args->threadn;
  smatrix_t* smx = args->smx;

  for (r = 0; r < args->user1; r++) {
    for (n = 0; n < 23; n++) {
      for (i = 0; i < 22; i++) {
        smatrix_incr(smx, n + o, i + o, 1);
        smatrix_incr(smx, i + o, n + o, 1);
      }
    }
  }

  return NULL;
}

void* benchmark_incr_independent(void* args_) {
  args_t* args = (args_t*) args_;
  int i, r, o;

  smatrix_t* smx = args->smx;
  o = 123 + args->threadn;

  for (r = 0; r < args->user1; r++) {
    for (i = 0; i < 1000; i++) {
      smatrix_incr(smx, o, 1, 1);
    }
  }

  return NULL;
}

void* benchmark_incr_compete(void* args_) {
  args_t* args = (args_t*) args_;
  int i, r;

  smatrix_t* smx = args->smx;

  for (r = 0; r < args->user1; r++) {
    for (i = 0; i < 1000; i++) {
      smatrix_incr(smx, 23, 1, 1);
    }
  }

  return NULL;
}

void measure(void* (*cb)(void*), int nthreads, smatrix_t* smx, int user1) {
  char str[20] = "           ";
  int n;
  pthread_t* threads;
  double elapsed;
  struct timeval t0, t1;
  void* retval;

  args_t* args = malloc(sizeof(args_t) * nthreads);
  threads = malloc(sizeof(pthread_t) * nthreads);

  gettimeofday(&t0, NULL);

  for (n = 0; n < nthreads; n++) {
    args[n].smx     = smx;
    args[n].threadn = n;
    args[n].user1   = user1;
    pthread_create(threads + n, NULL, cb, &args[n]);
  }

  for (n = 0; n < nthreads; n++) {
    pthread_join(threads[n], &retval);
  }

  gettimeofday(&t1, NULL);

  elapsed  = (double) (t1.tv_sec - t0.tv_sec)   * 1000;
  elapsed += (double) (t1.tv_usec - t0.tv_usec) / 1000;

  str[snprintf(str, 20, " %.1fms", elapsed)] = ' ';

  free(threads);
  free(args);

  printf("%s|", str);
}

void print_header() {
  printf("libsmatrix benchmark [date]\n");
  printf("-------------------------------------------------------------------------------------------------------------\n");
  printf("                                    | T=1       | T=2       | T=4       | T=8       | T=16      | T=32      |\n");
  printf("-------------------------------------------------------------------------------------------------------------\n");
}

void test_incr() {
  smatrix_t* smx_mem = smatrix_open(NULL);

  print_header();

  for (;;) {
    printf("500k x incr (independent, mem)      |");
    measure(&benchmark_incr_independent, 1,  smx_mem, 512);
    measure(&benchmark_incr_independent, 2,  smx_mem, 256);
    measure(&benchmark_incr_independent, 4,  smx_mem, 128);
    measure(&benchmark_incr_independent, 8,  smx_mem, 64);
    measure(&benchmark_incr_independent, 16, smx_mem, 32);
    measure(&benchmark_incr_independent, 32, smx_mem, 16);
    printf("\n");

    printf("100k x incr (competing)             |");
    measure(&benchmark_incr_compete, 1,  smx_mem, 128);
    measure(&benchmark_incr_compete, 2,  smx_mem, 64);
    measure(&benchmark_incr_compete, 4,  smx_mem, 32);
    measure(&benchmark_incr_compete, 8,  smx_mem, 16);
    measure(&benchmark_incr_compete, 16, smx_mem, 8);
    measure(&benchmark_incr_compete, 32, smx_mem, 4);
    printf("\n");

    printf("500k x incr (mixed, mem)            |");
    measure(&benchmark_incr_mixed, 1,  smx_mem, 512);
    measure(&benchmark_incr_mixed, 2,  smx_mem, 256);
    measure(&benchmark_incr_mixed, 4,  smx_mem, 128);
    measure(&benchmark_incr_mixed, 8,  smx_mem, 64);
    measure(&benchmark_incr_mixed, 16, smx_mem, 32);
    measure(&benchmark_incr_mixed, 32, smx_mem, 16);
    printf("\n");

    printf("-------------------------------------------------------------------------------------------------------------\n");
  }

  smatrix_close(smx_mem);
}

void test_full() {
  smatrix_t* smx_mem = smatrix_open(NULL);

  print_header();

  printf("1M x incr (independent, mem, cold)  |");
  measure(&benchmark_incr_independent, 1,  smx_mem, 1024);
  measure(&benchmark_incr_independent, 2,  smx_mem, 1024 / 2);
  measure(&benchmark_incr_independent, 4,  smx_mem, 1024 / 4);
  measure(&benchmark_incr_independent, 8,  smx_mem, 1024 / 8);
  measure(&benchmark_incr_independent, 16, smx_mem, 1024 / 16);
  measure(&benchmark_incr_independent, 32, smx_mem, 1024 / 32);
  printf("\n");

  printf("1M x incr (independent, mem, warm)  |");
  measure(&benchmark_incr_independent, 1,  smx_mem, 1024);
  measure(&benchmark_incr_independent, 2,  smx_mem, 1024 / 2);
  measure(&benchmark_incr_independent, 4,  smx_mem, 1024 / 4);
  measure(&benchmark_incr_independent, 8,  smx_mem, 1024 / 8);
  measure(&benchmark_incr_independent, 16, smx_mem, 1024 / 16);
  measure(&benchmark_incr_independent, 32, smx_mem, 1024 / 32);
  printf("\n");

  printf("1M x incr (competing, mem, cold)    |");
  measure(&benchmark_incr_compete, 1,  smx_mem, 1024);
  measure(&benchmark_incr_compete, 2,  smx_mem, 1024 / 2);
  measure(&benchmark_incr_compete, 4,  smx_mem, 1024 / 4);
  measure(&benchmark_incr_compete, 8,  smx_mem, 1024 / 8);
  measure(&benchmark_incr_compete, 16, smx_mem, 1024 / 16);
  measure(&benchmark_incr_compete, 32, smx_mem, 1024 / 32);
  printf("\n");

  printf("1M x incr (competing, mem, warm)    |");
  measure(&benchmark_incr_compete, 1,  smx_mem, 1024);
  measure(&benchmark_incr_compete, 2,  smx_mem, 1024 / 2);
  measure(&benchmark_incr_compete, 4,  smx_mem, 1024 / 4);
  measure(&benchmark_incr_compete, 8,  smx_mem, 1024 / 8);
  measure(&benchmark_incr_compete, 16, smx_mem, 1024 / 16);
  measure(&benchmark_incr_compete, 32, smx_mem, 1024 / 32);
  printf("\n");

  printf("1M x incr (mixed, mem, cold)        |");
  measure(&benchmark_incr_mixed, 1,  smx_mem, 1024);
  measure(&benchmark_incr_mixed, 2,  smx_mem, 1024 / 2);
  measure(&benchmark_incr_mixed, 4,  smx_mem, 1024 / 4);
  measure(&benchmark_incr_mixed, 8,  smx_mem, 1024 / 8);
  measure(&benchmark_incr_mixed, 16, smx_mem, 1024 / 16);
  measure(&benchmark_incr_mixed, 32, smx_mem, 1024 / 32);
  printf("\n");

  printf("1M x incr (mixed, mem, warm)        |");
  measure(&benchmark_incr_mixed, 1,  smx_mem, 1024);
  measure(&benchmark_incr_mixed, 2,  smx_mem, 1024 / 2);
  measure(&benchmark_incr_mixed, 4,  smx_mem, 1024 / 4);
  measure(&benchmark_incr_mixed, 8,  smx_mem, 1024 / 8);
  measure(&benchmark_incr_mixed, 16, smx_mem, 1024 / 16);
  measure(&benchmark_incr_mixed, 32, smx_mem, 1024 / 32);
  printf("\n");

  smatrix_close(smx_mem);
}

int main(int argc, char** argv) {
  if (argc > 1) {
    if (!strcmp(argv[1], "incr")) {
      test_incr();
      return 0;
    }
  }

  test_full();
}
