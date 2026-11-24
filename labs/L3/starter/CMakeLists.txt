add_executable(counter counter.cpp)
target_link_libraries(counter PRIVATE Threads::Threads)
add_executable(queue_test queue_test.cpp)
target_link_libraries(queue_test PRIVATE Threads::Threads)
