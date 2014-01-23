/**
 * This file is part of the "libsmatrix" project
 *   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
 *
 * Licensed under the MIT License (the "License"); you may not use this
 * file except in compliance with the License. You may obtain a copy of
 * the License at: http://opensource.org/licenses/MIT
 */
#include <stdlib.h>
#include <ruby.h>
#include "smatrix.h"
#include "smatrix_ruby.h"

VALUE smatrix = Qnil;

void Init_smatrix() {
  smatrix = rb_define_class("SparseMatrix", rb_cObject);
}
