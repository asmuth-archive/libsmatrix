/**
 * This file is part of the "libsmatrix" project
 *   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
 *
 * Licensed under the MIT License (the "License"); you may not use this
 * file except in compliance with the License. You may obtain a copy of
 * the License at: http://opensource.org/licenses/MIT
 */
#include "smatrix_jni.h"

JNIEXPORT void JNICALL Java_com_paulasmuth_libsmatrix_SparseMatrix_init (JNIEnv * env, jobject self) {

  printf("smatrix_init :)\n");
}

JNIEXPORT void JNICALL Java_com_paulasmuth_libsmatrix_SparseMatrix_close (JNIEnv * env, jobject self) {
  printf("smatrix_close :)\n");
}
