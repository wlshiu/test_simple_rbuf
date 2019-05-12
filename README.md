# test_simple_rbuf
---

simple ring buffer

+ it will over write the data when wr_index catchs the rd_index
    > the pop handler MUST be faster than push handler.

+ it may pop no data
    > you should verify the data and length of pop api are 0 or not.

