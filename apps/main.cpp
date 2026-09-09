#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <utility>
#include <cmath>
#include <mutex>
#include <iomanip>

#include "benchmark.hpp"
#include "benchmarks/mandelbrot_benchmark.hpp"
#include "benchmarks/raytrace_benchmark.hpp"
#include "benchmarks/nbody_benchmark.hpp"

/*
We first create a static-dependencies task graph (create Task objects and wire up dependencies (add_successor) before executing anything at all)

Job : [Filter out all the even numbers from 0-40] (first 40 numbers) 
Split 40 integers => 4 chunks x 10 chunk_size => 4 filter tasks (1 filter task per-chunk)

The vector holding Task objects must stay alive UNTIL every task has finished executing (to handle memory & prevent dangling pointers[if task's memory is lost]])
[unique_ptr<Task* t> => owns the Task* t's memory]

wait_all() : blocks main thread (orchestrator) until workers (executors) finish executing all tasks 
*/

void run_benchmark(Benchmark& b, unsigned int num_workers){
    std::cout << "=== " << b.name() << " ===\n";

    double serial_time = b.run_serial();
    std::cout << "Serial time   : " << std::fixed << std::setprecision(3) << serial_time << " sec\n";

    double parallel_time = b.run_parallel(num_workers);
    std::cout << "Parallel time : " << parallel_time << " sec (" << num_workers << " workers)\n";
    std::cout << "Speedup       : " << (serial_time / parallel_time) << "x\n";
}

int main()
{
    const unsigned int cores = std::thread::hardware_concurrency();
    const unsigned int num_workers = (cores > 0) ? cores : 4;
    
    std::vector<std::unique_ptr<Benchmark>> benchmarks;
    
    // benchmarks.push_back(std::make_unique<MandelbrotBenchmark>(16000, 16000, 50));
    benchmarks.push_back(std::make_unique<RaytraceBenchmark>(10000, 10000));
    // benchmarks.push_back(std::make_unique<NBodyBenchmark>(2000));

    for(auto& b : benchmarks){
        run_benchmark(*b, num_workers);
    }

    return 0;
}  
