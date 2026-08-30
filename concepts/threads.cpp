#include <iostream>
#include <algorithm>
#include <execution>
#include <vector>
#include <thread>
#include <future>
#include <atomic>

# define N_WORKERS 4

void born(int id){ std::cout << "Worker " << id << " born" << std::endl; }
void ThreadVecSpawn()
{
    // main thread spawning 4 other threads
    std::vector<std::thread> workers;

    for(int i=0 ; i<N_WORKERS ; i++){
        workers.push_back(std::thread(born, i));
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    for(auto& worker : workers){
        worker.join();
    }

    std::cout << "All workers are born.\n";
}

void MasterThreadSpawn()
{
    std::thread Master([]() {
        std::vector<std::thread> sub_workers;

        for(int i = 0 ; i < N_WORKERS ; i++){
            sub_workers.push_back(std::thread([i]() {
                std::cout << "Sub-worker " << i << " running inside Master.\n";
            }));
        }

        for(auto& w : sub_workers){
            w.join();
        }
        std::cout << "Master thread completed managing workers.\n";
    });

    Master.join();
}

int square(int x) { return x*x; }
void TaskBasedParallelism()
{
    std::vector<std::future<int>> results;

    for(int i = 0 ; i < N_WORKERS ; i++){
        results.push_back(std::async(std::launch::async, square, i));
    }

    for(size_t i = 0 ; i < results.size() ; i++){
        std::cout << "Result from worker " << i << ": " << results[i].get() << "\n";
    }
}

void ParallelExecutionPolicy()
{
    std::vector<int> data = {13, 24, 33, 4, 15, 66, 17, 89};

    std::for_each(std::execution::par, data.begin(), data.end(), [](int& x) {
        x *= 2;
    });

    for(auto& d : data){
        std::cout << d << ", ";
    }
    std::cout << "Parallel loop transformations complete.\n";
}

int normal_counter = 0;
std::atomic<int> atomic_counter(0);

void increment_counter(){
    for(int i = 0 ; i < 10000000 ; i++){
        normal_counter++;
        atomic_counter++;
    }
}

int main()
{
    std::thread t1(increment_counter);
    std::thread t2(increment_counter);

    t1.join(); t2.join();

    std::cout << "Normal Counter: " << normal_counter << "\n"; // Will likely be less than 20000
    std::cout << "Atomic Counter: " << atomic_counter << "\n"; // Guaranteed to be exactly 20000

    return 0;
}
