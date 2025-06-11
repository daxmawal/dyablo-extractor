cmake_minimum_required(VERSION 3.13)
project(extractor)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(extractor src/main.cpp)
target_link_libraries(extractor
  clangTooling
  clangBasic
  clangASTMatchers
)