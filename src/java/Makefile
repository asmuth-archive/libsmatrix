# This file is part of the "libsmatrix" project
#   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

include ../Makefile.in

TARGET     = libsmatrix_java.$(LIBEXT)

ifeq ($(UNAME), Darwin)
JNI_FLAGS  = -I /System/Library/Frameworks/JavaVM.framework/Headers -I /System/Library/Frameworks/JavaVM.framework/Versions/CurrentJDK/Headers
endif
ifeq ($(UNAME), Linux)
JNI_FLAGS  = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
endif

all: $(TARGET)
	mvn package

$(TARGET): ../smatrix.o ../smatrix_jni.c ../smatrix_jni.h
	$(CC) $(JNI_FLAGS) $(LIBFLAGS) $(LDFLAGS) ../smatrix_jni.c ../smatrix.o -o $(TARGET)

../smatrix.o:
	cd .. && make
