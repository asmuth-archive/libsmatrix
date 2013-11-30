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

#define _JM(X) Java_com_paulasmuth_libsmatrix_SparseMatrix_##X
#define ERR_PTRNOTFOUND "can't find native object. maybe close() was already called"

void throw_exception(JNIEnv* env, const char* error) {
  jclass exception = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
  (*env)->ThrowNew(env, exception, error);
}

void set_ptr(JNIEnv* env, jobject self, void* ptr_) {
  jclass   cls;
  jfieldID fid;
  long     ptr = (long) ptr_;

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
    throw_exception(env, ERR_PTRNOTFOUND);

    return 1;
  }
}

JNIEXPORT void JNICALL _JM(init) (JNIEnv* env, jobject self, jstring file_) {
  void* ptr;
  char* file = NULL;

  if (file_ != NULL) {
    file = (char *) (*env)->GetStringUTFChars(env, file_, 0);
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

JNIEXPORT void JNICALL _JM(close) (JNIEnv* env, jobject self) {
  void* ptr = NULL;

  if (!get_ptr(env, self, &ptr)) {
    smatrix_close(ptr);
    set_ptr(env, self, NULL);
  }
}

JNIEXPORT jint _JM(get) (JNIEnv* env, jobject self, jint x, jint y) {
  void* ptr = NULL;

  if (get_ptr(env, self, &ptr)) {
    return 0;
  } else {
    return (jint) smatrix_get(ptr, x, y);
  }
}

JNIEXPORT void JNICALL _JM(set) (JNIEnv* env, jobject self, jint x, jint y, jint v) {
  void* ptr = NULL;

  if (!get_ptr(env, self, &ptr)) {
    smatrix_set(ptr, (uint32_t) x, (uint32_t) y, (uint32_t) v);
  }
}

JNIEXPORT void JNICALL _JM(incr) (JNIEnv* env, jobject self, jint x, jint y, jint v) {
  void* ptr = NULL;

  if (!get_ptr(env, self, &ptr)) {
    smatrix_incr(ptr, (uint32_t) x, (uint32_t) y, (uint32_t) v);
  }
}

JNIEXPORT void JNICALL _JM(decr) (JNIEnv* env, jobject self, jint x, jint y, jint v) {
  void* ptr = NULL;

  if (!get_ptr(env, self, &ptr)) {
    smatrix_decr(ptr, (uint32_t) x, (uint32_t) y, (uint32_t) v);
  }
}

JNIEXPORT void JNICALL _JM(getRowNative) (JNIEnv* env, jobject self, jint x, jobject map, jint maxlen) {
  jclass    cls;
  jmethodID mid;
  uint32_t  len, *data, i;
  void*     ptr = NULL;
  size_t    bytes;

  cls = (*env)->GetObjectClass(env, map);
  mid = (*env)->GetMethodID(env, cls, "putIntTuple", "(II)V");

  if (mid == NULL) {
    return;
  }

  if (get_ptr(env, self, &ptr)) {
    return;
  }

  len   = smatrix_rowlen(ptr, x);
  bytes = len * 8;
  data  = malloc(bytes);

  if (data == NULL) {
    throw_exception(env, "malloc() failed");
    return;
  }

  len = smatrix_getrow(ptr, (uint32_t) x, data, bytes);

  for (i = 0; i < len; i++) {
    if (maxlen > 0 && i >= maxlen) {
      break;
    }

    (*env)->CallVoidMethod(env, map, mid, data[i * 2], data[i * 2 + 1]);
  }

  free(data);
}

JNIEXPORT jint JNICALL _JM(getRowLength) (JNIEnv* env, jobject self, jint x) {
  void* ptr = NULL;

  if (get_ptr(env, self, &ptr)) {
    return 0;
  } else {
    return (jint) smatrix_rowlen(ptr, x);
  }
}

