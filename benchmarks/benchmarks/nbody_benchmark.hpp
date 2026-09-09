#pragma once

#include <vector>
#include <memory>
#include <chrono>
#include <random>

#include "benchmark.hpp"
#include "benchmarks/tasks/nbody_task.hpp"

#include "jobsys/job_system.hpp"
#include "jobsys/task.hpp"

using namespace jobsys;

class NBodyBenchmark : public Benchmark {
public:
    NBodyBenchmark(int num_bodies, int chunk_multiplier = 4)
        : num_bodies_(num_bodies), chunk_multiplier_(chunk_multiplier) {
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> pos_dist(-1000.0f, 1000.0f);
        std::uniform_real_distribution<float> mass_dist(1e10f, 1e15f);

        bodies_.resize(num_bodies_);
        for (auto& b : bodies_) {
            b.x = pos_dist(rng); b.y = pos_dist(rng); b.z = pos_dist(rng);
            b.vx = b.vy = b.vz = 0.0f;
            b.mass = mass_dist(rng);
        }
        out_serial_ = bodies_;
        out_parallel_ = bodies_;
    }

    std::string name() const override { return "N-Body Gravitational Sim (single timestep)"; }

    double run_serial() override {
        auto t0 = std::chrono::steady_clock::now();
        nbody_serial(bodies_, out_serial_, dt_);
        auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(t1 - t0).count();
    }

    double run_parallel(unsigned int num_workers) override {
        const int num_chunks = static_cast<int>(num_workers) * chunk_multiplier_;
        int chunk_size = num_bodies_ / num_chunks;

        std::vector<std::unique_ptr<Task>> tasks;
        std::vector<Task*> initial_tasks;
        auto aggregate = std::make_unique<Task>([]() {});

        for (int c = 0; c < num_chunks; ++c) {
            int start = c * chunk_size;
            int end = (c == num_chunks - 1) ? num_bodies_ : (start + chunk_size);

            auto t = create_nbody_task(bodies_, out_parallel_, start, end, dt_);
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
        if (out_serial_.size() != out_parallel_.size()) return false;
        for (size_t i = 0; i < out_serial_.size(); ++i) {
            // Exact float equality is valid here for the SAME reason it was
            // valid in your blur task: each body's new velocity/position is
            // computed from a fixed set of reads with no cross-thread
            // floating-point accumulation reordering -- same computation,
            // same order of operations, regardless of which worker or
            // thread executes it.
            if (out_serial_[i].x != out_parallel_[i].x ||
                out_serial_[i].y != out_parallel_[i].y ||
                out_serial_[i].z != out_parallel_[i].z ||
                out_serial_[i].vx != out_parallel_[i].vx ||
                out_serial_[i].vy != out_parallel_[i].vy ||
                out_serial_[i].vz != out_parallel_[i].vz) {
                return false;
            }
        }
        return true;
    }

private:
    int num_bodies_;
    int chunk_multiplier_;
    float dt_ = 0.01f;
    std::vector<Body> bodies_, out_serial_, out_parallel_;
};