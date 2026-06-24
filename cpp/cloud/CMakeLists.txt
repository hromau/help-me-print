add_library(duplexprint_cloud INTERFACE)

target_include_directories(duplexprint_cloud
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/cpp/core
)

target_link_libraries(duplexprint_cloud INTERFACE duplexprint_core)
