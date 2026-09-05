add_executable(scheduler main.cpp)
# link the executable to the jobsys library that performs the computation
target_link_libraries(scheduler PRIVATE jobsys Threads::Threads)

target_include_directories(scheduler PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/benchmarks
)