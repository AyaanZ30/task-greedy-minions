#pragma once

#include <vector>
#include <memory>
#include <cmath>
#include "jobsys/task.hpp"

// Deliberately compute-heavy, memory-light: each element does a fixed
// number of transcendental-function iterations (sin/cos/sqrt), touching
// very little memory per unit of CPU work. This isolates scheduler/
// threading overhead from memory-bandwidth effects — unlike the blur
// task, this should scale close to linearly with physical core count
// if the scheduler itself is efficient.

inline std::unique_ptr<jobsys::Task> create_compute_task(
    const std::vector<double>& input,
    std::vector<double>& output,
    size_t start, size_t end,
    int iterations_per_element)
{
    return std::make_unique<jobsys::Task>(
        [&input, &output, start, end, iterations_per_element]() {
            for (size_t i = start; i < end; ++i) {
                double x = input[i];
                for (int k = 0; k < iterations_per_element; ++k) {
                    x = std::sin(x) * std::cos(x) + std::sqrt(std::abs(x) + 1.0);
                }
                output[i] = x;
            }
        });
}

inline void compute_serial(
    const std::vector<double>& input,
    std::vector<double>& output,
    int iterations_per_element)
{
    for (size_t i = 0; i < input.size(); ++i) {
        double x = input[i];
        for (int k = 0; k < iterations_per_element; ++k) {
            x = std::sin(x) * std::cos(x) + std::sqrt(std::abs(x) + 1.0);
        }
        output[i] = x;
    }
}



/*main() for benchmarking for compute task*/

// int main()
// {
//     const unsigned int cores = std::thread::hardware_concurrency();
//     const unsigned int num_workers = (cores > 0) ? cores : 4;

//     constexpr size_t N = 10000; 
//     constexpr int ITERATIONS = 10000;

//     std::vector<double> input(N);
//     for (int i = 0; i < N; ++i) input[i] = static_cast<double>(i % 1000) * 0.001;

//     std::vector<double> out_serial(N, 0.0);
//     std::vector<double> out_parallel(N, 0.0);

//     double serial_time = serial_execution(compute_serial, input, out_serial, ITERATIONS);
//     std::cout << "Serial time : " << std::fixed << std::setprecision(3) << serial_time << " sec\n";
    
//     const int num_chunks = num_workers * 4;
//     int chunk_size = (N / num_chunks);      

//     /*stores task objects in memory throughout execution lifecycle*/
//     std::vector<std::unique_ptr<Task>> tasks;
//     std::vector<Task*> filter_tasks;     // stores ptr to 4 filter tasks (void fn's => filter even numbers from the specific chunk)

//     /*
//     define the aggregator task => basically returns the final result (aggregation of worker executed tasks) => 4 * result(filter-task)
//     Modify aggregate when => running a different job (with it's own aggregator reqs)
//     */
//     auto aggregate = std::make_unique<Task>([]() {
        
//     });

//     // Task *aggregate_ptr = aggregate.get();   non-owning reference to actual owner (unique_ptr)

//     for(int c = 0 ; c < num_chunks ; ++c){
//         int start = (c * chunk_size);
//         int end = (c == num_chunks - 1) ? N : (start + chunk_size);
        
//         /*lambda filter fn(void()) => basically stored in each workers deque as a task to be popped and executed*/
//         auto filter = create_compute_task(input, out_parallel, start, end, ITERATIONS);

//         /*
//         once filter finished => decrement aggregator's predecessor count [as aggregator is the final step of the job]
//         if predecessors(aggregator) = 0 {no filter task pending} : aggregator is now runnable (ready to be executed)
//         Hence => successor(filter) = aggregator || predecessor(aggregator) = filter 
//         */
//         filter->add_successors(aggregate.get());
//         filter_tasks.push_back(filter.get());

//         /*racks up lambda filter fn in the deque's of worker threads as tasks (to be executed / stolen)*/
//         tasks.push_back(std::move(filter));
//     }

//     tasks.push_back(std::move(aggregate));

//     double parallel_time = 0.0;
//     {
//         auto p0 = std::chrono::steady_clock::now();
//         {
//             JobSystem system(num_workers);
//             for(Task* t : filter_tasks){
//                 system.submit(t);
//             }

//             system.wait_all();
//             // JobSystem destructor executed here (sets shutdown_flag_, joins threads)
//             // Safe because wait_all() already guaranteed no work is outstanding.
//         }
//         auto p1 = std::chrono::steady_clock::now();
//         parallel_time = std::chrono::duration<double>(p1 - p0).count();
//     }   
//     std::cout << "Parallel Time : " << parallel_time << " sec (" << num_workers << " workers)\n";
//     std::cout << "Speedup       : " << (serial_time / parallel_time) << "x\n";

//     return 0;
// } 
