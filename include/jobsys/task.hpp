#pragma once

#include <atomic>
#include <functional>
#include <vector>
#include <memory>

/*
Ensures safe pushing of a task fn in the WorkStealingDeque [which handles exec-stealing logic]
Tasks : Successor <=== Predecessor [in a forward chaining fashion]
RULE : do not push successor in deque with unfinished predecessors [unsafe operation (can crash)]

TASK DEPENDENCY RESOLUTION:
Task A (Producer): Loads 1,000,000 raw integers into a buffer
Task B (Consumer): Computes the average of those integers [or any other operation]

{A, B} -> Deque[A(top), B(bottom)] => pop_bottom() => execute(B) [A not loaded into buffer yet] => error/crash (no integers present for average calculation)
[A was a predecessor of successor B] : Unfinished(A) = Crash(B) / Finished(A) = Safe(B)
*/
namespace jobsys{
    // forward declare — Task needs to notify the system when done
    class JobSystem;
    
    // A single unit of work in the dependency graph (blueprint for each task T)
    struct Task{
        // std::function is a wrapper that can store any function [void() : function stored returns nothing]
        std::function<void()> fn;

        std::atomic<int> unfinished_predecessors{0};
        std::vector<Task*> successors;

        Task() = default;
        // constructor [Task gets an argument work (type void function) => initializes local member var fn with that arg-function(work)]
        explicit Task(std::function<void()> work_fn) : fn(std::move(work_fn)) {}

        // non-copyable => memory address of task* should not change [as copying creates another Task* pointing to a diff mem location]
        // Copying would silently break dependency wiring [Task Identity matters]
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        // non-movable [atomic is not movable by default anyway]
        Task(Task&&) = delete;
        Task& operator=(Task&&) = delete;

        //Add a dependency edge: `this` must finish before `successor` can run.
        // not thread-safe [call only during graph construction]
        void add_successors(Task* successor){
            successors.push_back(successor);

            // add any unfinished predecessors [prev tasks in hierarchy] for the current task (successor)
            successor->unfinished_predecessors.fetch_add(1, std::memory_order_relaxed);
        }
    };
}