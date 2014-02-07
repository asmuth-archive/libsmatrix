include ../Makefile.in
include Makefile.in

TARGET = libsmatrix_ruby.$(LIBEXT)

all: $(TARGET)

../smatrix.o:
	cd .. && make

$(TARGET): ../smatrix.o ../smatrix_ruby.c ../smatrix_ruby.h
	$(CC) -I$(RUBY_INCLUDE) $(LIBFLAGS) $(LDFLAGS) ../smatrix_ruby.c ../smatrix.o -o $(TARGET)

install: $(TARGET)
