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

#ifndef SMATRIX_RUBY_H
#define SMATRIX_RUBY_H

#ifndef RUBY_T_STRING
#define RUBY_T_STRING T_STRING
#endif

void smatrix_rb_gethandle(VALUE self, smatrix_t** handle);
VALUE smatrix_rb_get(VALUE self, VALUE x, VALUE y);
VALUE smatrix_rb_initialize(VALUE self, VALUE filename);
VALUE smatrix_rb_incr(VALUE self, VALUE x, VALUE y, VALUE value);
VALUE smatrix_rb_decr(VALUE self, VALUE x, VALUE y, VALUE value);
VALUE smatrix_rb_set(VALUE self, VALUE x, VALUE y, VALUE value);
void smatrix_rb_free(smatrix_t* smatrix);
void Init_libsmatrix();

#endif
