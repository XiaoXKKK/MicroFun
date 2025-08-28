cmake_minimum_required(VERSION 3.14)
                                    
include(FetchContent)               

# Disable GoogleTest's own tests and examples
set(gtest_build_tests OFF CACHE BOOL "" FORCE)
set(gtest_build_samples OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
                    
# Google Test                       
FetchContent_Declare(               
  googletest                        
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main                      
)                                   
                                    
FetchContent_MakeAvailable(googletest)                   

# Disable Google Benchmark's own tests and examples
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
                                    
# Google Benchmark                  
FetchContent_Declare(               
  googlebenchmark                   
  GIT_REPOSITORY https://github.com/google/benchmark.git 
  GIT_TAG main                      
)                                   
                                    
FetchContent_MakeAvailable(googlebenchmark)              
                                    
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