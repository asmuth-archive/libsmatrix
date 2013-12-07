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

smatrix_t* smx_mem;

int test_pset[] = { 4990250,22365309,21407061,24005841,19108133,24265765,14882906,15989594,15521590,8940574,9033418,2673414,26775961,14683985,8767274,7109242,12707246,994998,21842305,14255953,24085733,5532030,23940901,18045917,3560214,24204993,50324,15000750,8425214,8582218,1048794,19500129,27556825,17381005,23392101,8288626,16128922,309437,23832789,7624090,16266302,25173329,23401173,571730,7856082,25810797,24685825,15290362,13175738,25834349,3598926, 0 };

void benchmark_pset_index(smatrix_t* smx, int runs) {
  int i, n, t;

  for (i = 0; i < runs; i++) {
    for (n = 0; test_pset[n]; n++) {
      for (i = 0; test_pset[i]; i++) {
        smatrix_incr(smx, test_pset[n], test_pset[i], 1);
      }
    }
  }
}

int main(int argc, char** argv) {
  int i;

  smx_mem = smatrix_open(NULL);

  printf("libsmatrix benchmark [date]\n");

  printf("-------------------------------------------------------------------------------------------------\n");
  printf("test                     | 1 thread | 2 threads | 4 threads | 8 threads | 16 threads | 32 threads\n");
  printf("-------------------------------------------------------------------------------------------------\n");

  printf("index 10k psets (mem)\n");
  benchmark_pset_index(smx_mem, 10000);

  printf("done");
  smatrix_close(smx_mem);
}
