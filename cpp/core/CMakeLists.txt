add_library(duplexprint_core
  workflow_engine.cpp
)

target_include_directories(duplexprint_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(duplexprint_core PUBLIC cxx_std_20)
