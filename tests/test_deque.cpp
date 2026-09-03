#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "jobsys/work_stealing_deq.hpp"
#include "jobsys/task.hpp"

#include <thread>
#include <vector>
#include <atomic>
#include <set>

using namespace jobsys;

/* ---------- Single-threaded correctness (no concurrency yet) ---------- */ 
TEST_CASE("LIFO integrity : push_bottom -> pop_bottom removes the same task"){
    WorkStealingDeque dq;
    Task t1, t2, t3;

    dq.push_bottom(&t1);
    dq.push_bottom(&t2);
    dq.push_bottom(&t3);

    CHECK(dq.pop_bottom() == &t3);
    CHECK(dq.pop_bottom() == &t2);
    CHECK(dq.pop_bottom() == &t1);
}

TEST_CASE("pop_bottom on empty deque returns nullptr"){
    WorkStealingDeque dq;
    CHECK(dq.pop_bottom() == nullptr);
}

TEST_CASE("steal_top on empty deque returns nullptr"){
    WorkStealingDeque dq;
    CHECK(dq.steal_top() == nullptr);
}

TEST_CASE("steal_top => oldest task (FIFO order relative to push order)"){
    WorkStealingDeque dq;
    Task t1, t2, t3;

    dq.push_bottom(&t1);
    dq.push_bottom(&t2);
    dq.push_bottom(&t3);

    CHECK(dq.steal_top() == &t1);
    CHECK(dq.steal_top() == &t2);
    CHECK(dq.steal_top() == &t3);
}

TEST_CASE("mixed push/pop/steal maintains total task count with no duplicates or losses"){
    WorkStealingDeque dq;
    constexpr int N = 100;
    std::vector<Task> storage(N);

    for(int i = 0 ; i < N ; i++) dq.push_bottom(&storage[i]);

    std::set<Task*> seen;
    Task* t;
    while((t = dq.pop_bottom()) != nullptr){
        // if the deque didnt return duplicate of a unique task 
        CHECK(seen.insert(t).second);
    }

    CHECK(seen.size() == N);
}

/* ---------- Concurrent correctness  ---------- */ 

/*
Heavy payload (N) : 100000
Thieves : 4 threads (workers)
Tasks per thief ~ 25,000 tasks

Target race cond : Steady-State Throughput & Memory Order
[4 thief threads + 1 owner fighting for a massive pool of 100k tasks] (high vol)
*/
TEST_CASE("concurrent stealing: every task is delivered exactly once, no duplicates, no losses"){
    /*
    Most important failure mode to test & address (if the test fails)
    every task that goes in (push) => comes out (pop/steal) to exactly ONE caller(worker), exactly once
    2 workers executing the same task 2x OR loss of a task  (& its dependents never unblock)
    */
   WorkStealingDeque dq;
   constexpr int N = 100000;
   std::vector<Task> storage(N);

   for(int i = 0 ; i < N ; i++) dq.push_bottom(&storage[i]);

   constexpr int n_thieves = 4;
    std::vector<std::vector<Task*>> stolen_by(n_thieves);
    std::vector<Task*> popped_by_owner;

    std::atomic<bool> owner_done{false};

    // Owner thread (keeps pop_bottom() until all tasks are popped => signals done)
    // In reality : owner push_bottom() more to counteract the pop_bottom(), but this is just for check
    std::thread owner([&]() {              
        Task* t;
        while((t = dq.pop_bottom()) != nullptr) popped_by_owner.push_back(t);
        owner_done.store(true, std::memory_order_release);
    });

    std::vector<std::thread> thieves;
    for(int i = 0 ; i < n_thieves ; i++){
        thieves.emplace_back([&, i]() {
            while(true){
                Task *t = dq.steal_top();
                if(t) stolen_by[i].push_back(t);     // ith thief (thread) stole that task from the dq
                else if(owner_done.load(std::memory_order_acquire)) break;
            }
        });
    }

    owner.join();
    for(auto& th : thieves) th.join();

    std::set<Task*> all_delivered;
    size_t total_delivered = 0;

    for(Task* t : popped_by_owner){
        CHECK(all_delivered.insert(t).second);
        // ++total_delivered;
        ++total_delivered;
    }
    for(const std::vector<Task*>& list : stolen_by){
        for(Task *t : list) {CHECK(all_delivered.insert(t).second); ++total_delivered;}
    }

    CHECK(total_delivered == N);
    CHECK(all_delivered.size() == N);
}

/*
Lighter payload (N) : 500
Thieves : 8 threads (workers)
Tasks per thief ~ 60-62 tasks

Target race cond : No duplicated theft + Boundary/Empty Deque contention
[8 thief threads + 1 owner fighting for a small pool of 500 tasks] (low-vol)
*/
TEST_CASE("high_contention : many thieves, small deque, still no duplicates/losses"){
    WorkStealingDeque dq;
    constexpr int N = 500;
    std::vector<Task> storage(N);

    // constructed a dq with N(500) tasks 
    for(int i = 0 ; i < N ; i++) dq.push_bottom(&storage[i]);

    int n_thieves = 8;

    // keeps track of all tasks stolen by each thief (8 thieves x (N / 8) or dynamic no of tasks per thief)
    std::vector<std::vector<Task*>> stolen_by(n_thieves);     
    std::vector<Task*> popped_by_owner;
    std::atomic<bool> owner_done{false};

    // Main thread (owner) launched in the background (the workers threads start running immediately as well)
    std::thread owner([&]() {
        Task* t;
        while((t = dq.pop_bottom()) != nullptr) popped_by_owner.push_back(t);
        owner_done.store(true, std::memory_order_release);
    }); 

    // thieves => workers (threads) (below demonstrates the act of workers(thieves) stealing from a deque as individual threads)
    std::vector<std::thread> thieves;
    for(int i = 0 ; i < n_thieves ; i++){
        thieves.emplace_back([&, i]() {
            while(true){
                Task* t = dq.steal_top();
                if(t) stolen_by[(i)].push_back(t);
                else if(owner_done.load(std::memory_order_acquire)) break;
            }
        });
    }

    owner.join();
    for(auto& th : thieves) th.join();

    std::set<Task*> all_delivered;
    size_t total = 0;

    for(Task* t : popped_by_owner){
        CHECK(all_delivered.insert(t).second);
        ++total;
    }
    for(const std::vector<Task*>& list : stolen_by){
        // if a thief stole a task 2 times (duplicate theft)
        for(Task* t : list) {CHECK(all_delivered.insert(t).second); ++total;}
    }

    CHECK(total == N);     // if all N tasks were correctly popped by the owner 
    CHECK(all_delivered.size() == N);
}

