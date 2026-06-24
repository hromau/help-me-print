add_library(duplexprint_platform
  filesystem_profile_store.cpp
  printer_profile_resolver.cpp
  printer_identity.cpp
)

target_include_directories(duplexprint_platform
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/cpp/core
)

target_link_libraries(duplexprint_platform PUBLIC duplexprint_core duplexprint_cloud)
