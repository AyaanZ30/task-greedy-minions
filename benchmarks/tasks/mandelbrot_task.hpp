#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include "jobsys/task.hpp"

// Computes a slice of the Mandelbrot set.
// Heavy math per pixel, minimal memory writes (1 byte per pixel).
// Naturally unbalanced across rows — ideal for testing work-stealing!
inline std::unique_ptr<jobsys::Task> create_mandelbrot_task(
    std::vector<uint8_t>& output,
    int width, int height,
    int start_row, int end_row,
    int max_iterations = 1000)
{
    return std::make_unique<jobsys::Task>(
        [&output, width, height, start_row, end_row, max_iterations]() {
            for (int y = start_row; y < end_row; ++y) {
                double cy = (y - height / 2.0) * 4.0 / height;
                for (int x = 0; x < width; ++x) {
                    double cx = (x - width / 2.0) * 4.0 / width;
                    double zx = 0.0, zy = 0.0;
                    int iter = 0;

                    while (zx * zx + zy * zy <= 4.0 && iter < max_iterations) {
                        double temp = zx * zx - zy * zy + cx;
                        zy = 2.0 * zx * zy + cy;
                        zx = temp;
                        ++iter;
                    }

                    output[y * width + x] = static_cast<uint8_t>(iter % 256);
                }
            }
        });
}

inline void mandelbrot_serial(
    std::vector<uint8_t>& output,
    int width, int height,
    int max_iterations = 1000)
{
    for (int y = 0; y < height; ++y) {
        double cy = (y - height / 2.0) * 4.0 / height;
        for (int x = 0; x < width; ++x) {
            double cx = (x - width / 2.0) * 4.0 / width;
            double zx = 0.0, zy = 0.0;
            int iter = 0;

            while (zx * zx + zy * zy <= 4.0 && iter < max_iterations) {
                double temp = zx * zx - zy * zy + cx;
                zy = 2.0 * zx * zy + cy;
                zx = temp;
                ++iter;
            }

            output[y * width + x] = static_cast<uint8_t>(iter % 256);
        }
    }
}
