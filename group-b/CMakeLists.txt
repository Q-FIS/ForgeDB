# Group B: the event-loop (epoll) HTTP server (weeks 1-4).
#
# You never need to edit this file to add code:
#   - every .cpp in src/   is part of the group library (qfis_b): Socket, HTTP parser, ...
#   - every .cpp in tests/ is a GoogleTest unit test (run with: ctest --preset debug)
#   - each program with a main() is in apps/ and listed below.

file(GLOB QFIS_B_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(qfis_b STATIC ${QFIS_B_SOURCES})
target_include_directories(qfis_b PUBLIC src)
target_link_libraries(qfis_b PUBLIC Threads::Threads)

foreach(app echo_server echo_client http_server)
  add_executable(b_${app} apps/${app}.cpp)
  target_link_libraries(b_${app} PRIVATE qfis_b)
endforeach()

file(GLOB QFIS_B_TESTS CONFIGURE_DEPENDS tests/*.cpp)
add_executable(b_unit_tests ${QFIS_B_TESTS})
target_link_libraries(b_unit_tests PRIVATE qfis_b GTest::gtest_main)
gtest_discover_tests(b_unit_tests)
