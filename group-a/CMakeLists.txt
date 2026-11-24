# Group A: the thread-pool HTTP server (weeks 1-4).
#
# You never need to edit this file to add code:
#   - every .cpp in src/   is part of the group library (qfis_a): Socket, HTTP parser, ...
#   - every .cpp in tests/ is a GoogleTest unit test (run with: ctest --preset debug)
#   - each program with a main() is in apps/ and listed below.

file(GLOB QFIS_A_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(qfis_a STATIC ${QFIS_A_SOURCES})
target_include_directories(qfis_a PUBLIC src)
target_link_libraries(qfis_a PUBLIC Threads::Threads)

foreach(app echo_server echo_client http_server)
  add_executable(a_${app} apps/${app}.cpp)
  target_link_libraries(a_${app} PRIVATE qfis_a)
endforeach()

file(GLOB QFIS_A_TESTS CONFIGURE_DEPENDS tests/*.cpp)
add_executable(a_unit_tests ${QFIS_A_TESTS})
target_link_libraries(a_unit_tests PRIVATE qfis_a GTest::gtest_main)
gtest_discover_tests(a_unit_tests)
