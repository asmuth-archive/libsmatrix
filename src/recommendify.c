// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "recommendify.h"
#include "smatrix.h"
#include "version.h"

smatrix_t* db;

int main(int argc, char **argv) {
  db = smatrix_init();

  printf("INIT %p, %i\n", db, db->size);
  smatrix_lookup(db, 53415246, 22361353, 1);
  smatrix_lookup(db, 53415246, 22361353, 1);
  smatrix_lookup(db, 53415249, 22361353, 1);
  smatrix_lookup(db, 53415249, 22361353, 1);
  smatrix_lookup(db, 53415249, 22361359, 1);
  smatrix_lookup(db, 53415249, 22361359, 1);
  printf("...\n");

  print_version();
  return 0;
}

void print_version() {
  printf(
    VERSION_STRING,
    VERSION_MAJOR,
    VERSION_MINOR,
    VERSION_PATCH
  );
}
