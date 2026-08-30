add_executable(scheduler main.cpp)
# link the executable to the jobsys library that performs the computation
target_link_libraries(scheduler PRIVATE jobsys)