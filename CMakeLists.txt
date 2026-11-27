# ForgeDB: a small search database, built from scratch by the Core CS wing.
#
#   cmake --preset debug             # configure (the first time downloads GoogleTest and JSON)
#   cmake --build --preset debug -j  # compile
#   ctest --preset debug             # run every test
#
# You never need to edit this file to add code:
#   - every .cpp under src/ is part of the `forge_core` library;
#   - every .cpp in tests/unit/<component>/ is a unit test (GoogleTest);
#   - every .cpp in bench/ is a benchmark program;
#   - deleting src/<component>/stub_<part>.cpp switches on
#     tests/contract/<component>/<part>_test.cpp (see issues/REFERENCE.md).
cmake_minimum_required(VERSION 3.25)
project(forgedb LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # lets editors (VS Code, CLion) understand the code

add_compile_options(-Wall -Wextra -Wpedantic)
find_package(Threads REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Downloaded automatically the first time you configure.
include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest json)

enable_testing()
include(GoogleTest)

# ---- The library: every component's code ---------------------------------
file(GLOB_RECURSE FORGE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_library(forge_core STATIC ${FORGE_SOURCES})
target_include_directories(forge_core PUBLIC include)
target_link_libraries(forge_core PUBLIC Threads::Threads nlohmann_json::nlohmann_json)

# ---- The programs ----------------------------------------------------------
add_executable(forgedb apps/forgedb.cpp)        # the server       (APP)
add_executable(forge apps/forge.cpp)            # the client CLI   (CLI)
add_executable(forge-load apps/forge_load.cpp)  # load generator   (LOAD)
foreach(app forgedb forge forge-load)
  target_link_libraries(${app} PRIVATE forge_core)
endforeach()

# ---- Contract tests: switched on when their stub file is deleted -----------
file(GLOB CONTRACT_TESTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/contract/*/*_test.cpp)
foreach(test_file ${CONTRACT_TESTS})
  get_filename_component(component_dir ${test_file} DIRECTORY)
  get_filename_component(component ${component_dir} NAME)
  get_filename_component(part ${test_file} NAME_WE)
  string(REGEX REPLACE "_test$" "" part ${part})
  if(EXISTS ${CMAKE_SOURCE_DIR}/src/${component}/stub_${part}.cpp)
    # Not started yet: shows as "Skipped" in ctest.
    add_test(NAME contract.${component}.${part} COMMAND sh -c "exit 77")
    set_tests_properties(contract.${component}.${part} PROPERTIES SKIP_RETURN_CODE 77)
  else()
    add_executable(contract_${component}_${part} ${test_file})
    target_include_directories(contract_${component}_${part} PRIVATE tests)
    target_link_libraries(contract_${component}_${part} PRIVATE forge_core GTest::gtest_main)
    gtest_discover_tests(contract_${component}_${part} TEST_PREFIX contract.${component}.
                         DISCOVERY_TIMEOUT 30 PROPERTIES TIMEOUT 120)
  endif()
endforeach()

# ---- Unit tests: your own, one program per component ----------------------
file(GLOB UNIT_DIRS LIST_DIRECTORIES true ${CMAKE_SOURCE_DIR}/tests/unit/*)
foreach(dir ${UNIT_DIRS})
  if(IS_DIRECTORY ${dir})
    get_filename_component(component ${dir} NAME)
    file(GLOB unit_sources CONFIGURE_DEPENDS ${dir}/*.cpp)
    if(unit_sources)
      add_executable(unit_${component} ${unit_sources})
      target_include_directories(unit_${component} PRIVATE tests)
      target_link_libraries(unit_${component} PRIVATE forge_core GTest::gtest_main)
      gtest_discover_tests(unit_${component} TEST_PREFIX unit.${component}.
                           DISCOVERY_TIMEOUT 30 PROPERTIES TIMEOUT 120)
    endif()
  endif()
endforeach()

# ---- Benchmarks: every bench/<name>.cpp becomes a program bench_<name> -----
# Measure with the release preset: cmake --build --preset release --target bench_<name>
file(GLOB BENCH_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/bench/*.cpp)
foreach(src ${BENCH_SOURCES})
  get_filename_component(name ${src} NAME_WE)
  add_executable(bench_${name} ${src})
  target_link_libraries(bench_${name} PRIVATE forge_core)
endforeach()

# ---- Black-box tests: Python, over real sockets ----------------------------
add_subdirectory(tests/blackbox)

# ---- Fuzzing (HTTP-5): only with the `fuzz` preset (needs clang) -----------
option(FORGE_FUZZ "Build the libFuzzer targets in fuzz/" OFF)
if(FORGE_FUZZ)
  file(GLOB FUZZ_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/fuzz/*.cpp)
  foreach(src ${FUZZ_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    add_executable(fuzz_${name} ${src})
    target_compile_options(fuzz_${name} PRIVATE -fsanitize=fuzzer)
    target_link_options(fuzz_${name} PRIVATE -fsanitize=fuzzer)
    target_link_libraries(fuzz_${name} PRIVATE forge_core)
  endforeach()
endif()
