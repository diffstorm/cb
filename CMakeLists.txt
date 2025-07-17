cmake_minimum_required(VERSION 3.10)
project(cb VERSION 1.2 LANGUAGES C CXX)

# Set C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Configuration options
option(CB_ENABLE_STATISTICS "Enable statistics tracking in circular buffer" ON)

# Add compile definitions based on options
if(CB_ENABLE_STATISTICS)
    add_compile_definitions(CB_ENABLE_STATISTICS=1)
else()
    add_compile_definitions(CB_ENABLE_STATISTICS=0)
endif()

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# Main library
add_library(cb
    src/cb.c
    src/cb.h
    src/cb_wordsize_detect.h
    src/cb_atomicindex_detect.h
    src/cb_memorybarrier_detect.h
    src/cb_atomic_access.h
)

target_include_directories(cb
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Demo programs
add_executable(demo demo/demo.c)
target_link_libraries(demo PRIVATE cb)

add_executable(demo_mutex demo/demo_mutex.c)
target_link_libraries(demo_mutex PRIVATE cb)
if(UNIX)
    target_link_libraries(demo_mutex PRIVATE pthread)
endif()

add_executable(demo_bulk demo/demo_bulk.c)
target_link_libraries(demo_bulk PRIVATE cb)

add_executable(demo_overwrite demo/demo_overwrite.c)
target_link_libraries(demo_overwrite PRIVATE cb)

add_executable(demo_enhanced demo/demo_enhanced.c)
target_link_libraries(demo_enhanced PRIVATE cb)

add_executable(demo_stats demo/demo_stats.c)
target_link_libraries(demo_stats PRIVATE cb)

add_executable(demo_timeout demo/demo_timeout.c)
target_link_libraries(demo_timeout PRIVATE cb)

# Enable testing and add tests directory
enable_testing()
add_subdirectory(tests)
