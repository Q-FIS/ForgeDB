# The engine behind include/qfis/engine.hpp (weeks 5-8).
# Until Group A's real engine exists, the stub is used. Group A's week-5 issues
# add the real sources and switch this option off.
option(QFIS_STUB_ENGINE "Use the stub engine (fixed results) instead of the real one" ON)

if(QFIS_STUB_ENGINE)
  set(QFIS_ENGINE_SOURCES src/stub_engine.cpp)
else()
  file(GLOB QFIS_ENGINE_SOURCES CONFIGURE_DEPENDS src/real/*.cpp)
endif()

add_library(qfis_engine STATIC ${QFIS_ENGINE_SOURCES})
target_include_directories(qfis_engine PUBLIC include)
target_link_libraries(qfis_engine PUBLIC Threads::Threads)

add_executable(engine_contract_tests tests/test_engine_contract.cpp)
target_link_libraries(engine_contract_tests PRIVATE qfis_engine GTest::gtest_main)
gtest_discover_tests(engine_contract_tests)
