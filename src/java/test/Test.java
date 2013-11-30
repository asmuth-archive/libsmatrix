import com.paulasmuth.libsmatrix.SparseMatrix;

class Test {

  public static void main(String[] opts) {
    SparseMatrix.setLibraryPath("libsmatrix.so");

    SparseMatrix smx1 = new SparseMatrix();
    smx1.close();
    SparseMatrix smx2 = new SparseMatrix("/tmp/fnord.smx");
    smx2.close();
  }

}
