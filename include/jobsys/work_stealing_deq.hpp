#pragma once 

#include <mutex>
#include <deque>
#include <optional>

namespace jobsys {

    // forward declaration — deque doesn't need Task's definition, only a pointer
    struct Task;  

    /*
    per-worker deque [double-ended queue] of tasks (for execution one-by-one)

    Thread categs:
    1] Owner  => pops tasks from bottom for execution        [LIFO -> cache-friendly, works on its recent tasks provided by master job]
    (push_bottom, pop_bottom)

    2] Robber => steals tasks (one-at-a-time) from the owner [FIFO -> takes the oldest/biggest chunk of work, once idle from owner]
    (steal_top)

    v1 : correctness-oriented, not lock-free (temporarily)
    */
    class WorkStealingDeque 
    {
        private:
            std::deque<Task*> tasks_;
            /*
            mutable allows mutex to be modified even inside (const) member functions
            when we lock a mutex (via mutex.lock() / lock_guard()) inside a const member => state of mutex object modified
            to avoid compiler from throwing error of modification in a const mem func => `mutable`
            */
            mutable std::mutex mtx_;
        public:
            WorkStealingDeque() = default;

            // Disabling copy + assignment operations [to ensure ONLY 1 deque per worker]
            // passing a WorkStealingDeque object by value into a function    => NOT ALLOWED
            // transferring ownership of WorkStealingDeque to a diff variable => NOT ALLOWED
            WorkStealingDeque(const WorkStealingDeque&) = delete;
            WorkStealingDeque& operator=(const WorkStealingDeque&) = delete;

            // owner-end [push a task to the bottom of the deque] (task <= new FIFO)
            void push_bottom(Task *task);

            // robber-only [steal a task from top of the target threads deque] (nullptr if empty deque)
            Task* steal_top();

            // owner-only [pop a task from bottom of deque for exec] (nullptr if empty deque)
            Task* pop_bottom();

            size_t unsafe_size() const;
    };
}
