set(CMAKE_CXX_STANDARD 20)
project(Cpp-Template)
cmake_minimum_required(VERSION 3.4.1)
set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_C_FLAGS   "-Wall -g")
set(CMAKE_CXX_FLAGS "-Wall -g")

add_subdirectory(src)
