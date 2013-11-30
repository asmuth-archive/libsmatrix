# This file is part of the "libsmatrix" project
#   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

SHELL        = /bin/sh
CC           = clang
CFLAGS       = -Wall -Wextra -O3 -march=native -mtune=native -D NDEBUG
LDFLAGS      = -lpthread -lm
DEBUGFLAGS   = -g3 -O0 -D _DEBUG
PREFIX       = $(DESTDIR)/usr/local
BINDIR       = $(PREFIX)/bin

all: libsmatrix.so

clean:
	find . -name "*.o" -o -name "*.class" -o -name "*.so" -exec rm {} \;

smatrix_jni.h:
	javac java/com/paulasmuth/libsmatrix/SparseMatrix.java
	javah -o smatrix_jni.h -classpath ./java com.paulasmuth.libsmatrix.SparseMatrix

libsmatrix.so: smatrix_jni.h
	$(CC) $(CFLAGS) -shared -o libsmatrix.so -I /System/Library/Frameworks/JavaVM.framework/Headers smatrix.c smatrix_jni.c $(LDFLAGS)

test: clean test_java

test_java: libsmatrix.so
	javac -classpath ./src:./java java/test/Test.java
	java -classpath ./src:./java:./java/test Test
