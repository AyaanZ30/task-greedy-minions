#pragma once

#include <vector>
#include <memory>
#include <chrono>
#include "benchmark.hpp"
#include "jobsys/job_system.hpp"
#include "jobsys/task.hpp"
#include "tasks/raytrace_task.hpp"

using namespace jobsys;

class RaytraceBenchmark : public Benchmark {
public:
    RaytraceBenchmark(int width, int height, int chunk_multiplier = 8)
        : width_(width), height_(height), chunk_multiplier_(chunk_multiplier),
          out_serial_(width * height * 3, 0), out_parallel_(width * height * 3, 0) {
        // Fixed, small scene -- read-only, shared safely across all tasks.
        scene_ = {
            {{0, 0, 20}, 3.0f, 220, 60, 60},
            {{-4, 1, 25}, 2.0f, 60, 220, 60},
            {{4, -1, 18}, 1.5f, 60, 60, 220},
            {{0, -3, 30}, 4.0f, 220, 220, 60},
        };
    }

    std::string name() const override { return "Ray-Sphere Tracing (single frame, no bounces)"; }

    double run_serial() override {
        auto t0 = std::chrono::steady_clock::now();
        raytrace_serial(out_serial_, scene_, width_, height_);
        auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(t1 - t0).count();
    }

    double run_parallel(unsigned int num_workers) override {
        const int num_chunks = static_cast<int>(num_workers) * chunk_multiplier_;
        int rows_per_chunk = height_ / num_chunks;

        std::vector<std::unique_ptr<Task>> tasks;
        std::vector<Task*> initial_tasks;
        auto aggregate = std::make_unique<Task>([]() {});

        for (int c = 0; c < num_chunks; ++c) {
            int start = c * rows_per_chunk;
            int end = (c == num_chunks - 1) ? height_ : (start + rows_per_chunk);

            auto t = create_raytrace_task(out_parallel_, scene_, width_, height_, start, end);
            t->add_successors(aggregate.get());
            initial_tasks.push_back(t.get());
            tasks.push_back(std::move(t));
        }
        tasks.push_back(std::move(aggregate));

        auto p0 = std::chrono::steady_clock::now();
        {
            JobSystem system(num_workers);
            for (Task* t : initial_tasks) system.submit(t);
            system.wait_all();
        }
        auto p1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(p1 - p0).count();
    }

    bool verify(){
        return out_serial_ == out_parallel_; // exact match: deterministic, no
                                              // shared accumulation, safe for
                                              // exact byte comparison
    }

private:
    int width_, height_, chunk_multiplier_;
    std::vector<Sphere> scene_;
    std::vector<uint8_t> out_serial_, out_parallel_;
};