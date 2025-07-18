#include "test_common.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <random>

// Define TimeoutTest fixture
class TimeoutTest : public ::testing::Test {
protected:
    cb buffer;
    CbItem storage[TEST_BUFFER_SIZE_SMALL];
    
    void SetUp() override {
        cb_init(&buffer, storage, TEST_BUFFER_SIZE_SMALL);
    }
    
    void TearDown() override {
        // Reset buffer statistics
        cb_reset_stats(&buffer);
    }
};

// Test immediate success with timeout functions
TEST_F(TimeoutTest, ImmediateSuccess) {
    // Insert should succeed immediately
    EXPECT_TRUE(cb_insert_timeout(&buffer, 42, 0));
    
    // Remove should succeed immediately
    CbItem item;
    EXPECT_TRUE(cb_remove_timeout(&buffer, &item, 0));
    EXPECT_EQ(item, 42);
}

// Test immediate failure with timeout=0
TEST_F(TimeoutTest, DISABLED_ImmediateFailure) {
    // Remove from empty buffer with timeout=0 should fail immediately
    CbItem item;
    EXPECT_FALSE(cb_remove_timeout(&buffer, &item, 0));
    
    // Fill the buffer
    for (int i = 0; i < TEST_BUFFER_SIZE_SMALL - 1; i++) {
        EXPECT_TRUE(cb_insert(&buffer, i));
    }
    
    // Insert to full buffer with timeout=0 should fail immediately
    EXPECT_FALSE(cb_insert_timeout(&buffer, 42, 0));
}

// Test successful timeout operation
TEST_F(TimeoutTest, SuccessfulTimeout) {
    // Fill the buffer
    for (int i = 0; i < TEST_BUFFER_SIZE_SMALL - 1; i++) {
        EXPECT_TRUE(cb_insert(&buffer, i));
    }
    
    // Start a thread that will remove an item after a delay
    std::thread consumer([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        CbItem item;
        EXPECT_TRUE(cb_remove(&buffer, &item));
    });
    
    // This should succeed after the consumer thread removes an item
    EXPECT_TRUE(cb_insert_timeout(&buffer, 42, 100));
    
    consumer.join();
}

// Test timeout expiration
TEST_F(TimeoutTest, DISABLED_TimeoutExpiration) {
    // Fill the buffer
    for (int i = 0; i < TEST_BUFFER_SIZE_SMALL - 1; i++) {
        EXPECT_TRUE(cb_insert(&buffer, i));
    }
    
    // This should timeout after 50ms
    auto start = std::chrono::steady_clock::now();
    EXPECT_FALSE(cb_insert_timeout(&buffer, 42, 50));
    auto end = std::chrono::steady_clock::now();
    
    // Verify that we waited at least 50ms
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_GE(duration, 50);
    
    // Verify the error was a timeout
    cb_error_info_t error = cb_get_last_error(&buffer);
    EXPECT_EQ(error.code, CB_ERROR_TIMEOUT);
}

// Test extended API
TEST_F(TimeoutTest, ExtendedAPI) {
    // Test insert_timeout_ex
    cb_result_t result = cb_insert_timeout_ex(&buffer, 42, 0);
    EXPECT_EQ(result, CB_SUCCESS);
    
    // Test remove_timeout_ex
    CbItem item;
    result = cb_remove_timeout_ex(&buffer, &item, 0);
    EXPECT_EQ(result, CB_SUCCESS);
    EXPECT_EQ(item, 42);
    
    // Test timeout expiration with extended API
    result = cb_remove_timeout_ex(&buffer, &item, 50);
    EXPECT_EQ(result, CB_ERROR_TIMEOUT);
}

// Test error information
TEST_F(TimeoutTest, ErrorInfo) {
    // Clear any previous errors
    cb_clear_error(&buffer);
    
    // Cause a timeout error
    CbItem item;
    EXPECT_FALSE(cb_remove_timeout(&buffer, &item, 50));
    
    // Check error information
    cb_error_info_t error = cb_get_last_error(&buffer);
    EXPECT_EQ(error.code, CB_ERROR_TIMEOUT);
    EXPECT_STREQ(error.function, "cb_remove_timeout");
    EXPECT_STREQ(error.parameter, "timeout_ms");
    EXPECT_GT(error.line, 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
