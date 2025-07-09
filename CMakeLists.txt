cmake_minimum_required(VERSION 3.10)
project(circular_buffer LANGUAGES C CXX)

enable_testing()

# === Fixed values for consistent testing across platforms ===
# These override the platform detection headers to ensure consistent behavior
set(CB_TEST_DEFINITIONS
    -DCB_WORD_INDEX_TYPE=uint32_t          # Fixed 32-bit index for consistent testing
    -DCB_ATOMIC_INDEX_TYPE=uint32_t        # Fixed 32-bit atomic index
    -DCB_USE_MEMORY_BARRIERS=1             # Always enable memory barriers for testing
)

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
target_compile_definitions(cb PRIVATE ${CB_TEST_DEFINITIONS})

# === Original Demo (C) ===
add_executable(demo demo.c)
target_link_libraries(demo cb pthread)
target_compile_definitions(demo PRIVATE ${CB_TEST_DEFINITIONS})

# === Mutex Demo (C) ===
add_executable(demo_mutex demo_mutex.c)
target_link_libraries(demo_mutex cb pthread)
target_compile_definitions(demo_mutex PRIVATE ${CB_TEST_DEFINITIONS})

# === Bulk Demo (C) ===
add_executable(demo_bulk demo_bulk.c)
target_link_libraries(demo_bulk cb pthread)
target_compile_definitions(demo_bulk PRIVATE ${CB_TEST_DEFINITIONS})

# === Overwrite Demo (C) ===
add_executable(demo_overwrite demo_overwrite.c)
target_link_libraries(demo_overwrite cb pthread)
target_compile_definitions(demo_overwrite PRIVATE ${CB_TEST_DEFINITIONS})

# === Tests (C++) ===
find_package(GTest REQUIRED)

# Basic tests
add_executable(tests test.cpp)
target_link_libraries(tests cb GTest::gtest_main pthread)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(tests PRIVATE ${CB_TEST_DEFINITIONS})

# Comprehensive tests
add_executable(tests2 tests2.cpp)
target_link_libraries(tests2 cb GTest::gtest_main pthread)
target_include_directories(tests2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(tests2 PRIVATE ${CB_TEST_DEFINITIONS})

include(GoogleTest)
gtest_discover_tests(tests)
gtest_discover_tests(tests2)

install(TARGETS cb 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES cb.h DESTINATION include)
