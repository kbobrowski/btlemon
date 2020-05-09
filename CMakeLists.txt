cmake_minimum_required(VERSION 3.11)
project(btlemon C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_library(btlemon src/lib.c)
add_executable(btlemonrun src/main.c)
add_executable(example examples/example.c)

target_link_libraries(btlemonrun btlemon)
target_link_libraries(example btlemon)
