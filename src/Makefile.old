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

TARGET       = smatrix_example
TARGET_DBG   = smatrix_example_dbg
SOURCES      = smatrix_example.c smatrix.c
HEADERS      = smatrix.h
OBJECTS      = $(SOURCES:.c=.o)

PREFIX       = $(DESTDIR)/usr/local
BINDIR       = $(PREFIX)/bin

all: $(TARGET)
debug: $(TARGET_DBG)

clean:
	rm smatrix_example smatrix_example_dbg *.o

$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) $(CFLAGS) $(RELEASEFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

$(TARGET_DBG): $(OBJECTS)
	$(CC) $(FLAGS) $(CFLAGS) $(DEBUGFLAGS) -o $(TARGET_DBG) $(OBJECTS) $(LDFLAGS)
