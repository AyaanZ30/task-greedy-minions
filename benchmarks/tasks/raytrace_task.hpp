#pragma once

#include <vector>
#include <memory>
#include <cmath>
#include <cstdint>
#include "jobsys/task.hpp"

namespace jobsys {

// Single-frame ray-sphere intersection rendering, NO recursive bouncing
// (no reflections/refractions -- that would reintroduce the "recursive
// spawning" pattern we're deliberately avoiding after the quicksort
// lesson). Each task computes color for a disjoint block of pixel ROWS,
// testing rays against a small, fixed, read-only scene -- structurally
// identical in shape to your Mandelbrot task, just with 3D vector math
// and a scene list instead of complex-plane iteration.

struct Vec3 {
    float x, y, z;
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    float length() const { return std::sqrt(dot(*this)); }
    Vec3 normalized() const { float l = length(); return {x / l, y / l, z / l}; }
};

struct Sphere {
    Vec3 center;
    float radius;
    uint8_t r, g, b; // flat color, no shading model needed for this benchmark
};

// Returns true and sets `t` to the nearest intersection distance if the
// ray hits the sphere; false otherwise. Standard quadratic-formula
// ray-sphere intersection test.
inline bool intersect_sphere(const Vec3& origin, const Vec3& dir, const Sphere& s, float& t) {
    Vec3 oc = origin - s.center;
    float a = dir.dot(dir);
    float b = 2.0f * oc.dot(dir);
    float c = oc.dot(oc) - s.radius * s.radius;
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false;
    t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    return t > 0;
}

inline void render_rows(
    std::vector<uint8_t>& output, // RGB, 3 bytes per pixel
    const std::vector<Sphere>& scene,
    int width, int height,
    int start_row, int end_row)
{
    Vec3 camera{0, 0, -5};
    for (int y = start_row; y < end_row; ++y) {
        for (int x = 0; x < width; ++x) {
            float u = (x - width / 2.0f) / width;
            float v = (y - height / 2.0f) / height;
            Vec3 dir = Vec3{u, v, 1.0f}.normalized();

            float closest_t = 1e9f;
            const Sphere* hit = nullptr;
            for (const auto& s : scene) {
                float t;
                if (intersect_sphere(camera, dir, s, t) && t < closest_t) {
                    closest_t = t;
                    hit = &s;
                }
            }

            size_t idx = (static_cast<size_t>(y) * width + x) * 3;
            if (hit) {
                output[idx] = hit->r;
                output[idx + 1] = hit->g;
                output[idx + 2] = hit->b;
            } else {
                output[idx] = output[idx + 1] = output[idx + 2] = 20; // background
            }
        }
    }
}

inline std::unique_ptr<Task> create_raytrace_task(
    std::vector<uint8_t>& output,
    const std::vector<Sphere>& scene,
    int width, int height, int start_row, int end_row)
{
    return std::make_unique<Task>([&output, &scene, width, height, start_row, end_row]() {
        render_rows(output, scene, width, height, start_row, end_row);
    });
}

inline void raytrace_serial(
    std::vector<uint8_t>& output,
    const std::vector<Sphere>& scene,
    int width, int height)
{
    render_rows(output, scene, width, height, 0, height);
}

} 