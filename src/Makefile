# This file is part of the "libsmatrix" project
#   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

include Makefile.in

TARGET         = smatrix.$(LIBEXT)
TARGET_STATIC  = smatrix-static.a

all: $(TARGET) $(TARGET_STATIC)

$(TARGET): smatrix.o
	$(CC) $(LIBFLAGS) smatrix.o -o $(TARGET) $(LDFLAGS)

$(TARGET_STATIC): smatrix.o
	ar cr $(TARGET_STATIC) smatrix.o

smatrix.o: config.h smatrix.c smatrix.h smatrix_private.h
	$(CC) -c $(CFLAGS) smatrix.c -o smatrix.o

config.h:
	touch config.h

smatrix_jni.h:
	javac com/paulasmuth/libsmatrix/SparseMatrix.java
	javah -o smatrix_jni.h -classpath . com.paulasmuth.libsmatrix.SparseMatrix

smatrix_benchmark: smatrix.o smatrix_benchmark.c
	$(CC) $(CFLAGS) smatrix_benchmark.c smatrix.o -o smatrix_benchmark $(LDFLAGS)
