# This file is part of the "smatrix_example" project
#   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

SHELL        = /bin/sh
CC           = clang
FLAGS        =
CFLAGS       = -Wall -Wextra
LDFLAGS      = -lpthread -lm
DEBUGFLAGS   = -g3 -O0 -D _DEBUG
RELEASEFLAGS = -O3 -march=native -mtune=native -D NDEBUG
PREFIX       = $(DESTDIR)/usr/local
BINDIR       = $(PREFIX)/bin

all:
	echo "hello world"

clean:
	find . -name "*.o" -o -name "*.class" -exec rm {} \;
	rm smatrix_example

java:
	javac com/paulasmuth/libsmatrix/SparseMatrix.java
	javah -o smatrix_jni.h com.paulasmuth.libsmatrix.SparseMatrix

smatrix_example: $(OBJECTS)
	$(CC) $(FLAGS) $(CFLAGS) $(RELEASEFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
