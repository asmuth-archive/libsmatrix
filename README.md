libsmatrix
==========

a sparse matrix data structure. we use this to run a recommendation engine with roughly
10 million recommendable items and hundreds of million of input data points. it features
two modes of operation:

+ a memory-only mode in which all data is kept in main memory, we use this for matrices
with up to ~40GB in size and it is lightning fast.

+ a hybrid memory/disk mode in which only a pool of recently/frequently used records is
kept in memory. use this to handle datasets larger than your available main memory. in
this mode, the data is also persisted across restarts.


API
---

Open a smatrix (if filename is NULL, use in memory only mode, otherwise create file)

    smatrix_t* smatrix_open(const char* fname);

Close a smatrix:
    
    void smatrix_close(smatrix_t* self);

Get, Set, Increment, Decrement a (x,y) position


    uint32_t smatrix_get(smatrix_t* self, uint32_t x, uint32_t y);
    uint32_t smatrix_set(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);
    uint32_t smatrix_incr(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);
    uint32_t smatrix_decr(smatrix_t* self, uint32_t x, uint32_t y, uint32_t value);

Get a whole "row" of the matrix by row coordinate x

    uint32_t smatrix_rowlen(smatrix_t* self, uint32_t x);
    uint32_t smatrix_getrow(smatrix_t* self, uint32_t x, uint32_t* ret, size_t ret_len);


Example
-------

There is a simple example in src/smatrix_example.c


License
-------

Copyright (c) 2011 Paul Asmuth

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to use, copy and modify copies of the Software, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
