cmake_minimum_required(VERSION 3.10)
project(MicroFun LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lib)

add_executable(split_tool tools/split_main.cpp)
add_executable(check_tool tools/check_main.cpp)

target_link_libraries(split_tool PRIVATE mapcore)
target_link_libraries(check_tool PRIVATE mapcore)
