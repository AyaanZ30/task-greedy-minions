## Race conditon
When 2 threads try to access the same memory location at the same time w/o any synchronization (atleast one of the accesses being W[write]).

## Steps to build + run the app executable (from project root [greedy-minions/]):

1) rm -rf build (everytime you open the project AND need a fresh MinGW build)

2) cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXE_LINKER_FLAGS="-static" -DJOBSYS_ENABLE_TSAN=ON (optional for casual runs)

3) cmake --build build

4) Run with GNU debugger [gdb ./build/apps/job_scheduler.exe] (if error persists)      OR  ./build/apps/job_scheduler.exe


## Steps to test a functional component (from project root [greedy-minions/]):

1) Ensure you have a properly configured build directory in the root [check steps to build above]

3) cmake --build build

4) Run with GNU debugger [gdb ./build/tests/test_deque.exe] (if error persists)      OR  ./build/tests/test_deque.exe 

5) Testing with ctest => cd build/ && ctest --output-on-failure 

