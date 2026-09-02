## Race conditon
When 2 threads try to access the same memory location at the same time w/o any synchronization (atleast one of the accesses being W[write]).

## On-the-fly improvements needed:

1] add_successor is explicitly NOT thread-safe

[new dependent tasks should be spawned while running => dynamic graph mutation during execution]