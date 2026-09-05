#pragma once

#include <string>

class Benchmark{
    public:
        virtual ~Benchmark() = default;

        virtual std::string name() const = 0;                                                                              

        virtual double run_serial() = 0;

        virtual double run_parallel(unsigned int num_workers) = 0;
};