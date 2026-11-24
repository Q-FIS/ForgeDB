# finforge-coreCS: the Core CS wing's group builds.
# Build:  cmake --preset debug && cmake --build --preset debug
# Test:   ctest --preset debug
cmake_minimum_required(VERSION 3.25)
project(finforge_corecs LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # lets editors (VS Code, CLion) understand the code

# Warnings help beginners most. Read them; fix them.
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Threads REQUIRED)

# GoogleTest, downloaded automatically the first time you configure.
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

add_subdirectory(group-a)   # weeks 1-4: thread-pool HTTP server
add_subdirectory(group-b)   # weeks 1-4: event-loop HTTP server
add_subdirectory(engine)    # weeks 5-8: the shared engine header + stub
add_subdirectory(tests)     # black-box tests that check both groups' programs
