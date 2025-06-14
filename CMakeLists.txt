cmake_minimum_required(VERSION 3.10)
project(circular_buffer LANGUAGES C CXX)

enable_testing()

# === Library ===
add_library(cb STATIC cb.c)
target_include_directories(cb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# === Original Demo (C) ===
add_executable(demo demo.c)
target_link_libraries(demo cb pthread)

# === Mutex Demo (C) ===
add_executable(demo_mutex demo_mutex.c)
target_link_libraries(demo_mutex cb pthread)

# === Tests (C++) ===
find_package(GTest REQUIRED)

add_executable(tests test.cpp)
target_link_libraries(tests cb GTest::gtest_main pthread)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(tests)
