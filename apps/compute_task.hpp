#pragma once

#include <vector>
#include <memory>
#include <cmath>
#include "jobsys/task.hpp"

// Deliberately compute-heavy, memory-light: each element does a fixed
// number of transcendental-function iterations (sin/cos/sqrt), touching
// very little memory per unit of CPU work. This isolates scheduler/
// threading overhead from memory-bandwidth effects — unlike the blur
// task, this should scale close to linearly with physical core count
// if the scheduler itself is efficient.
inline std::unique_ptr<jobsys::Task> create_compute_task(
    const std::vector<double>& input,
    std::vector<double>& output,
    int start, int end,
    int iterations_per_element)
{
    return std::make_unique<jobsys::Task>(
        [&input, &output, start, end, iterations_per_element]() {
            for (int i = start; i < end; ++i) {
                double x = input[i];
                for (int k = 0; k < iterations_per_element; ++k) {
                    x = std::sin(x) * std::cos(x) + std::sqrt(std::abs(x) + 1.0);
                }
                output[i] = x;
            }
        });
}

inline void compute_serial(
    const std::vector<double>& input,
    std::vector<double>& output,
    int iterations_per_element)
{
    for (size_t i = 0; i < input.size(); ++i) {
        double x = input[i];
        for (int k = 0; k < iterations_per_element; ++k) {
            x = std::sin(x) * std::cos(x) + std::sqrt(std::abs(x) + 1.0);
        }
        output[i] = x;
    }
}