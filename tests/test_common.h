#ifndef CB_TEST_COMMON_H
#define CB_TEST_COMMON_H

#include <gtest/gtest.h>
#include "cb.h"

// Common buffer sizes for testing
#define TEST_BUFFER_SIZE_TINY   2
#define TEST_BUFFER_SIZE_SMALL  8
#define TEST_BUFFER_SIZE_MEDIUM 32
#define TEST_BUFFER_SIZE_LARGE  128

// Test fixture base class
class CircularBufferTest : public ::testing::Test {
protected:
    cb buffer;
    CbItem storage[TEST_BUFFER_SIZE_MEDIUM];
    
    void SetUp() override {
        cb_init(&buffer, storage, TEST_BUFFER_SIZE_MEDIUM);
    }
    
    void TearDown() override {
        // Reset buffer statistics
        cb_reset_stats(&buffer);
    }
    
    // Helper to fill buffer with sequential data
    void fillBuffer(int count) {
        for (int i = 0; i < count && i < TEST_BUFFER_SIZE_MEDIUM - 1; i++) {
            ASSERT_TRUE(cb_insert(&buffer, i));
        }
    }
    
    // Helper to verify buffer contents
    void verifyBufferContents(int start, int count) {
        CbItem item;
        for (int i = 0; i < count; i++) {
            ASSERT_TRUE(cb_remove(&buffer, &item));
            ASSERT_EQ(item, start + i);
        }
    }
    
    // Helper to verify buffer is empty
    void verifyBufferEmpty() {
        ASSERT_EQ(cb_dataSize(&buffer), 0);
        CbItem item;
        ASSERT_FALSE(cb_remove(&buffer, &item));
    }
};

#endif // CB_TEST_COMMON_H
