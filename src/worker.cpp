#include "jobsys/worker.hpp"
#include "jobsys/task.hpp"
#include "jobsys/job_system.hpp"

#include <chrono>
#include <iostream>

using namespace jobsys;

Worker::Worker(int id, JobSystem& system) 
    : id_(id), system_(system), rng_(std::random_device{}() + (id*2)) {
}

Worker::~Worker(){
    if(thread_.joinable()){
        thread_.join();
    }
}

void Worker::start(){
    // calls the run_loop() impl below to start the thread-task processing
    thread_ = std::thread(&Worker::run_loop, this);
}

void Worker::push_task(Task* task){
    queue_.push_bottom(task);
}

Task* Worker::try_steal(){
    return queue_.steal_top();
}

void Worker::run_loop(){
    // JobSystem::current_worker_ = this; won't work as current_worker_ is a private member of JobSystem class
    system_.set_current_worker(this);
    int failed_attempts = 0;

    while(!system_.is_shutting_down()){
        Task *task = queue_.pop_bottom();
        // If no local task present [in that worker's deque] => try to steal from a VICTIM worker's deque
        if(!task){
            int n = system_.worker_count();
            if(n > 1){
                int victim = static_cast<int>(rng_() % static_cast<unsigned>(n));

                // re-calculate the victim index (if current worker is its own victim)
                if(victim == id_){
                    victim = (victim + 1) % n;
                }
                task = system_.worker_at(victim).try_steal(); 
            }
        }

        if(task){
            failed_attempts = 0;

            // If throws an error => std::terminate (silently kills a worker) => task never executed (num_outstanding_ always > 0) => system stuck in wait_all() forever
            execute(task);         
        }else{
            ++failed_attempts;
            if(failed_attempts < 100){
                std::this_thread::yield();
            }else{
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }
        }
    }
}

/* 
the void func being the task to be executed (stored as a member in struct Task)
For a Task t : t->fun() => calls the void fn to be executed.
*/
void Worker::execute(Task* task){
    if(task->skip_.load(std::memory_order_acquire)){
        std::cerr << "[Worker " << id_ << "] Skipping task -- ancestor failed.\n";
        system_.on_task_finished(task);
        return;
    }

    try{
        task->fn();
    }catch(...){
        task->failed_.store(true, std::memory_order_relaxed);
        std::cerr << "[Worker " << id_ << "] Task threw (unknown) exception (--successors will NOT be scheduled).\n";
    }
    /*
    always decrement predecessors [for num_outstanding_ to -> 0] => so system doesn't wait_all() in a hanging-state
    (called irrespective of task's execution failure)
    */
    system_.on_task_finished(task);    
}


