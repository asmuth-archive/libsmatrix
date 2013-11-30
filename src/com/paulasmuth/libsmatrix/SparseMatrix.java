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

/**
 * A libsmatrix sparse matrix
 */
public class SparseMatrix {

  /**
   * Create a new in-memory only Sparse Matrix
   *
   * @return a new SparseMatrix
   */
  public SparseMatrix() {
    System.out.println("in-memory mode");
    load();
    init();
  }

  /**
   * Create a new persistent disk/memory Sparse Matrix. This will create
   * the file if it doesn't exist yet and open it if it already exists.
   *
   * @param filename path to the file
   * @return a new SparseMatrix
   */
  public SparseMatrix(String filename) {
    System.out.println("hybrid mode");
    load();
  }

  private native void init();
  public  native void close();

  private void load() {
    File libfile = new File("libsmatrix.so");

    if (libfile.exists()) {
      System.load(libfile.getAbsolutePath());
    } else {
      System.loadLibrary("libsmatrix");
    }
  }

}
