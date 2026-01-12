cmake_minimum_required(VERSION 3.16)
project(CompilerForNASMCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Include)

file(GLOB_RECURSE SOURCES ${SRC_DIR}/*.cpp)

find_package(FLEX)
if(FLEX_FOUND AND EXISTS "${SRC_DIR}/scanner.lx")
  flex_target(scanner "${SRC_DIR}/scanner.lx" "${CMAKE_CURRENT_BINARY_DIR}/scanner.cpp")
  add_executable(compiler ${SOURCES} ${FLEX_scanner_OUTPUTS})
  target_include_directories(compiler PRIVATE ${INC_DIR} ${CMAKE_CURRENT_BINARY_DIR})
else()
  message(STATUS "Flex not found (or scanner.lx missing) -> using the provided src/scanner.cpp")
  add_executable(compiler ${SOURCES})
  target_include_directories(compiler PRIVATE ${INC_DIR})
endif()
