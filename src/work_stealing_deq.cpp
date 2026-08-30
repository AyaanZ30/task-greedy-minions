#include <mutex>
#include <thread>

#include "jobsys/work_stealing_deq.hpp"


namespace jobsys{

    void WorkStealingDeque::push_bottom(Task *t){
        std::lock_guard<std::mutex> lock(mtx_);
        tasks_.push_back(t);
    }

    // robber-only [steal a task from top of the target threads deque] (nullptr if empty deque)
    Task* WorkStealingDeque::steal_top(){
        std::lock_guard<std::mutex> steal_lock(mtx_);  
        if(tasks_.empty()) return nullptr;

        Task* t = tasks_.front(); 
        tasks_.pop_front(); 
        return t;     
    }

    // owner-only [pop a task from bottom of deque for exec] (nullptr if empty deque)
    Task* WorkStealingDeque::pop_bottom(){
        std::lock_guard<std::mutex> pop_lock(mtx_); 
        if(tasks_.empty()) return nullptr;

        Task* t = tasks_.back(); 
        tasks_.pop_back(); 
        return t;
    }

    size_t WorkStealingDeque::unsafe_size() const {
        std::lock_guard<std::mutex> size_safe_lock(mtx_); 
        return tasks_.size();
    }
}
