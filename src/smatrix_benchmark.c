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

#include "smatrix.h"

smatrix_t* smx_mem;

int test_pset[] = { 4990250,22365309,21407061,24005841,19108133,24265765,14882906,15989594,15521590,8940574,9033418,2673414,26775961,14683985,8767274,7109242,12707246,994998,21842305,14255953,24085733,5532030,23940901,18045917,3560214,24204993,50324,15000750,8425214,8582218,1048794,19500129,27556825,17381005,23392101,8288626,16128922,309437,23832789,7624090,16266302,25173329,23401173,571730,7856082,25810797,24685825,15290362,13175738,25834349,3598926, 0 };

void benchmark_pset_index(smatrix_t* smx, int runs) {
  int i, n, r;

  for (r = 0; r < runs; r++) {
    for (n = 0; test_pset[n]; n++) {
      for (i = 0; test_pset[i]; i++) {
        smatrix_incr(smx, test_pset[n], test_pset[i], 1);
      }
    }
  }
}

void measure(void (*cb)(smatrix_t*, int), int nthreads, smatrix_t* smx, int user1) {
  char str[20] = "           ";
  double elapsed;
  struct timeval t0, t1;

  gettimeofday(&t0, NULL);
  cb(smx, user1);
  gettimeofday(&t1, NULL);

  elapsed  = (double) (t1.tv_sec - t0.tv_sec)   * 1000;
  elapsed += (double) (t1.tv_usec - t0.tv_usec) / 1000;

  str[snprintf(str, 20, " %.1fms", elapsed)] = ' ';

  printf("%s|", str);
}

int main(int argc, char** argv) {
  int i;

  smx_mem = smatrix_open(NULL);

  printf("libsmatrix benchmark [date]\n");
  printf("--------------------------------------------------------------------------------------------------\n");
  printf("                         | T=1       | T=2       | T=4       | T=8       | T=16      | T=32      |\n");
  printf("--------------------------------------------------------------------------------------------------\n");

  printf("index 1k psets (mem)     |");
  measure(&benchmark_pset_index, 1, smx_mem, 1000);
  measure(&benchmark_pset_index, 2, smx_mem, 1000);
  measure(&benchmark_pset_index, 4, smx_mem, 1000);
  measure(&benchmark_pset_index, 8, smx_mem, 1000);
  measure(&benchmark_pset_index, 16, smx_mem, 1000);
  measure(&benchmark_pset_index, 32, smx_mem, 1000);
  printf("\n");

  printf("index 1k psets (mem)     |");
  measure(&benchmark_pset_index, 1, smx_mem, 1000);
  measure(&benchmark_pset_index, 2, smx_mem, 1000);
  measure(&benchmark_pset_index, 4, smx_mem, 1000);
  measure(&benchmark_pset_index, 8, smx_mem, 1000);
  measure(&benchmark_pset_index, 16, smx_mem, 1000);
  measure(&benchmark_pset_index, 32, smx_mem, 1000);
  printf("\n");

  smatrix_close(smx_mem);
}
