// This file is part of the "recommendify" project
//   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <stdlib.h>
#include <stdint.h>

#include "smatrix.h"

#ifndef CF_H
#define CF_H

void cf_add_session(smatrix_t* smatrix, uint32_t* session, size_t size);

#endif
