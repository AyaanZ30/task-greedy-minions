## Race conditon
When 2 threads try to access the same memory location at the same time w/o any synchronization (atleast one of the accesses being W[write]).

## On-the-fly improvements needed:

1] add_successor is explicitly NOT thread-safe

[new dependent tasks should be spawned while running => dynamic graph mutation during execution]


## Steps to build + run the app executable (from project root [greedy-minions/]):

1) rm -rf build (everytime you open the project AND need a fresh MinGW build)
2) cmake -B build -S . -G "MinGW Makefiles"
3) cmake --build build
4) ./build/apps/job_scheduler.exe