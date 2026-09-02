file(GLOB JOBSYS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_library(jobsys STATIC ${JOBSYS_SOURCES})

target_include_directories(jobsys PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(jobsys PUBLIC Threads::Threads)
target_compile_options(jobsys PRIVATE -Wall -Wextra -Wpedantic)