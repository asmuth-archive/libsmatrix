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

  static { testCases.add(new TestCase() {
    public String getName() {
      return "simple set/get";
    }
    public boolean run(SparseMatrix smx) {
      smx.set(42,23,17);
      return smx.get(42,23) == 17;
    }
  }); }

  static { testCases.add(new TestCase() {
    public String getName() {
      return "simple increment 1";
    }
    public boolean run(SparseMatrix smx) {
      smx.set(4231,2634,0);
      smx.incr(4231,2634,1);
      return smx.get(4231,2634) == 1;
    }
  }); }

  static { testCases.add(new TestCase() {
    public String getName() {
      return "simple increment 5";
    }
    public boolean run(SparseMatrix smx) {
      smx.set(1231,2634,0);
      smx.incr(1231,2634,1);
      smx.incr(1231,2634,5);
      return smx.get(1231,2634) == 6;
    }
  }); }

  static { testCases.add(new TestCase() {
    public String getName() {
      return "1 million increments + 1 million gets";
    }
    public boolean run(SparseMatrix smx) {
      int v = 34;
      int i;
      int n;

      for (n = 0; n < 1000; n++) {
        for (i = 0; i < 1000; i++) {
          smx.set(i, n, v);
        }
      }

      for (n = 0; n < 1000; n++) {
        for (i = 0; i < 1000; i++) {
          if ((smx.get(i, n) != v)) {
            return false;
          }
        }
      }

      return true;
    }
  }); }

  public static void main(String[] opts) {
    boolean success = true;
    SparseMatrix.setLibraryPath("libsmatrix.so");

    SparseMatrix smx1 = new SparseMatrix();
    success &= run_tests(smx1);
    smx1.close();

    SparseMatrix smx2 = new SparseMatrix("/tmp/fnord.smx");
    success &= run_tests(smx2);
    smx2.close();

    if (success) {
      System.out.println("\033[1;32mAll tests finished successfully :)\033[0m");
      System.exit(0);
    } else {
      System.out.println("\033[1;31mTests failed :(\033[0m");
      System.exit(1);
    }
  }

  public static boolean run_tests(SparseMatrix smx) {
    boolean success = true;
    Iterator iter = testCases.iterator();

    while (iter.hasNext()) {
      TestCase testcase = (TestCase) iter.next();

      if (testcase.run(smx)) {
        System.out.println("\033[1;32m[SUCCESS] " + testcase.getName() + "\033[0m");
      } else {
        System.out.println("\033[1;31m[FAILED] "  + testcase.getName() + "\033[0m");
        success = false;
      }
    }

    return success;
  }

}
