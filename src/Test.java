/**
 * This file is part of the "libsmatrix" project
 *   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
 *
 * Licensed under the MIT License (the "License"); you may not use this
 * file except in compliance with the License. You may obtain a copy of
 * the License at: http://opensource.org/licenses/MIT
 */
import com.paulasmuth.libsmatrix.SparseMatrix;

class Test {

  public static void main(String[] opts) {
    SparseMatrix.setLibraryPath("../src/libsmatrix.so");

    SparseMatrix smx1 = new SparseMatrix();
    smx1.close();
    SparseMatrix smx2 = new SparseMatrix("/tmp/fnord.smx");
    smx2.close();
  }

}
