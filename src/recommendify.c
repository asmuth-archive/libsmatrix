#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "recommendify.h"
#include "version.h"

int main(int argc, char **argv) {
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
