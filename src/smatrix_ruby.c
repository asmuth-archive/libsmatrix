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
#include <ruby/ruby.h>
#include <ruby/defines.h>
#include <ruby/config.h>
#include "smatrix.h"
#include "smatrix_ruby.h"

VALUE smatrix_rb_initialize(VALUE self, VALUE filename) {
  printf("initialize called\n");

  switch (rb_type(filename)) {

    case RUBY_T_STRING:
      printf("init with string\n");
      break;

    case RUBY_T_NIL:
      printf("init with nil\n");
      break;

    default:
      rb_raise(rb_eTypeError, "first argument (filename) must be nil or a string");
      break;

  }

  return self;
}

void Init_libsmatrix() {
  VALUE klass = rb_define_class("SparseMatrix", rb_cObject);

  rb_define_method(klass, "initialize", smatrix_rb_initialize, 1);
}
