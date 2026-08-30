add_library(jobsys STATIC *.cpp)

target_include_directories(jobsys PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(jobsys PUBLIC Threads::Threads)
target_compile_options(jobsys PRIVATE -Wall -Wextra -Wpedantic)