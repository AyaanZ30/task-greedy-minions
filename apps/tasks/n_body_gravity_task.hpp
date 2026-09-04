#pragma once

#include <vector>
#include <memory>
#include <cmath>
#include "jobsys/task.hpp"

struct Body{
    double x, y, z;
    double vx, vy, vz;
    double mass;
};

inline std::unique_ptr<jobsys::Task> create_gravity_task(const std::vector<Body>& bodies, std::vector<Body>& next_bodies, size_t start, size_t end, double dt = 0.001){
    return std::make_unique<jobsys::Task>([&bodies, &next_bodies, start, end, dt]() {
        constexpr double G = 6.67430e-11;
        constexpr double softening = 1e-9;

        for(size_t i = start ; i < end ; ++i){
            double fx = 0.0, fy = 0.0, fz = 0.0;
            const Body& bi = bodies[i];
            
            for(size_t j = 0 ; i < bodies.size() ; ++j){
                if(i == j) continue;
                const Body& bj = bodies[j];

                double dx = (bj.x - bi.x); 
                double dy = (bj.y - bi.y); 
                double dz = (bj.z - bi.z); 

                double dist_sq = dx*dx + dy*dy + dz*dz + softening;
                double inv_dist = 1.0 / std::sqrt(dist_sq);
                double inv_dist3 = inv_dist * inv_dist * inv_dist;

                double f = (G * bi.mass * bj.mass * inv_dist3);
                fx += f * dx;
                fy += f * dy;
                fz += f * dz    ;
            }

            Body updated = bi;
            updated.vx += (fx / bi.mass) * dt;
            updated.vy += (fy / bi.mass) * dt;
            updated.vz += (fz / bi.mass) * dt;
            updated.x += updated.vx * dt;
            updated.y += updated.vy * dt;
            updated.z += updated.vz * dt;

            next_bodies[i] = updated;
        }
    });
}

inline void gravity_serial(const std::vector<Body>& bodies, std::vector<Body>& next_bodies, double dt = 0.001){
    constexpr double G = 6.67430e-11;
    constexpr double softening = 1e-9;

    for (size_t i = 0; i < bodies.size(); ++i) {
        double fx = 0.0, fy = 0.0, fz = 0.0;
        const Body& bi = bodies[i];

        for (size_t j = 0; j < bodies.size(); ++j) {
            if (i == j) continue;
            const Body& bj = bodies[j];

            double dx = bj.x - bi.x;
            double dy = bj.y - bi.y;
            double dz = bj.z - bi.z;

            double dist_sq = dx*dx + dy*dy + dz*dz + softening;
            double inv_dist = 1.0 / std::sqrt(dist_sq);
            double inv_dist3 = inv_dist * inv_dist * inv_dist;

            double f = G * bi.mass * bj.mass * inv_dist3;
            fx += f * dx;
            fy += f * dy;
            fz += f * dz;
        }

        Body updated = bi;
        updated.vx += (fx / bi.mass) * dt;
        updated.vy += (fy / bi.mass) * dt;
        updated.vz += (fz / bi.mass) * dt;
        updated.x += updated.vx * dt;
        updated.y += updated.vy * dt;
        updated.z += updated.vz * dt;

        next_bodies[i] = updated;
    }
}   