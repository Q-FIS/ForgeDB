# Black-box tests: Python scripts that start a server and talk to it over real
# sockets. While a program is still a placeholder (it exits with 77), its
# tests report SKIPPED.
#
# Lines marked "switched on by" are uncommented by that issue, in the same PR
# as the code that makes them pass.

function(forge_blackbox name script)
  add_test(NAME blackbox.${name}
           COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/${script} ${ARGN})
  set_tests_properties(blackbox.${name} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
endfunction()

set(FORGEDB $<TARGET_FILE:forgedb>)

# Against the fake server: proves the tests themselves are right.
forge_blackbox(smoke.fake    smoke_test.py   fake)
forge_blackbox(restart.fake  restart_test.py fake)
forge_blackbox(api.fake      api_test.py     fake)

# The clients, against the fake server.
forge_blackbox(cli               cli_test.py           $<TARGET_FILE:forge>)
forge_blackbox(cli.load          cli_load_test.py      $<TARGET_FILE:forge>)
forge_blackbox(python_client     python_client_test.py fake)

# Against the real server.
forge_blackbox(smoke         smoke_test.py       ${FORGEDB})
forge_blackbox(api           api_test.py         ${FORGEDB})
forge_blackbox(keepalive     keepalive_test.py   ${FORGEDB})
forge_blackbox(shutdown      shutdown_test.py    ${FORGEDB})
forge_blackbox(concurrency   concurrency_test.py ${FORGEDB})
forge_blackbox(chaos         chaos_test.py       ${FORGEDB})
forge_blackbox(crash         crash_test.py       ${FORGEDB})
# forge_blackbox(restart     restart_test.py     ${FORGEDB})            # switched on by WAL-4
# forge_blackbox(smoke.epoll smoke_test.py       ${FORGEDB} --io epoll) # switched on by CONN-5
