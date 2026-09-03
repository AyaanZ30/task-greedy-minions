cmake_minimum_required(VERSION 4.0.0)
project(greedy-minions CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)
# target_include_directories({PROJECT_NAME} PRIVATE {CMAKE_SOURCE_DIR}/include)

option(JOBSYS_BUILD_TESTS "Build tests" ON)
option(JOBSYS_BUILD_BENCH "Build benchmarks" ON)

# ThreadSantizer (TSan) : detects data races in multi-threaded code at runtime [effective testing]
option(JOBSYS_ENABLE_TSAN "Build with ThreadSanitizer" OFF)

if(JOBSYS_ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -g)
    add_link_options(-fsanitize=thread)
endif()

add_subdirectory(src)
add_subdirectory(apps)

if(JOBSYS_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(JOBSYS_BUILD_BENCH)
    add_subdirectory(benchmarks)
endif()
