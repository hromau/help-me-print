cmake_minimum_required(VERSION 3.21)

project(DuplexPrintCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DUPLEXPRINT_BUILD_TESTS "Build DuplexPrint C++ tests" ON)

add_subdirectory(cpp/core)
add_subdirectory(cpp/cloud)
add_subdirectory(cpp/platform)
add_subdirectory(cpp/app)

if(DUPLEXPRINT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(cpp/tests)
endif()
