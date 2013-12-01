# This file is part of the "libsmatrix" project
#   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

SHELL        = /bin/sh
CC           = clang
CFLAGS_      = $(CFLAGS) -Wall -Wextra -O3 -march=native -mtune=native -D NDEBUG
LDFLAGS      = -lpthread -lm
PREFIX       = $(DESTDIR)/usr/local
LIBDIR       = $(PREFIX)/lib
UNAME        = $(shell uname)
SOURCES      = smatrix.c smatrix_jni.c

clean:
	find . -name "*.o" -o -name "*.class" -o -name "*.so" -o -name "*.dylib" -exec rm {} \;

smatrix_jni.h:
	javac java/com/paulasmuth/libsmatrix/SparseMatrix.java
	javah -o smatrix_jni.h -classpath ./java com.paulasmuth.libsmatrix.SparseMatrix

all: smatrix_jni.h
ifeq ($(UNAME), Linux)
	$(CC) $(CFLAGS_) -shared -o libsmatrix.so -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux $(SOURCES) $(LDFLAGS)
endif
ifeq ($(UNAME), Darwin)
	$(CC) $(CFLAGS_) -dynamiclib -o libsmatrix.dylib -I /System/Library/Frameworks/JavaVM.framework/Headers -I /System/Library/Frameworks/JavaVM.framework/Versions/CurrentJDK/Headers $(SOURCES) $(LDFLAGS)
endif

install:
ifeq ($(UNAME), Linux)
	cp libsmatrix.so $(LIBDIR)
endif
ifeq ($(UNAME), Darwin)
	cp libsmatrix.dylib $(LIBDIR)
endif
