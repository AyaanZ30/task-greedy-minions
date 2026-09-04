#pragma once

#include <atomic>
#include <random>
#include <thread>

#include "work_stealing_deq.hpp"
#include "lockfree_deq.hpp"

/*
A Worker object => one persistent OS worker thread (bound to its own local WorkStealingDeque)
Each worker has its own set of deque operations (below code acting as blueprint)

[t : task/smallest unit of execution]
*/

namespace jobsys{

    /* 
    JobSystem => vector<Workers> OR vector<unique_ptr<Worker>> 
    
    All workers are initialized first (via the constructor) with unique ID's

    constructor != start()
    however, constructor DOES NOT get the thread running DUE TO premature race-cond danger
    [premature race-cond : accessing un-init memory OR trying to look at a Worker NOT CONSTRUCTED yet] 

    Hence: vector<Workers> initialized first(all workers) => start by run_loop()
    */
    class JobSystem;
    struct Task;

    class Worker{
        // Worker 1 <=> Deque[t1, t2, t3, ..,] 
        // Worker 2 <=> Deque[t1, t2, t3, ..,] 
        private:
            int id_;
            JobSystem& system_;
            // WorkStealingDeque queue_;
            LockFreeDeque queue_;
            std::thread thread_;
            std::mt19937 rng_;

            void run_loop();
            void execute(Task* task);
        public:
            /*
            id: index into JobSystem's worker list (used to avoid stealing from self)
            system: back-pointer so a worker can see other workers to steal from, report task-completion
            */ 
            Worker(int id, JobSystem& sytem);
            ~Worker();

            Worker(const Worker&) = delete;
            Worker& operator=(const Worker&) = delete;

            // Commence the OS thread before run_loop()
            void start();
            /*
            for the OWNER => push a task to its own WorkStealingDeque
            */
            void push_task(Task* task);

            // Called by OTHER workers to steal from this one.
            Task* try_steal();
            int id() const { return id_; }

    };
}