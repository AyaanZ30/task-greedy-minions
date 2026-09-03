#pragma once

#include <vector>
#include <memory>
#include "jobsys/task.hpp"

// Builds ONE parallel blur task covering rows [start_row, end_row) of the image.
// Reads from `input` only; writes only to its own exclusive row range in `output`.
// Safe for concurrent execution across chunks because:
//   - input is never mutated by any task (read-only for the whole run)
//   - each task's output rows are disjoint from every other task's output rows
//     (only READS may cross into a neighboring chunk's rows, via the 3x3 kernel;
//      writes never do)
inline std::unique_ptr<jobsys::Task> create_blur_task(const std::vector<float>& input, std::vector<float>& output, int start_row, int end_row, int width, int height)
{
    return std::make_unique<jobsys::Task>(
        [&input, &output, start_row, end_row, width, height]() {
            for (int y = start_row; y < end_row; ++y) {
                for (int x = 0; x < width; ++x) {
                    float pixel_sum = 0.0f;
                    int neighbors = 0;
                    for (int ky = -1; ky <= 1; ++ky) {
                        for (int kx = -1; kx <= 1; ++kx) {
                            int ny = y + ky;
                            int nx = x + kx;
                            if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
                                pixel_sum += input[ny * width + nx];
                                neighbors++;
                            }
                        }
                    }
                    output[(y * width) + x] = pixel_sum / neighbors;
                }
            }
        });
}

// Serial reference implementation of the SAME algorithm, no jobsys involved.
// Used as (a) a timing baseline and (b) a correctness oracle to diff against
// the parallel result. Must stay in lockstep with the logic inside
// create_blur_task's lambda above — if you change one, change both, or the
// comparison becomes meaningless.
inline void blur_serial(
    const std::vector<float>& input,
    std::vector<float>& output,
    int width, int height)
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float pixel_sum = 0.0f;
            int neighbors = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int ny = y + ky;
                    int nx = x + kx;
                    if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
                        pixel_sum += input[ny * width + nx];
                        neighbors++;
                    }
                }
            }
            output[(y * width) + x] = pixel_sum / neighbors;
        }
    }
}