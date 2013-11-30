/**
 * This file is part of the "libsmatrix" project
 *   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
 *
 * Licensed under the MIT License (the "License"); you may not use this
 * file except in compliance with the License. You may obtain a copy of
 * the License at: http://opensource.org/licenses/MIT
 */
package com.paulasmuth.libsmatrix;
import java.io.File;

public class SparseMatrix {
  public native void test();

  static {
    File libfile = new File("libsmatrix.so");

    if (libfile.exists()) {
      System.load(libfile.getAbsolutePath());
    } else {
      System.loadLibrary("libsmatrix");
    }
  }
}
