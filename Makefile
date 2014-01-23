# This file is part of the "libsmatrix" project
#   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

SHELL        = /bin/sh
CC           = clang
CFLAGS_      = $(CFLAGS) -Wall -Wextra -O3 -march=native -mtune=native -D NDEBUG -fPIC
LDFLAGS      = -lpthread -lm -lruby
PREFIX       = $(DESTDIR)/usr/local
LIBDIR       = $(PREFIX)/lib
UNAME        = $(shell uname)
SOURCES      = src/smatrix.c src/smatrix_jni.c src/smatrix_ruby.c

ifeq ($(UNAME), Darwin)
LIBEXT       = dylib
endif
ifeq ($(UNAME), Linux)
LIBEXT       = so
endif

all: src/smatrix.c src/smatrix.h src/smatrix_jni.h src/config.h
ifeq ($(UNAME), Linux)
	$(CC) $(CFLAGS_) -shared -o src/libsmatrix.so $(RUBY_INCLUDE) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux $(SOURCES) $(LDFLAGS)
endif
ifeq ($(UNAME), Darwin)
	$(CC) $(CFLAGS_) -dynamiclib -o src/libsmatrix.dylib $(RUBY_INCLUDE) -I /System/Library/Frameworks/JavaVM.framework/Headers -I /System/Library/Frameworks/JavaVM.framework/Versions/CurrentJDK/Headers $(SOURCES) $(LDFLAGS)
endif

install: src/libsmatrix.$(LIBEXT)
	cp src/libsmatrix.$(LIBEXT) $(LIBDIR)

src/config.h:
	touch src/config.h

pom.xml:
	ln -s build/maven/pom.xml

src/smatrix_jni.h:
	javac java/com/paulasmuth/libsmatrix/SparseMatrix.java
	javah -o smatrix_jni.h -classpath ./java com.paulasmuth.libsmatrix.SparseMatrix

clean:
	find . -name "*.o" -o -name "*.class" -o -name "*.so" -o -name "*.dylib" -delete
	rm -rf target pom.xml src/config.h src/smatrix_benchmark

benchmark: src/smatrix_benchmark
	src/smatrix_benchmark full

src/smatrix_benchmark: src/smatrix_benchmark.c
	$(CC) $(CFLAGS_) -o src/smatrix_benchmark src/smatrix_benchmark.c src/smatrix.c $(LDFLAGS)

test: clean all test_java

test_java:
	javac -classpath ./src:./src/java src/java/test/TestSparseMatrix.java
	java -Djava.library.path=./src -classpath ./src:./src/java:./src/java/test TestSparseMatrix

build_maven:
	mvn package

publish_maven: build_maven
	mvn deploy
