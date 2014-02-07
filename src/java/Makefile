include ../Makefile.in

TARGET     = libsmatrix_java.$(LIBEXT)

ifeq ($(UNAME), Darwin)
JNI_FLAGS  = -I /System/Library/Frameworks/JavaVM.framework/Headers -I /System/Library/Frameworks/JavaVM.framework/Versions/CurrentJDK/Headers
endif
ifeq ($(UNAME), Linux)
JNI_FLAGS  = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
endif

all: $(TARGET)

../smatrix.o:
	cd .. && make

$(TARGET): ../smatrix.o ../smatrix_jni.c ../smatrix_jni.h
	$(CC) $(JNI_FLAGS) $(LIBFLAGS) $(LDFLAGS) ../smatrix_jni.c ../smatrix.o -o $(TARGET)
