/**
 * This file is part of the "libsmatrix" project
 *   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
 *
 * Licensed under the MIT License (the "License"); you may not use this
 * file except in compliance with the License. You may obtain a copy of
 * the License at: http://opensource.org/licenses/MIT
 */
import java.util.LinkedList;
import java.util.Iterator;
import com.paulasmuth.libsmatrix.SparseMatrix;

interface TestCase {
  public String getName();
  public boolean run(SparseMatrix smx);
}


class Test {
  static LinkedList<TestCase> testCases = new LinkedList<TestCase>();

  static {

    testCases.add(new TestCase() {
      public String getName() {
        return "fnord";
      }
      public boolean run(SparseMatrix smx) {
        System.out.println("run test");
        return true;
      }
    });

  }

  public static void main(String[] opts) {
    SparseMatrix.setLibraryPath("libsmatrix.so");

    SparseMatrix smx1 = new SparseMatrix();
    run_tests(smx1);
    smx1.close();

    SparseMatrix smx2 = new SparseMatrix("/tmp/fnord.smx");
    run_tests(smx2);
    smx2.close();
  }

  public static void run_tests(SparseMatrix smx) {
    Iterator iter = testCases.iterator();

    while (iter.hasNext()) {
      TestCase testcase = (TestCase) iter.next();

      if (testcase.run(smx)) {
        System.out.println("[SUCCESS] " + testcase.getName());
      } else {
        System.out.println("[FAILED] " + testcase.getName());
      }
    }
  }

}
