# Greedy Minions

Greedy Minions is a compact C++ project for exploring work-stealing task scheduling and dependency-driven parallel execution. The code builds a task graph, schedules tasks across worker threads, and benchmarks parallel speedups on compute-heavy workloads such as ray tracing, Mandelbrot rendering, n-body simulation, and transcendental computation.

## Project overview

This repository is organized around a small runtime that models a task scheduler:

- `JobSystem` creates and manages worker threads.
- `Worker` owns a local task queue and attempts to steal tasks from peers when idle.
- `Task` is a function-based unit of work with dependency metadata.
- `Task::add_successors()` wires a dependency graph so downstream tasks are only released once predecessors complete.
- `wait_all()` blocks until all outstanding tasks have finished.

The scheduler is deliberately educational but still concrete: the runtime is built around real thread coordination, task DAG scheduling, and benchmark-style validation.

## Repository structure

- `src/` — scheduler implementation (`JobSystem`, `Worker`, task completion flow)
- `include/jobsys/` — public task scheduler interfaces and queue abstractions
- `benchmarks/` — benchmark harnesses and task generators
- `apps/` — runnable scheduler executable
- `tests/` — doctests validating deque behavior and race safety expectations
- `concepts/` — exploratory prototypes and experiments
- `third_party/` — vendored `doctest.h`
- `CMakeLists.txt` — top-level build configuration
- `LICENSE` — project license

## Core scheduler design

### Task graph model

A `Task` stores:

- a callable `fn` (`std::function<void()>`)
- successor pointers
- a count of unfinished predecessors
- failure and skip flags used to propagate cancellation along dependency chains

When a task finishes, `JobSystem::on_task_finished()` decrements the predecessor count of each successor. As soon as a successor's remaining predecessor count reaches zero, it is submitted for execution.

### Worker runtime

Each worker owns a local queue and repeatedly:

1. pops work from its local queue,
2. steals from another worker if it is idle,
3. executes the task body, and
4. updates successor readiness.

This is a classic work-stealing pattern intended to minimize idle time and improve throughput on irregular workloads.

### Queue implementations

The project includes two queue variants with different roles:

- `include/jobsys/lockfree_deq.hpp` — Chase-Lev style lock-free deque intended for the scheduler runtime.
- `include/jobsys/work_stealing_deq.hpp` — mutex-protected deque for correctness testing and queue behavior validation.

The active runtime in `Worker` uses `LockFreeDeque`; the `WorkStealingDeque` implementation is also exercised in `tests/test_deque.cpp` to validate partitioning, stealing, and duplicate-elimination guarantees.

## Benchmarks included

The benchmark layer is built around a common `Benchmark` interface and evaluates the scheduler on CPU-heavy workloads:

- `RaytraceBenchmark` — ray-sphere rendering across rows
- `MandelbrotBenchmark` — pixel-wise Mandelbrot iteration
- `NBodyBenchmark` — single-step gravitational simulation
- `TranscedentalBenchmark` — element-wise transcendental math

The app currently runs the ray-trace benchmark by default from `apps/main.cpp`, while the other benchmark classes are available and can be enabled by uncommenting them.

## Build and run

### Prerequisites

- CMake 4.0+ (the project requests `cmake_minimum_required(VERSION 4.0.0)`)
- C++20-capable compiler (GCC, Clang, or MinGW)
- Make or Ninja via CMake

### Configure and build

From the project root:

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --clean-first
```

For a release build tuned for the local machine:

```bash
cmake -S . -B build -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-march=native -ffast-math" \
  -DCMAKE_EXE_LINKER_FLAGS="-static -static-libgcc -static-libstdc++"
cmake --build build --clean-first
```

### Run the scheduler app

```bash
./build/apps/scheduler.exe
```

If needed for debugging:

```bash
gdb ./build/apps/scheduler.exe
```

### Run tests

```bash
ctest --test-dir build --output-on-failure
```

Or, to run the doctest binary directly:

```bash
./build/tests/test_deque.exe
```

## What the current app does

`apps/main.cpp` computes the machine's hardware thread count, chooses that many workers, and runs benchmark tasks through the scheduler. The benchmark output reports:

- serial runtime
- parallel runtime
- speedup ratio

The current default configuration executes a ray-tracing workload and prints timing information for each benchmark pass.

## Notes on correctness and design philosophy

This repository is not a production scheduler; it is a research-and-learning project focused on:

- dependency-aware task scheduling,
- work stealing under contention,
- deterministic queue behavior,
- parallel benchmark measurement.

The comments in the source note several tradeoffs around correctness, memory ordering, and task lifetime. If you are reading the code as a learning exercise, the most important implementation points are the dependency tracking in `Task`, the scheduler lifecycle in `JobSystem`, and the queue semantics in `LockFreeDeque`.

## License

This project is distributed under the MIT license. See `LICENSE` for details.

