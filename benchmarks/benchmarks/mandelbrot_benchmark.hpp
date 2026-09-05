#pragma once

#include <vector>
#include <memory>
#include <cstdint>

#include "../benchmark.hpp"
#include "../tasks/mandelbrot_task.hpp"

#include "jobsys/job_system.hpp"
#include "jobsys/task.hpp"

using namespace jobsys;

class MandelbrotBenchmark : public Benchmark {
    public:
        MandelbrotBenchmark(int height, int width, int max_iterations) 
            : height_(height), width_(width), max_iter_(max_iterations), out_serial_(width * height, 0), out_parallel_(width * height, 0) {}

        std::string name() const override { return "Mandelbrot"; }

        double run_serial() override {
            auto t0 = std::chrono::steady_clock::now();
            mandelbrot_serial(out_serial_, width_, height_, max_iter_);
            auto t1 = std::chrono::steady_clock::now();

            return std::chrono::duration<double>(t1 - t0).count();
        }

        double run_parallel(unsigned int num_workers) override {
            const int num_chunks = (num_workers * 8);
            const int chunk_size = (height_ / num_chunks);

            std::vector<std::unique_ptr<Task>> tasks;
            std::vector<Task*> initial_tasks;

            auto aggregate = std::make_unique<Task>([]() {});

            for(int c = 0 ; c < num_chunks ; ++c){
                int start = c * chunk_size;
                int end = (c == num_chunks - 1) ? height_ : (start + chunk_size);

                auto mb_task = create_mandelbrot_task(out_parallel_, width_, height_, start, end, max_iter_);
                mb_task->add_successors(aggregate.get());
                initial_tasks.push_back(mb_task.get());
                tasks.push_back(std::move(mb_task));
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
        int height_;
        int width_;
        int max_iter_;

        std::vector<uint8_t> out_serial_; 
        std::vector<uint8_t> out_parallel_; 
};