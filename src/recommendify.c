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
