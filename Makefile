# This file is part of the "libsmatrix" project
#   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

include src/Makefile.in

SHELL        = /bin/sh
CC           = clang
CFLAGS_      = $(CFLAGS) -Wall -Wextra -O3 -march=native -mtune=native -D NDEBUG -fPIC
LDFLAGS      = -lpthread -lm -lruby
PREFIX       = $(DESTDIR)/usr/local
LIBDIR       = $(PREFIX)/lib
UNAME        = $(shell uname)
SOURCES      = src/smatrix.c src/smatrix_jni.c src/smatrix_ruby.c

all: src/libsmatrix.$(LIBEXT)

src/libsmatrix.$(LIBEXT):
	cd src && make

install:
	cp src/libsmatrix.$(LIBEXT) $(LIBDIR)

clean:
	find . -name "*.o" -o -name "*.class" -o -name "*.so" -o -name "*.dylib" -o -name "*.bundle" | xargs rm
	rm -rf src/java/target src/config.h src/smatrix_benchmark *.gem

benchmark: src/smatrix_benchmark
	src/smatrix_benchmark full

src/smatrix_benchmark:
	cd src && make smatrix_benchmark

test: clean all test_java

test_java:
	javac -classpath ./src:./src/java src/java/test/TestSparseMatrix.java
	java -Djava.library.path=./src -classpath ./src:./src/java:./src/java/test TestSparseMatrix
