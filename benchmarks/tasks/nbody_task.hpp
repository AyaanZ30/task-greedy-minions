#pragma once

#include <vector>
#include <memory>
#include <cmath>
#include "jobsys/task.hpp"

namespace jobsys {

// Single-timestep, all-pairs (O(n^2)) gravitational force calculation.
// Structurally safe for this scheduler by design:
//   - EVERY task reads the FULL positions/masses arrays (read-only, shared,
//     safe for concurrent reads by construction)
//   - EVERY task writes ONLY to its own disjoint slice of the output
//     velocity array (no shared mutable state, no locking needed)
//   - flat one-level fan-out, no recursive spawning (unlike quicksort)
//   - memory footprint is a handful of float arrays sized N, not N^2 --
//     small even for N in the tens of thousands
struct Body {
    float x, y, z;
    float vx, vy, vz;
    float mass;
};

constexpr float GRAVITATIONAL_CONSTANT = 6.674e-11f;
constexpr float SOFTENING = 1e-4f; // avoids division-by-zero / singularity
                                     // when two bodies get very close --
                                     // standard trick in n-body sims, not
                                     // a correctness workaround for a bug

inline void compute_forces_range(
    const std::vector<Body>& bodies,
    std::vector<Body>& output,
    int start, int end,
    float dt)
{
    int n = static_cast<int>(bodies.size());
    for (int i = start; i < end; ++i) {
        float fx = 0.0f, fy = 0.0f, fz = 0.0f;
        for (int j = 0; j < n; ++j) {
            if (j == i) continue;
            float dx = bodies[j].x - bodies[i].x;
            float dy = bodies[j].y - bodies[i].y;
            float dz = bodies[j].z - bodies[i].z;
            float dist_sq = dx * dx + dy * dy + dz * dz + SOFTENING;
            float inv_dist = 1.0f / std::sqrt(dist_sq);
            float inv_dist3 = inv_dist * inv_dist * inv_dist;
            float f = GRAVITATIONAL_CONSTANT * bodies[j].mass * inv_dist3;
            fx += f * dx;
            fy += f * dy;
            fz += f * dz;
        }
        output[i] = bodies[i];
        output[i].vx += fx * dt;
        output[i].vy += fy * dt;
        output[i].vz += fz * dt;
        output[i].x += output[i].vx * dt;
        output[i].y += output[i].vy * dt;
        output[i].z += output[i].vz * dt;
    }
}

inline std::unique_ptr<Task> create_nbody_task(
    const std::vector<Body>& bodies,
    std::vector<Body>& output,
    int start, int end, float dt)
{
    return std::make_unique<Task>([&bodies, &output, start, end, dt]() {
        compute_forces_range(bodies, output, start, end, dt);
    });
}

inline void nbody_serial(
    const std::vector<Body>& bodies,
    std::vector<Body>& output,
    float dt)
{
    compute_forces_range(bodies, output, 0, static_cast<int>(bodies.size()), dt);
}

} 