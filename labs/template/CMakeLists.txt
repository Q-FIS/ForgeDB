# My Core CS labs. Every folder L1, L2, ... with a CMakeLists.txt is built automatically.
cmake_minimum_required(VERSION 3.25)
project(qfis_labs LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wall -Wextra -Wpedantic)
find_package(Threads REQUIRED)

file(GLOB LAB_DIRS LIST_DIRECTORIES true ${CMAKE_SOURCE_DIR}/L*)
foreach(dir ${LAB_DIRS})
  if(EXISTS ${dir}/CMakeLists.txt)
    add_subdirectory(${dir})
  endif()
endforeach()
