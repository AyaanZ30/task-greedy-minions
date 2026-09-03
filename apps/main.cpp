#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include <iomanip>

#include "../include/jobsys/job_system.hpp"
#include "../include/jobsys/task.hpp"

/*
We first create a static-dependencies task graph (create Task objects and wire up dependencies (add_successor) before executing anything at all)

Job : [Filter out all the even numbers from 0-40] (first 40 numbers) 
Split 40 integers => 4 chunks x 10 chunk_size => 4 filter tasks (1 filter task per-chunk)

The vector holding Task objects must stay alive UNTIL every task has finished executing (to handle memory & prevent dangling pointers[if task's memory is lost]])
[unique_ptr<Task* t> => owns the Task* t's memory]

wait_all() : blocks main thread (orchestrator) until workers (executors) finish executing all tasks 
*/
using namespace jobsys;

int main()
{
    const unsigned int cores = std::thread::hardware_concurrency();
    const unsigned int num_workers = cores;

    constexpr int data_size = 100000000;
    std::vector<int> data(data_size, 0);
    for(int i = 0 ; i < data_size ; i++) data[i] = i;

    const int num_chunks = num_workers * 8;
    int chunk_size = (data_size / num_chunks);      // 4 chunks (10 integers per-chunk)

    /*stores task objects in memory throughout execution lifecycle*/
    std::vector<std::unique_ptr<Task>> tasks;
     
    std::vector<int> results;         // stores global result (post-aggregating each filter tasks result)
    std::mutex results_mtx;           // to ensure no race-cond occur when different workers concurrently WRITE to results

    /*
    define the aggregator task => basically returns the final result (aggregation of worker executed tasks) => 4 * result(filter-task)
    pass &results, &results_mtx (by reference) : AS we want these to be shared among all workers performing their own tasks 
    */
    auto aggregate = std::make_unique<Task>([&results, &results_mtx, num_workers]() {
        std::lock_guard<std::mutex> lock(results_mtx);
        // std::cout << "[aggregator] collected " << results.size() << " even numbers (using " << num_workers << " workers)\n";
    });

    // non-owning reference to actual owner (unique_ptr)
    // Task *aggregate_ptr = aggregate.get();     

    std::vector<Task*> filter_tasks;     // stores ptr to 4 filter tasks (void fn's => filter even numbers from the specific chunk)
    
    for(int c = 0 ; c < num_chunks ; ++c){
        int start = (c * chunk_size);
        int end = (c == num_chunks - 1) ? data_size : (start + chunk_size);
        
        /*lambda filter fn(void()) => basically stored in each workers deque as a task to be popped and executed*/
        auto filter = std::make_unique<Task>([&data, &results, &results_mtx, start, end, c]() {
            std::vector<int> local_evens;
            for(int i = start ; i < end ; ++i){
                if(data[i] % 2 == 0) local_evens.push_back(data[i]);
            }
            
            // avoid possible race due to concurrent thread-bound write operations (each worker persistently finding local_evens for its chunk and writing it back to results in parallel)
            std::lock_guard<std::mutex> safe_write_lock(results_mtx);
            results.insert(results.end(), local_evens.begin(), local_evens.end());
            // std::cout << "[filter " << c << "] processed range [" << start << ", " << end << ")\n";
        });

        /*
        once filter finished => decrement aggregator's predecessor count [as aggregator is the final step of the job]
        if predecessors(aggregator) = 0 {no filter task pending} : aggregator is now runnable (ready to be executed)
        Hence => successor(filter) = aggregator || predecessor(aggregator) = filter 
        */
        filter->add_successors(aggregate.get());
        filter_tasks.push_back(filter.get());

        /*racks up lambda filter fn in the deque's of worker threads as tasks (to be executed / stolen)*/
        tasks.push_back(std::move(filter));
    }

    tasks.push_back(std::move(aggregate));
    {
        auto t0 = std::chrono::steady_clock::now();
        {
            JobSystem system(num_workers);
            for(Task* t : filter_tasks){
                system.submit(t);
            }

            system.wait_all();
            // JobSystem destructor executed here (sets shutdown_flag_, joins threads)
            // Safe because wait_all() already guaranteed no work is outstanding.
        }
        auto t1 = std::chrono::steady_clock::now();

        auto elapsed = std::chrono::duration<double>(t1 - t0).count();
        std::cout << "Elapsed: " << std::fixed << std::setprecision(3) << elapsed << " sec\n";
    }   

    std::cout << "Job done (Total evens found : " << results.size() << ")\n";
    return 0;
} 
