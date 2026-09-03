#include <jobsys/job_system.hpp>
#include <jobsys/task.hpp>
#include <chrono>
#include <stdexcept>
#include <thread>

using namespace jobsys;

// Initialize a worker locally
thread_local Worker* JobSystem::current_worker_ = nullptr;

JobSystem::JobSystem(int num_workers) {
    workers_.reserve(static_cast<size_t>(num_workers));

    // Allocate unique pointers to each worker (construct the workers vector)
    for(int i = 0 ; i < num_workers ; ++i){
        workers_.push_back(std::make_unique<Worker>(i, *this));
    }

    // Now that all workers have been constructed => start the system
    for(auto& w : workers_){
        w->start();     // calls run_loop() by passing it as a fn to a new thread per worker
    }
}

JobSystem::~JobSystem(){
    /*
    Caller is to ensure => wait_all() is completed [for execution of all tasks]
    this destructor simply shutsdown the system w/o knowledge of execution completion
    */
    shutdown_flag_.store(true, std::memory_order_relaxed);
}

void JobSystem::set_current_worker(Worker *worker){
    current_worker_ = worker;
}

void JobSystem::submit(Task *task){
    num_outstanding_.fetch_add(1, std::memory_order_relaxed);

    if(current_worker_ != nullptr){
        /*
        Pushed to the worker's own deque (for cache locality)
        [Fast Path] : called from inside a running task on a worker thread
        */
        current_worker_->push_task(task);
    }else{
        /*
        [Slow/ext Path] : called from an external main thread(s) (outside of jobs)
        acts as a round-robin to distribute tasks across workers (n_workers)
        */
        int idx = next_submit_worker_;
        if(worker_count() < 1) throw std::invalid_argument("Atleast 1 Worker required");
        next_submit_worker_ = (next_submit_worker_ + 1) % worker_count();    // update the round-robin index to target the future worker (to distribute tasks to)
        workers_[static_cast<size_t>(idx)]->push_task(task);
    }
}
           
/*
reads the current count of active jobs (running workers) => before trying to conclude the process
memory_order_acquire : prevents infinite loop cond(as updated memory needs to be read {after worker threads finished their jobs})

no READ/WRITE ops allowed [memory_order_acquire => fence] (to prevent stale data reads or dangerous writes)
*/
void JobSystem::wait_all(){
    while(num_outstanding_.load(std::memory_order_acquire) > 0){
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
}

bool JobSystem::is_shutting_down() const {
    return shutdown_flag_.load(std::memory_order_relaxed);
}

int JobSystem::worker_count() const {
    return static_cast<int>(workers_.size());
}

/*Internal helpers exposed to Worker [for utility purposes]*/
Worker& JobSystem::worker_at(int index){
   return *workers_[static_cast<size_t>(index)]; 
}

void JobSystem::on_task_finished(Task *task){
    for(Task *succ : task->successors){
        /*
        Check unfinished previous tasks to the successor (by counting)
        Multiple predecessors of a successor CAN finish concurrently ON different threads (hence, atomic predecessors)
        If for a successor <= (unfinished_predecessors = 0) : submit(successor) [task] 
        */
        int remaining = succ->unfinished_predecessors.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if(remaining == 0){
            submit(succ);
        }
    }
    // Decrement(sub) the number of outstanding tasks post submit(task)
    num_outstanding_.fetch_sub(1, std::memory_order_acq_rel);
}