# Black-box tests: Python scripts that start your programs and talk to them
# over real sockets. A test reports SKIPPED while the program is still the
# placeholder (it exits with code 77).
#
# Tests for later weeks are switched on by the issue that needs them:
# uncomment the line, in the same PR as the code that makes it pass.
find_package(Python3 REQUIRED COMPONENTS Interpreter)

function(qfis_test name script)
  add_test(NAME ${name}
           COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/${script} ${ARGN})
  set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
endfunction()

foreach(g a b)
  # Week 1
  qfis_test(${g}_echo echo_test.py $<TARGET_FILE:${g}_echo_server> $<TARGET_FILE:${g}_echo_client>)
  # Week 2
  qfis_test(${g}_http http_test.py $<TARGET_FILE:${g}_http_server>)
  qfis_test(${g}_http_fuzz http_fuzz.py $<TARGET_FILE:${g}_http_server> 3000)
endforeach()

# Week 3, Group A: switched on by W3.A3 (backpressure), W3.A4 (shutdown), W3.A5 (concurrency)
# qfis_test(a_backpressure backpressure_test.py $<TARGET_FILE:a_http_server>)
# qfis_test(a_shutdown shutdown_test.py $<TARGET_FILE:a_http_server>)
# qfis_test(a_concurrency concurrency_test.py $<TARGET_FILE:a_http_server>)

# Week 3, Group B: switched on by W3.B5
# qfis_test(b_concurrency concurrency_test.py $<TARGET_FILE:b_http_server>)
# qfis_test(b_many_connections many_connections_test.py $<TARGET_FILE:b_http_server>)

# Week 4: switched on by W4.A1 / W4.B1 (idle timeout) and W4.B2 (B's shutdown)
# qfis_test(a_idle_timeout idle_timeout_test.py $<TARGET_FILE:a_http_server>)
# qfis_test(b_idle_timeout idle_timeout_test.py $<TARGET_FILE:b_http_server>)
# qfis_test(b_shutdown shutdown_test.py $<TARGET_FILE:b_http_server>)
