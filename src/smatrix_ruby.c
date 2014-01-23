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

smatrix_t* smatrix_rb_gethandle(VALUE self) {
  return NULL;
}

VALUE smatrix_rb_initialize(VALUE self, VALUE filename) {
  smatrix_t* smatrix = NULL;
  VALUE      smatrix_handle = NULL;

  switch (rb_type(filename)) {

    case RUBY_T_STRING:
      smatrix = smatrix_open(RSTRING_PTR(filename));
      break;

    case RUBY_T_NIL:
      smatrix = smatrix_open(NULL);
      break;

    default:
      rb_raise(rb_eTypeError, "first argument (filename) must be nil or a string");
      break;

  }

  if (smatrix) {
    smatrix_handle = Data_Wrap_Struct(rb_cObject, NULL, NULL, smatrix);
    rb_iv_set(self, "@handle", smatrix_handle);
  }

  return self;
}

VALUE smatrix_rb_get(VALUE self, VALUE x, VALUE y) {
  smatrix_t* smatrix = smatrix_rb_gethandle(self);

  if (!smatrix) {
    rb_raise(rb_eTypeError, "smatrix @handle is Nil, something went horribly wrong :(");
    return Qnil;
  }

  printf("get called: %p\n", smatrix);
}

void Init_libsmatrix() {
  VALUE klass = rb_define_class("SparseMatrix", rb_cObject);

  rb_define_method(klass, "initialize", smatrix_rb_initialize, 1);
  rb_define_method(klass, "get", smatrix_rb_get, 2);
}
