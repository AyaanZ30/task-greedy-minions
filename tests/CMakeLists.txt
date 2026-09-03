add_executable(test_deque test_deque.cpp)
target_link_libraries(test_deque PRIVATE jobsys)
target_include_directories(test_deque PRIVATE ${CMAKE_SOURCE_DIR}/third_party)

add_test(NAME deque COMMAND test_deque)