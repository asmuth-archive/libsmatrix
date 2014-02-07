# This file is part of the "libsmatrix" project
#   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

SHELL        = /bin/sh
CC           = clang
CFLAGS      ?= -c -Wall -Wextra -O3 -march=native -mtune=native -D NDEBUG -fPIC
LDFLAGS      = -lpthread -lm
PREFIX       = $(DESTDIR)/usr/local
LIBDIR       = $(PREFIX)/lib
UNAME        = $(shell uname)

#ifeq ($(UNAME), Darwin)
#CFLAGS_     +=  -dynamic -bundle -o src/libsmatrix.bundle
#INCLUDES     = -I /System/Library/Frameworks/JavaVM.framework/Headers -I /System/Library/Frameworks/JavaVM.framework/Versions/CurrentJDK/Headers
#endif
#ifeq ($(UNAME), Linux)
#CFLAGS_     +=  -shared -o src/libsmatrix.so
#INCLUDES     = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
#endif

all: smatrix.o

smatrix.o: config.h smatrix_jni.h smatrix.c smatrix.h smatrix_private.h
	$(CC) $(CFLAGS) smatrix.c -o smatrix.o

config.h:
	touch src/config.h

smatrix_jni.h:
	javac java/com/paulasmuth/libsmatrix/SparseMatrix.java
	javah -o smatrix_jni.h -classpath ./java com.paulasmuth.libsmatrix.SparseMatrix
