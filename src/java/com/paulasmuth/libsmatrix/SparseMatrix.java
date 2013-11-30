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
import java.util.SortedMap;
import java.util.concurrent.ConcurrentSkipListMap;

/**
 * A libsmatrix sparse matrix
 */
public class SparseMatrix {
  private static String library_path = "libsmatrix.so";
  private long ptr;

  /**
   * Create a new sparse matrix that will be stored in main memory only.
   *
   * @param file_path path to the file or null
   * @return a new SparseMatrix
   */
  public SparseMatrix() {
    SparseMatrix.loadLibrary();
    init(null);
  }

  /**
   * Create a new sparse matrix that will be persisted to disk. A cache of the
   * data is still kept in main memory (the cache size can be adjusted by calling
   * setCacheSize on the instance). 
   *
   * If the file pointed to by filename exists it will be opened, otherwise a new
   * file will be created.
   *
   * @param file_path path to the file
   * @return a new SparseMatrix
   */
  public SparseMatrix(String file_path) {
    SparseMatrix.loadLibrary();
    init(file_path);
  }

  /**
   * Explcitly set the path to the native shared object (libsmatrix.so).
   *
   * @param the path to the native shared object
   */
  public static void setLibraryPath(String path) {
    library_path = path;
    loadLibrary();
  }

  /**
   * HERE BE DRAGONS
   */
  public native int get(int x, int y);

  /**
   * HERE BE DRAGONS
   */
  public native void set(int x, int y, int val);

  /**
   * HERE BE DRAGONS
   */
  public native void incr(int x, int y, int val);

  /**
   * HERE BE DRAGONS
   */
  public native void decr(int x, int y, int val);

  /**
   * HERE BE DRAGONS
   */
  public native int getRowLength(int x);

  /**
   * HERE BE DRAGONS
   */
  public SortedMap<Integer,Integer> getRow(int x) {
    SortedMap<Integer, Integer> map = new ConcurrentSkipListMap<Integer, Integer>() {
      public void putIntTuple(int k, int v) {
        this.put(k, v);
      }
    };

    getRowNative(x, map);

    return map;
  }

  /**
   * Close this matrix. Calling any other method on the instance after it was
   * closed will throw an exception.
   */
  public native void close();

  /**
   * HERE BE DRAGONS
   */
  private native void getRowNative(int x, SortedMap<Integer,Integer> map);

  /**
   * Load the native shared object (libsmatrix.so)
   */
  private static void loadLibrary() {
    File libfile = new File(library_path);

    if (libfile.exists()) {
      System.load(libfile.getAbsolutePath());
    } else {
      System.loadLibrary("libsmatrix");
    }
  }

  /**
   * Initialize a new native matrix
   */
  private native void init(String file_path);

}
