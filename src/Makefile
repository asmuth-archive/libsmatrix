# This file is part of the "libsmatrix" project
#   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

include Makefile.in

TARGET       = smatrix.$(LIBEXT)

all: $(TARGET)

$(TARGET): smatrix.o
	$(CC) $(LIBFLAGS) $(LDFLAGS) smatrix.o -o $(TARGET)

smatrix.o: config.h smatrix_jni.h smatrix.c smatrix.h smatrix_private.h
	$(CC) -c $(CFLAGS) smatrix.c -o smatrix.o

config.h:
	touch config.h

smatrix_jni.h:
	javac java/com/paulasmuth/libsmatrix/SparseMatrix.java
	javah -o smatrix_jni.h -classpath ./java com.paulasmuth.libsmatrix.SparseMatrix
