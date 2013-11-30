/**
 * This file is part of the "libsmatrix" project
 *   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
 *
 * Licensed under the MIT License (the "License"); you may not use this
 * file except in compliance with the License. You may obtain a copy of
 * the License at: http://opensource.org/licenses/MIT
 */
#include "smatrix_jni.h"
#include "smatrix.h"

void throw_exception(JNIEnv* env, const char* error) {
  jclass exception = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
  (*env)->ThrowNew(env, exception, error);
}

void set_ptr(JNIEnv* env, jobject self, void* ptr_) {
  jclass   cls;
  jfieldID fid;
  long     ptr = ptr_;

  cls = (*env)->FindClass(env, "com/paulasmuth/libsmatrix/SparseMatrix");
  fid = (*env)->GetFieldID(env, cls, "ptr", "J");

  (*env)->SetLongField(env, self, fid, ptr);
}

int get_ptr(JNIEnv* env, jobject self, void** ptr) {
  jclass   cls;
  jfieldID fid;
  jlong    ptr_;

  cls  = (*env)->FindClass(env, "com/paulasmuth/libsmatrix/SparseMatrix");
  fid  = (*env)->GetFieldID(env, cls, "ptr", "J");
  ptr_ = (*env)->GetLongField(env, self, fid);

  if (ptr_ > 0) {
    *ptr = (void *) ptr_;
    return 0;
  } else {
    throw_exception(env, "can't find native object. maybe close() was already called");
    return 1;
  }
}


JNIEXPORT void JNICALL Java_com_paulasmuth_libsmatrix_SparseMatrix_init (JNIEnv* env, jobject self, jstring file_) {
  void* ptr;
  char* file = NULL;

  if (file_ != NULL) {
    file = (*env)->GetStringUTFChars(env, file_, 0);
  }

  ptr = smatrix_open(file);

  if (ptr == NULL) {
    throw_exception(env, "smatrix_open() failed");
  } else {
    set_ptr(env, self, ptr);
  }

  if (file != NULL) {
    (*env)->ReleaseStringUTFChars(env, file_, file);
  }
}

JNIEXPORT void JNICALL Java_com_paulasmuth_libsmatrix_SparseMatrix_close (JNIEnv* env, jobject self) {
  void* ptr = NULL;

  if (!get_ptr(env, self, &ptr)) {
    smatrix_close(ptr);
    set_ptr(env, self, NULL);
  }
}

