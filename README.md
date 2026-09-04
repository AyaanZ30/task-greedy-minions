## Race conditon
When 2 threads try to access the same memory location at the same time w/o any synchronization (atleast one of the accesses being W[write]).

## Steps to build + run the app executable (from project root [greedy-minions/]):

1) rm -rf build (everytime you open the project AND need a fresh MinGW build)

2) cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXE_LINKER_FLAGS="-static" 

For max hardware speed, use:

[Build type : Release (stripped down to raw machine code, no debugging symbols, assertions disables, heavily re-written loops)]

2) cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release 
-DCMAKE_CXX_FLAGS="-march=native -ffast-math" -DCMAKE_EXE_LINKER_FLAGS="-static -static-libgcc -static-libstdc++"

3) cmake --build build --clean-first

4) ./build/apps/scheduler.exe       
   [gdb ./build/apps/scheduler.exe <= if error persists (run with GNU Debugger)] 


## Steps to test a functional component (from project root [greedy-minions/]):

1) Ensure you have a properly configured build directory in the root [check steps to build above]

3) cmake --build build

4) Run with GNU debugger [gdb ./build/tests/test_deque.exe] (if error persists)      OR  ./build/tests/test_deque.exe 

5) Testing with ctest => cd build/ && ctest --output-on-failure 

