cmake_minimum_required(VERSION 3.10)
project(circular_buffer LANGUAGES C CXX)

enable_testing()

# === Library ===
add_library(cb STATIC 
    cb.c
    cb_atomic_access.h
    cb_atomicindex_detect.h
    cb_memorybarrier_detect.h
    cb_wordsize_detect.h
    cb.h
)
target_include_directories(cb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# === Original Demo (C) ===
add_executable(demo demo.c)
target_link_libraries(demo cb pthread)

# === Mutex Demo (C) ===
add_executable(demo_mutex demo_mutex.c)
target_link_libraries(demo_mutex cb pthread)

# === Bulk Demo (C) ===
add_executable(demo_bulk demo_bulk.c)
target_link_libraries(demo_bulk cb pthread)

# === Overwrite Demo (C) ===
add_executable(demo_overwrite demo_overwrite.c)
target_link_libraries(demo_overwrite cb pthread)

# === Tests (C++) ===
find_package(GTest REQUIRED)

add_executable(tests test.cpp)
target_link_libraries(tests cb GTest::gtest_main pthread)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(tests)

install(TARGETS cb 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES cb.h DESTINATION include)
