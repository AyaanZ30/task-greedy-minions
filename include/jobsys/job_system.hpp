#pragma once 

#include <atomic>
#include <vector>
#include <memory>

#include "worker.hpp"

namespace jobsys{
    struct Task;

    class JobSystem{
        // friend class Worker;

        public:
            /*
            Spawns 'num_worker' threads. Workers are fully constructed before any of the 
            worker threads actually starts running [Worker() const seperate from Worker::start() rationale]
            */
            explicit JobSystem(int num_workers);

            /*
            a) Signals shutdown by joining all worker threads (aggr result)
            b) PRECONDITION :
                - Caller should have already ensured all the outstanding work is done [via wait_all()]
                - If unfinished work => abandons it [destructor does not wait for completion itself] 
                - Hence, not a safe "wait-then-shutdown" mechanism [finished work JOINED / unfinished work DROPPED] 
            
            Hence, destructor won't explicitly call wait_all()    
            */
            ~JobSystem();

            JobSystem(const JobSystem&) = delete;
            JobSystem& operator=(const JobSystem&) = delete;

            /*
            setter => called in run_loop() to identify "this" worker (current) 
            modifies current_worker_ 
            (setter sets the private variable internally)
            */
            void set_current_worker(Worker *worker);

            /*
            Submit the root task (no predecessors) [STARTING POINT] to the system
            Safe to call from main-thread (before workers consume) OR from inside a running worker thread
            called from a worker => push the task onto THAT worker's own queue [cache locality]
            */
            void submit(Task *task);
            /*
            Wait for all outstanding tasks [among all workers] to be executed i.e => (num_outstanding_ = 0)
            blocks the main-thread [until every task submitted/unblocked by it has finished executing]
            */
            void wait_all();

            /*Internal helpers exposed to Worker [for utility purposes]*/
            bool is_shutting_down() const;
            int worker_count() const;
            Worker& worker_at(int index);

            /*
            Called by a Worker (from Worker::execute(), on its own thread)
            After a task has been executed by THAT Worker & that task's fn() has been returned:
            - decrementing each successor's unfinished_predecessors
            - scheduling any successor that just hit 0
            - decrementing num_outstanding_ for keeping a global track of leftover work
            */
            void on_task_finished(Task *task);
        private:
            /*
            worker owning "this thread" if any. Remains nullptr for the main thread
            Purpose : recognition of the worker on the basis of its LOCAL thread

            If current_worker_ => thread_local 
            Every single thread in the prog gets its own completely private copy of 'current_worker_'
            */
            static thread_local Worker* current_worker_; 

            /*Stores a unique ptr to each Worker => memory location stored for each worker*/
            std::vector<std::unique_ptr<Worker>> workers_;

            std::atomic<int> num_outstanding_{0};     // outstanding Tasks remaining 
            std::atomic<bool> shutdown_flag_{false};  // Signals is_shutting_down() on being flagged True
    
            /*
            round-robin cursor for submissions (tasks) arriving from outside any worker
            outside thread(s) => main thread (only 1 for now) [atomic<int> + fetch_add() : if multiple external threads submit concurrently]
            */
            int next_submit_worker_ = 0;
    };
}