add_executable(ipc_bench ipc_bench.cpp)
target_link_libraries(ipc_bench PRIVATE Threads::Threads)
