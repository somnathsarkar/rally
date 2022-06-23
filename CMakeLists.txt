cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)

project(rally)

add_subdirectory(external)
add_subdirectory(tools)
add_subdirectory(rally)
add_subdirectory(examples)
add_subdirectory(tests)