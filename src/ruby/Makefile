# This file is part of the "libsmatrix" project
#   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

include ../Makefile.in
include Makefile.in

TARGET = smatrix_ruby.$(LIBEXT)

all: $(TARGET)

../smatrix.o:
	cd .. && make

$(TARGET): ../smatrix.o ../smatrix_ruby.c ../smatrix_ruby.h
	$(CC) -I$(RUBY_INCLUDE) $(LIBFLAGS) $(LDFLAGS) ../smatrix_ruby.c ../smatrix.o -o $(TARGET)

install: $(TARGET)
