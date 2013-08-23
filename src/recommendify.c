#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "recommendify.h"
#include "smatrix.h"
#include "version.h"

smatrix* db;

int main(int argc, char **argv) {
  db = smatrix_init();

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
