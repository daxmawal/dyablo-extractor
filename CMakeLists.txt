cmake_minimum_required(VERSION 3.14)
project(FunctionExtractor)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Clang REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories(/local/home/jd258565/Workspace/apex-kokkos-tuning/install/include/kokkos)

add_executable(function_extractor src/main.cpp)

target_link_libraries(function_extractor
  clangTooling
  clangBasic
  clangASTMatchers
)
