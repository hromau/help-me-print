add_executable(duplexprint_core_tests
  workflow_engine_tests.cpp
)

target_link_libraries(duplexprint_core_tests
  PRIVATE
    duplexprint_core
    duplexprint_platform
)

add_test(NAME duplexprint_core_tests COMMAND $<TARGET_FILE:duplexprint_core_tests>)
