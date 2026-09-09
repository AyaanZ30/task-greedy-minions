#pragma once

#include <vector>
#include <chrono>

#include "benchmark.hpp"
#include "benchmarks/tasks/transcedental_task.hpp"

#include "jobsys/job_system.hpp"
#include "jobsys/task.hpp"

using namespace jobsys;

class TranscedentalBenchmark : public Benchmark{
    public:
        TranscedentalBenchmark(std::vector<double> input, int N, int max_iter) 
            : input_(input), n_(N), max_iter_(max_iter), out_serial_(N, 0), out_parallel_(N, 0) {}

        std::string name() const { return "Transcedental"; }

        double run_serial() override {
            auto t0 = std::chrono::steady_clock::now();
            compute_serial(input_, out_serial_, max_iter_);
            auto t1 = std::chrono::steady_clock::now();

            return std::chrono::duration<double>(t1 - t0).count();
        }

        double run_parallel(unsigned int num_workers) override {
            const int num_chunks = (num_workers * 8);
            const int chunk_size = (n_ / num_chunks);

            std::vector<std::unique_ptr<Task>> tasks;
            std::vector<Task*> initial_tasks;

            auto aggregate = std::make_unique<Task>([]() {});

            for(int c = 0 ; c < num_chunks ; ++c){
                int start = (c * chunk_size);
                int end = (c == num_chunks - 1) ? n_ : (start + chunk_size);

                auto tsk = create_compute_task(input_, out_parallel_, start, end, max_iter_);
                tsk->add_successors(aggregate.get());
                initial_tasks.push_back(tsk.get());
                tasks.push_back(std::move(tsk));
            }
            tasks.push_back(std::move(aggregate));

             auto p0 = std::chrono::steady_clock::now();
            {
                JobSystem system(num_workers);
                for(Task* t : initial_tasks) { system.submit(t); }
                system.wait_all();
            }
            auto p1 = std::chrono::steady_clock::now();

            return std::chrono::duration<double>(p1 - p0).count();
        }

    private:
        int n_, max_iter_;
        
        std::vector<double> input_;
        std::vector<double> out_serial_; 
        std::vector<double> out_parallel_; 
};