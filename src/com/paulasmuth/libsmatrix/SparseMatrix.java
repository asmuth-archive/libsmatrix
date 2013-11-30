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
  private long ptr;

  /**
   * Create a new sparse matrix.
   *
   * If file_path is null the matrix will be stored completely in main memory.
   *
   * If file_path is not null, the matrix will be persisted to disk and only a
   * cache of the data is kept in main memory (the cache size can be adjusted
   * with setCacheSize). If the file pointed to by filename exists it will be
   * opened, otherwise a new file will be created.
   *
   * @param file_path path to the file or null
   * @return a new SparseMatrix
   */
  public SparseMatrix(String file_path) {
    System.loadLibrary("libsmatrix");

    if (file_path == null) {
      init_memonly();
    } else {
      init_file(file_path);
    }
  }

  /**
   * Create a new sparse matrix and load the native shared library from a
   * specified source path.
   *
   * If file_path is null the matrix will be stored completely in main memory.
   *
   * If file_path is not null, the matrix will be persisted to disk and only a
   * cache of the data is kept in main memory (the cache size can be adjusted
   * with setCacheSize). If the file pointed to by filename exists it will be
   * opened, otherwise a new file will be created.
   *
   * library_path is the path to the libsmatrix.so file.
   *
   * @param file_path path to the file or null
   * @param library_path path to the libsmatrix.so file
   * @return a new SparseMatrix
   */
  public SparseMatrix(String file_path, String library_path) {
    File libfile = new File(library_path);

    if (libfile.exists()) {
      System.load(libfile.getAbsolutePath());
    } else {
      System.loadLibrary("libsmatrix");
    }

    if (file_path == null) {
      init_memonly();
    } else {
      init_file(file_path);
    }
  }

  /**
   * Closes this smatrix. Calling any other method on an instance after it
   * was closed will throw an exception.
   */
  public native void close();

  private native void init_memonly();
  private native void init_file(String file_path);

}
