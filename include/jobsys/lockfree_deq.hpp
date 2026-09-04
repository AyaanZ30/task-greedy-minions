#pragma once

#include <atomic>
#include <cstdint>

namespace jobsys{
    struct Task;

    /*
    Chase-Lev lock free work-stealing deque (resolves the dangerous concurrent access to a shared state issue)
    
    Explanation:
    - Say a task => Task *P is the only remaining task in the deque of the owner thread ((bottom - top) = 1)
      Consider : Both owner & thief thread hold the pointer to the same task [Task *P] (last)
    
    - Hence, EITHER the owner OR the thief => can get the task *P for execution (dependent on how fast the CAS [Compare-and-swap] operation executes) (an atomic hardware level operation)
      If time[CAS(owner)] < time[CAS(thief)] : Owner gets to execute the task & vice-versa

    - Two workers will call execute(P) — P->fn() runs twice, and system_.on_task_finished(P) runs twice, which double-decrements successor predecessor-counts 
      (potentially triggering a successor to run before all its real dependencies finished) and double-decrements (num_outstanding_) => false reads for successor
    
    - This would lead to => premature scheduling of the successor 
    */
    class LockFreeDeque{
        public:
            static constexpr size_t CAPACITY = 4096;
            LockFreeDeque() : top_(0), bottom_(0) {}    

            /*one lock-free deque per worker (to be passed as ref only [as single source of truth]) */
            LockFreeDeque(const LockFreeDeque&) = delete;
            LockFreeDeque& operator=(const LockFreeDeque&) = delete;

            // owner-only
            void push_bottom(Task *t){
                int64_t b = bottom_.load(std::memory_order_relaxed);
                /* Write the task into the slot BEFORE publishing the new bottom.
                   Release semantics ensure this write is visible to any thief
                   that later observes the updated bottom via an acquire load.*/ 
                buffer_[b & (CAPACITY - 1)] = t;
                std::atomic_thread_fence(std::memory_order_release);
                bottom_.store(b + 1, std::memory_order_relaxed);    // update to new bottom (b -> b+1) as a new task was pushed to bottom
            }

            // owner-only
            Task *pop_bottom(){
                int64_t b = bottom_.load(std::memory_order_relaxed) - 1;
                bottom_.store(b, std::memory_order_relaxed);

                // Full fence: this pairs with steal_top's fence. Without this,
                // the owner's write to `bottom` and its subsequent read of `top`
                // could be reordered by the CPU/compiler, recreating the exact
                // race from Part 1 -- this fence is not optional or "for safety
                // margin," it is load-bearing for correctness.
                std::atomic_thread_fence(std::memory_order_seq_cst);

                int64_t t = top_.load(std::memory_order_relaxed);

                if(t > b){
                    bottom_.store(b + 1, std::memory_order_relaxed);
                    return nullptr;
                }

                /*task to pop from the bottom (b) of the deque*/
                Task* task = buffer_[b & (CAPACITY - 1)];
                if(t == b){
                    // Owner OR Thief can have the task (depending on which CAS-inst finished first)
                    // Race explicitly against any thief via CAS on top.
                    if(!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)){
                        // Meaning : If the thief's CAS fails => it fails to steal the task (popped from bottom by the owner itself)
                        task = nullptr;
                    }
                    bottom_.store(b + 1, std::memory_order_relaxed);
                }
                return task;
            }

            // thief-only (called by other workers)
            Task* steal_top(){
                int64_t t = top_.load(std::memory_order_acquire);

                // Pairs with pop_bottom's fence -- same reasoning, ensures this
                // thief sees an up-to-date `bottom` relative to its `top` read.
                std::atomic_thread_fence(std::memory_order_seq_cst);

                int64_t b = top_.load(std::memory_order_acquire);

                if(t >= b) return nullptr;

                // Task to be stolen (from top of deque)
                Task* task = buffer_[t & (CAPACITY - 1)];

                // ALL thieves must always CAS, since multiple thieves contending for the same top slot is a completely ordinary occurrence.
                // Thieves v/s Thieves [contention] for stealing from top of buffer
                if(t < b - 1){
                    if(!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)){
                        return nullptr;   // thief -> lost CAS race to owner OR another thief
                    }
                }
                return task;
            }
        
        private:
            /*
            Modern CPU (x86_64 / ARM64) => stores and fetches memory in cache-lines (64-byte chunks)
            alignas(64) : ensures the variable is stored at an in-memory address (in multiples of 64)
            
            Hence => alignas(64) prevents False Sharing in Multi-threading
            [If 2 diff workers write to 2 variables that happen to share the same 64-byte cache line,
            the CPU core invalidates each other's cache (slowing down execution)]
    
            alignas(64) => forces each variable into its own dedicated cache line (for smoother, non-obstructed updates)
            */
            alignas(64) std::atomic<int64_t> top_;
            alignas(64) std::atomic<int64_t> bottom_;

            Task* buffer_[CAPACITY] = {};
    };
}