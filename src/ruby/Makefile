TARGET = libsmatrix.so

include Makefile.in

all: $(TARGET)

../smatrix.o:
	cd .. && make

$(TARGET): ../smatrix.o
	echo $(RUBY_INCLUDE)
	echo "FNORD"

install: $(TARGET)
