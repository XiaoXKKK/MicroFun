cmake_minimum_required(VERSION 3.14)

# Disable GoogleTest's own tests and examples
set(gtest_build_tests OFF CACHE BOOL "" FORCE)
set(gtest_build_samples OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

# Add local GoogleTest directory with explicit binary directory
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/googletest ${CMAKE_BINARY_DIR}/googletest)

# Disable Google Benchmark's own tests and examples
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

# Add local Google Benchmark directory with explicit binary directory
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/googlebenchmark ${CMAKE_BINARY_DIR}/googlebenchmark)              
                                    
enable_testing()

add_executable(performance_test performance_test.cpp)

target_link_libraries(performance_test
    PRIVATE
    mapcore
    gtest
    gmock
    benchmark::benchmark
)

set_target_properties(performance_test
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_test(NAME performance_test COMMAND performance_test)