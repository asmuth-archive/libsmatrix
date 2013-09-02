// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdlib.h>
#include "marshal.h"

long int marshal_load_csv(char* data, size_t size) {
  printf("LOAD %p (%i bytes)\n", data, size);
  return 42L;
}

