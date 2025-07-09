/*
    @file    test.cpp
    @brief   GoogleTest unit tests for the cb circular buffer library.

    @date 2023-04-01
    @version 1.1
    @author Eray Ozturk | erayozturk1@gmail.com
    @url github.com/diffstorm
    @license MIT License
*/

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
extern "C" {
#include "cb.h"
}

// Test fixture for circular buffer
class CircularBufferTest : public ::testing::Test {
protected:
    static const size_t BUFFER_SIZE = 5;
    cb buffer;
    CbItem storage[BUFFER_SIZE];

    void SetUp() override {
        cb_init(&buffer, storage, BUFFER_SIZE);
    }
};

const size_t CircularBufferTest::BUFFER_SIZE;

// Test basic initialization
TEST_F(CircularBufferTest, Initialization) {
    EXPECT_EQ(buffer.size, BUFFER_SIZE);
    EXPECT_EQ(CB_ATOMIC_LOAD(&buffer.in), 0);
    EXPECT_EQ(CB_ATOMIC_LOAD(&buffer.out), 0);
    EXPECT_EQ(cb_freeSpace(&buffer), BUFFER_SIZE - 1);
    EXPECT_EQ(cb_dataSize(&buffer), 0);
    EXPECT_TRUE(cb_sanity_check(&buffer));
}

// Test single insert/remove
TEST_F(CircularBufferTest, SingleInsertRemove) {
    CbItem item = 42;
    EXPECT_TRUE(cb_insert(&buffer, item));
    EXPECT_EQ(cb_dataSize(&buffer), 1);
    EXPECT_EQ(cb_freeSpace(&buffer), BUFFER_SIZE - 2);

    CbItem output;
    EXPECT_TRUE(cb_remove(&buffer, &output));
    EXPECT_EQ(output, 42);
    EXPECT_EQ(cb_dataSize(&buffer), 0);
    EXPECT_EQ(cb_freeSpace(&buffer), BUFFER_SIZE - 1);
}

// Test buffer full condition
TEST_F(CircularBufferTest, BufferFull) {
    // Fill buffer (size-1 items)
    for (CbItem i = 0; i < BUFFER_SIZE - 1; i++) {
        bool result = cb_insert(&buffer, i);
        EXPECT_TRUE(result);
    }
    EXPECT_EQ(cb_freeSpace(&buffer), 0);
    
    // Should reject new items
    bool critical_result = cb_insert(&buffer, 99);
    EXPECT_FALSE(critical_result);
    
    // Remove one item and insert again
    CbItem temp;
    EXPECT_TRUE(cb_remove(&buffer, &temp));
    EXPECT_TRUE(cb_insert(&buffer, 99));
}

// Test overwrite mode
TEST_F(CircularBufferTest, OverwriteMode) {
    cb_set_overwrite(&buffer, true);
    ASSERT_TRUE(cb_get_overwrite(&buffer));

    // Fill buffer completely (size-1 items since one slot is always reserved)
    for (CbItem i = 0; i < BUFFER_SIZE - 1; i++) {
        EXPECT_TRUE(cb_insert(&buffer, i));
    }
    
    // Insert one more to trigger overwrite
    EXPECT_TRUE(cb_insert(&buffer, 99));
    EXPECT_EQ(cb_dataSize(&buffer), BUFFER_SIZE - 1);

    // Verify oldest item was overwritten
    CbItem output;
    EXPECT_TRUE(cb_remove(&buffer, &output));
    EXPECT_EQ(output, 1);  // First item (0) was overwritten
}

// Test bulk operations
TEST_F(CircularBufferTest, BulkOperations) {
    CbItem source[] = {10, 20, 30};
    CbIndex inserted = cb_insert_bulk(&buffer, source, 3);
    EXPECT_EQ(inserted, 3);
    EXPECT_EQ(cb_dataSize(&buffer), 3);

    CbItem dest[3];
    CbIndex removed = cb_remove_bulk(&buffer, dest, 3);
    EXPECT_EQ(removed, 3);
    EXPECT_EQ(dest[0], 10);
    EXPECT_EQ(dest[1], 20);
    EXPECT_EQ(dest[2], 30);
}

// Test peek functionality
TEST_F(CircularBufferTest, Peek) {
    CbItem items[] = {10, 20, 30};
    cb_insert_bulk(&buffer, items, 3);
    
    CbItem peek_val;
    EXPECT_TRUE(cb_peek(&buffer, 0, &peek_val));
    EXPECT_EQ(peek_val, 10);
    
    EXPECT_TRUE(cb_peek(&buffer, 1, &peek_val));
    EXPECT_EQ(peek_val, 20);
    
    EXPECT_TRUE(cb_peek(&buffer, 2, &peek_val));
    EXPECT_EQ(peek_val, 30);
    
    // Invalid peek
    EXPECT_FALSE(cb_peek(&buffer, 3, &peek_val));
}

// Test wrap-around behavior
TEST_F(CircularBufferTest, WrapAround) {
    // Fill buffer partially
    for (CbItem i = 0; i < 3; i++) {
        cb_insert(&buffer, i);
    }
    // Remove 2 items
    CbItem temp;
    cb_remove(&buffer, &temp);
    cb_remove(&buffer, &temp);

    // Insert until buffer wraps
    for (CbItem i = 10; i < 13; i++) {
        cb_insert(&buffer, i);
    }

    // Verify data - should be 4 items total (1 remaining + 3 new)
    EXPECT_EQ(cb_dataSize(&buffer), 4);
    cb_remove(&buffer, &temp);
    EXPECT_EQ(temp, 2);  // Remaining original item
    
    cb_remove(&buffer, &temp);
    EXPECT_EQ(temp, 10);
    cb_remove(&buffer, &temp);
    EXPECT_EQ(temp, 11);
    cb_remove(&buffer, &temp);
    EXPECT_EQ(temp, 12);
}

// Test sanity checks
TEST_F(CircularBufferTest, SanityChecks) {
    // Valid buffer
    EXPECT_TRUE(cb_sanity_check(&buffer));

    // Invalid buffer (nullptr)
    EXPECT_FALSE(cb_sanity_check(nullptr));

    // Invalid buffer (no storage)
    cb invalid_buffer = buffer;
    invalid_buffer.buf = nullptr;
    EXPECT_FALSE(cb_sanity_check(&invalid_buffer));

    // Invalid indices
    CB_ATOMIC_STORE(&buffer.in, BUFFER_SIZE + 10);
    EXPECT_FALSE(cb_sanity_check(&buffer));
}

// Test edge cases
TEST_F(CircularBufferTest, EdgeCases) {
    // Insert into null buffer
    EXPECT_FALSE(cb_insert(nullptr, 42));
    
    // Remove from null buffer
    CbItem temp;
    EXPECT_FALSE(cb_remove(nullptr, &temp));
    
    // Bulk operations with null pointers
    EXPECT_EQ(cb_insert_bulk(nullptr, &temp, 1), 0);
    EXPECT_EQ(cb_remove_bulk(nullptr, &temp, 1), 0);
    
    // Bulk operations with zero count
    EXPECT_EQ(cb_insert_bulk(&buffer, &temp, 0), 0);
    EXPECT_EQ(cb_remove_bulk(&buffer, &temp, 0), 0);
}
