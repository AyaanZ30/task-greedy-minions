#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <utility>
#include <cmath>
#include <mutex>
#include <iomanip>

#include "jobsys/job_system.hpp"
#include "jobsys/task.hpp"

// #include "tasks/compute_task.hpp"
// #include "tasks/mandelbrot_task.hpp"
#include "tasks/n_body_gravity_task.hpp"

/*
We first create a static-dependencies task graph (create Task objects and wire up dependencies (add_successor) before executing anything at all)

Job : [Filter out all the even numbers from 0-40] (first 40 numbers) 
Split 40 integers => 4 chunks x 10 chunk_size => 4 filter tasks (1 filter task per-chunk)

The vector holding Task objects must stay alive UNTIL every task has finished executing (to handle memory & prevent dangling pointers[if task's memory is lost]])
[unique_ptr<Task* t> => owns the Task* t's memory]

wait_all() : blocks main thread (orchestrator) until workers (executors) finish executing all tasks 
*/

using namespace jobsys;

template<typename Func, typename... Args>        // takes any task helper func with any number of parameters (dynamic forwarding)
double serial_execution(Func&& func, Args&&... args){
    auto t0 = std::chrono::steady_clock::now();
    std::forward<Func>(func)(std::forward<Args>(args)...);
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(t1 - t0).count();
}

int main()
{
    const unsigned int cores = std::thread::hardware_concurrency();
    const unsigned int num_workers = (cores > 0) ? cores : 4;

   // 1080p Mandelbrot setup
    constexpr size_t N_BODIES = 15000;

    std::vector<Body> bodies(N_BODIES);
    for(size_t i = 0 ; i < N_BODIES ; ++i){
        bodies[i] = {
            static_cast<double>(i % 100), static_cast<double>(i % 50), static_cast<double>(i % 25),
            0.1, 0.2, 0.3,
            1e10 + (i % 100) * 1e8
        };
    }

    std::vector<Body> out_serial(N_BODIES);
    std::vector<Body> out_parallel(N_BODIES);

    double serial_time = serial_execution(gravity_serial, bodies, out_serial, 0.001);
    std::cout << "Serial time : " << std::fixed << std::setprecision(3) << serial_time << " sec\n";
    
    const int num_chunks = num_workers * 8;   
    int chunk_size = (N_BODIES / num_chunks);      

    /*stores task objects in memory throughout execution lifecycle*/
    std::vector<std::unique_ptr<Task>> tasks;
    std::vector<Task*> initial_tasks;     // stores ptr to 4 filter tasks (void fn's => filter even numbers from the specific chunk)

    /*
    define the aggregator task => basically returns the final result (aggregation of worker executed tasks) => 4 * result(filter-task)
    Modify aggregate when => running a different job (with it's own aggregator reqs)
    */
    auto aggregate = std::make_unique<Task>([]() {
        
    });

    // Task *aggregate_ptr = aggregate.get();   non-owning reference to actual owner (unique_ptr)

    for(int c = 0 ; c < num_chunks ; ++c){
        int start = (c * chunk_size);
        int end = (c == num_chunks - 1) ? N_BODIES : (start + chunk_size);
        
        /*lambda filter fn(void()) => basically stored in each workers deque as a task to be popped and executed*/
        auto mb_task = create_gravity_task(bodies, out_parallel, start, end, 0.001);
        /*
        once filter finished => decrement aggregator's predecessor count [as aggregator is the final step of the job]
        if predecessors(aggregator) = 0 {no filter task pending} : aggregator is now runnable (ready to be executed)
        Hence => successor(filter) = aggregator || predecessor(aggregator) = filter 
        */
        mb_task->add_successors(aggregate.get());
        initial_tasks.push_back(mb_task.get());

        /*racks up lambda filter fn in the deque's of worker threads as tasks (to be executed / stolen)*/
        tasks.push_back(std::move(mb_task));
    }

    tasks.push_back(std::move(aggregate));

    double parallel_time = 0.0;
    {
        auto p0 = std::chrono::steady_clock::now();
        {
            JobSystem system(num_workers);
            for(Task* t : initial_tasks){
                system.submit(t);
            }

            system.wait_all();
            // JobSystem destructor executed here (sets shutdown_flag_, joins threads)
            // Safe because wait_all() already guaranteed no work is outstanding.
        }
        auto p1 = std::chrono::steady_clock::now();
        parallel_time = std::chrono::duration<double>(p1 - p0).count();
    }   
    std::cout << "Parallel Time : " << parallel_time << " sec (" << num_workers << " workers)\n";
    std::cout << "Speedup       : " << (serial_time / parallel_time) << "x\n";

    return 0;
} 
