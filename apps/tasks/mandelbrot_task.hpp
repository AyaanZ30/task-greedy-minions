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


/*main() for benchmarking for mandelbrot task*/

// int main()
// {
//     const unsigned int cores = std::thread::hardware_concurrency();
//     const unsigned int num_workers = (cores > 0) ? cores : 4;

//    // 1080p Mandelbrot setup
//     constexpr int WIDTH = 16000;
//     constexpr int HEIGHT = 16000;
//     constexpr int MAX_ITERATIONS = 50;

//     std::vector<uint8_t> out_serial(WIDTH * HEIGHT, 0);
//     std::vector<uint8_t> out_parallel(WIDTH * HEIGHT, 0);

//     double serial_time = serial_execution(mandelbrot_serial, out_serial, WIDTH, HEIGHT, MAX_ITERATIONS);
//     std::cout << "Serial time : " << std::fixed << std::setprecision(3) << serial_time << " sec\n";
    
//     const int num_chunks = num_workers * 8;
//     int chunk_size = (HEIGHT / num_chunks);      

//     /*stores task objects in memory throughout execution lifecycle*/
//     std::vector<std::unique_ptr<Task>> tasks;
//     std::vector<Task*> initial_tasks;     // stores ptr to 4 filter tasks (void fn's => filter even numbers from the specific chunk)

//     /*
//     define the aggregator task => basically returns the final result (aggregation of worker executed tasks) => 4 * result(filter-task)
//     Modify aggregate when => running a different job (with it's own aggregator reqs)
//     */
//     auto aggregate = std::make_unique<Task>([]() {
        
//     });

//     // Task *aggregate_ptr = aggregate.get();   non-owning reference to actual owner (unique_ptr)

//     for(int c = 0 ; c < num_chunks ; ++c){
//         int start = (c * chunk_size);
//         int end = (c == num_chunks - 1) ? HEIGHT : (start + chunk_size);
        
//         /*lambda filter fn(void()) => basically stored in each workers deque as a task to be popped and executed*/
//         auto mb_task = create_mandelbrot_task(out_parallel, WIDTH, HEIGHT, start, end, MAX_ITERATIONS);
//         /*
//         once filter finished => decrement aggregator's predecessor count [as aggregator is the final step of the job]
//         if predecessors(aggregator) = 0 {no filter task pending} : aggregator is now runnable (ready to be executed)
//         Hence => successor(filter) = aggregator || predecessor(aggregator) = filter 
//         */
//         mb_task->add_successors(aggregate.get());
//         initial_tasks.push_back(mb_task.get());

//         /*racks up lambda filter fn in the deque's of worker threads as tasks (to be executed / stolen)*/
//         tasks.push_back(std::move(mb_task));
//     }

//     tasks.push_back(std::move(aggregate));

//     double parallel_time = 0.0;
//     {
//         auto p0 = std::chrono::steady_clock::now();
//         {
//             JobSystem system(num_workers);
//             for(Task* t : initial_tasks){
//                 system.submit(t);
//             }

//             system.wait_all();
//             // JobSystem destructor executed here (sets shutdown_flag_, joins threads)
//             // Safe because wait_all() already guaranteed no work is outstanding.
//         }
//         auto p1 = std::chrono::steady_clock::now();
//         parallel_time = std::chrono::duration<double>(p1 - p0).count();
//     }   
//     std::cout << "Parallel Time : " << parallel_time << " sec (" << num_workers << " workers)\n";
//     std::cout << "Speedup       : " << (serial_time / parallel_time) << "x\n";

//     bool correct = (out_serial == out_parallel);
//     std::cout << "Verification  : " << (correct ? "PASSED" : "FAILED") << "\n";

//     return 0;
// } 