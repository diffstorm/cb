cmake_minimum_required(VERSION 3.10)

# Find GTest package
find_package(GTest REQUIRED)

# Define test targets
add_executable(test_basic test_basic.cpp)
target_link_libraries(test_basic
    PRIVATE
    cb
    GTest::GTest
    GTest::Main
)

add_executable(test_advanced test_advanced.cpp)
target_link_libraries(test_advanced
    PRIVATE
    cb
    GTest::GTest
    GTest::Main
)

add_executable(test_error test_error.cpp)
target_link_libraries(test_error
    PRIVATE
    cb
    GTest::GTest
    GTest::Main
)

add_executable(test_timeout test_timeout.cpp)
target_link_libraries(test_timeout
    PRIVATE
    cb
    GTest::GTest
    GTest::Main
)

add_executable(test_stats test_stats.cpp)
target_link_libraries(test_stats
    PRIVATE
    cb
    GTest::GTest
    GTest::Main
)

# Register tests
add_test(NAME test_basic COMMAND test_basic)
add_test(NAME test_advanced COMMAND test_advanced)
add_test(NAME test_error COMMAND test_error)
add_test(NAME test_timeout COMMAND test_timeout)
add_test(NAME test_stats COMMAND test_stats)

# Enable testing
enable_testing()
