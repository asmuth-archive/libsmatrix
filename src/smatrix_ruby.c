/**
 * This file is part of the "libsmatrix" project
 *   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
 *
 * Licensed under the MIT License (the "License"); you may not use this
 * file except in compliance with the License. You may obtain a copy of
 * the License at: http://opensource.org/licenses/MIT
 */
#include <stdlib.h>
#include <ruby/config.h>
#include <ruby/ruby.h>
#include "smatrix.h"
#include "smatrix_ruby.h"

void smatrix_rb_gethandle(VALUE self, smatrix_t** handle) {
  VALUE handle_wrapped = rb_iv_get(self, "@handle");

  if (rb_type(handle_wrapped) != RUBY_T_DATA) {
    rb_raise(rb_eTypeError, "smatrix @handle is of the wrong type, something went horribly wrong :(");
    return;
  }

  Data_Get_Struct(handle_wrapped, smatrix_t, *handle);
}

VALUE smatrix_rb_initialize(VALUE self, VALUE filename) {
  smatrix_t* smatrix = NULL;
  VALUE      smatrix_handle;

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
    smatrix_handle = Data_Wrap_Struct(rb_cObject, NULL, smatrix_rb_free, smatrix);
    rb_iv_set(self, "@handle", smatrix_handle);
  }

  return self;
}

VALUE smatrix_rb_get(VALUE self, VALUE x, VALUE y) {
  smatrix_t* smatrix = NULL;
  smatrix_rb_gethandle(self, &smatrix);

  if (!smatrix) {
    rb_raise(rb_eTypeError, "smatrix @handle is Nil, something went horribly wrong :(");
    return Qnil;
  }

  if (rb_type(x) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "first argument (x) must be a Fixnum");
    return Qnil;
  }

  if (rb_type(y) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "second argument (y) must be a Fixnum");
    return Qnil;
  }

  return INT2NUM(smatrix_get(smatrix, NUM2INT(x), NUM2INT(y)));
}

VALUE smatrix_rb_set(VALUE self, VALUE x, VALUE y, VALUE value) {
  smatrix_t* smatrix = NULL;
  smatrix_rb_gethandle(self, &smatrix);

  if (!smatrix) {
    rb_raise(rb_eTypeError, "smatrix @handle is Nil, something is very bad :'(");
    return Qnil;
  }

  if (rb_type(x) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "first argument (x) must be a Fixnum");
    return Qnil;
  }

  if (rb_type(y) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "second argument (y) must be a Fixnum");
    return Qnil;
  }

  if (rb_type(value) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "third argument must be a Fixnum");
    return Qnil;
  }

  return INT2NUM(smatrix_set(smatrix, NUM2INT(x), NUM2INT(y), NUM2INT(value)));
}

VALUE smatrix_rb_incr(VALUE self, VALUE x, VALUE y, VALUE value) {
  smatrix_t* smatrix = NULL;
  smatrix_rb_gethandle(self, &smatrix);

  if (!smatrix) {
    rb_raise(rb_eTypeError, "smatrix @handle is Nil, something is very bad :'(");
    return Qnil;
  }

  if (rb_type(x) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "first argument (x) must be a Fixnum");
    return Qnil;
  }

  if (rb_type(y) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "second argument (y) must be a Fixnum");
    return Qnil;
  }

  if (rb_type(value) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "third argument must be a Fixnum");
    return Qnil;
  }

  return INT2NUM(smatrix_incr(smatrix, NUM2INT(x), NUM2INT(y), NUM2INT(value)));
}

VALUE smatrix_rb_decr(VALUE self, VALUE x, VALUE y, VALUE value) {
  smatrix_t* smatrix = NULL;
  smatrix_rb_gethandle(self, &smatrix);

  if (!smatrix) {
    rb_raise(rb_eTypeError, "smatrix @handle is Nil, something is very bad :'(");
    return Qnil;
  }

  if (rb_type(x) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "first argument (x) must be a Fixnum");
    return Qnil;
  }

  if (rb_type(y) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "second argument (y) must be a Fixnum");
    return Qnil;
  }

  if (rb_type(value) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "third argument must be a Fixnum");
    return Qnil;
  }

  return INT2NUM(smatrix_decr(smatrix, NUM2INT(x), NUM2INT(y), NUM2INT(value)));
}

VALUE smatrix_rb_getrow(VALUE self, VALUE x, VALUE y) {
  smatrix_t* smatrix = NULL;
  smatrix_rb_gethandle(self, &smatrix);

  if (!smatrix) {
    rb_raise(rb_eTypeError, "smatrix @handle is Nil, something went horribly wrong :(");
    return Qnil;
  }

  if (rb_type(x) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "first argument (x) must be a Fixnum");
    return Qnil;
  }

  if (rb_type(y) != RUBY_T_FIXNUM) {
    rb_raise(rb_eTypeError, "first argument (y) must be a Fixnum");
    return Qnil;
  }

  uint32_t  *data, neighbors;
  data  = malloc(NUM2INT(y) * 8);
  VALUE ret_array = rb_ary_new2(NUM2INT(y));

  neighbors = smatrix_getrow(smatrix, NUM2INT(x), data, 20);

  for(int i=0; i < neighbors; i++) {
    rb_ary_store(ret_array, i, INT2NUM(data[i]));
  }
  return ret_array;
}

void smatrix_rb_free(smatrix_t* smatrix) {
  if (!smatrix) {
   rb_raise(rb_eTypeError, "smatrix @handle is Nil, something is very bad :'(");
   return;
  }

  smatrix_close(smatrix);
}

void Init_smatrix() {
  VALUE klass = rb_define_class("SparseMatrix", rb_cObject);

  rb_define_method(klass, "initialize", smatrix_rb_initialize, 1);
  rb_define_method(klass, "get", smatrix_rb_get, 2);
  rb_define_method(klass, "set", smatrix_rb_set, 3);
  rb_define_method(klass, "incr", smatrix_rb_incr, 3);
  rb_define_method(klass, "decr", smatrix_rb_decr, 3);
  rb_define_method(klass, "getrow", smatrix_rb_getrow, 2);
}

void Init_smatrix_ruby() {
  Init_smatrix();
}
