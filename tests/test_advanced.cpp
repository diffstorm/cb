#include "test_common.h"
#include <thread>
#include <atomic>

// Test overwrite mode
TEST_F(CircularBufferTest, OverwriteMode) {
    // Fill the buffer
    fillBuffer(TEST_BUFFER_SIZE_MEDIUM - 1);
    
    // Enable overwrite mode
    cb_set_overwrite(&buffer, true);
    EXPECT_TRUE(cb_get_overwrite(&buffer));
    
    // Insert should now succeed even when buffer is full
    EXPECT_TRUE(cb_insert(&buffer, 99));
    
    // First item should be overwritten
    CbItem item;
    EXPECT_TRUE(cb_remove(&buffer, &item));
    EXPECT_EQ(item, 1);  // First item (0) was overwritten
    
    // Disable overwrite mode
    cb_set_overwrite(&buffer, false);
    EXPECT_FALSE(cb_get_overwrite(&buffer));
}

// Test peek functionality
TEST_F(CircularBufferTest, PeekFunctionality) {
    // Add some items
    fillBuffer(5);
    
    // Peek at items without removing
    CbItem item;
    EXPECT_TRUE(cb_peek(&buffer, 0, &item));
    EXPECT_EQ(item, 0);  // First item
    
    EXPECT_TRUE(cb_peek(&buffer, 2, &item));
    EXPECT_EQ(item, 2);  // Third item
    
    EXPECT_TRUE(cb_peek(&buffer, 4, &item));
    EXPECT_EQ(item, 4);  // Fifth item
    
    // Out of bounds peek should fail
    EXPECT_FALSE(cb_peek(&buffer, 5, &item));
    
    // Buffer should still have all items
    EXPECT_EQ(cb_dataSize(&buffer), 5);
    
    // Remove items and verify
    verifyBufferContents(0, 5);
}

// Test bulk insert
TEST_F(CircularBufferTest, BulkInsert) {
    CbItem items[5] = {10, 20, 30, 40, 50};
    
    // Insert multiple items at once
    CbIndex inserted = cb_insert_bulk(&buffer, items, 5);
    EXPECT_EQ(inserted, 5);
    EXPECT_EQ(cb_dataSize(&buffer), 5);
    
    // Verify items
    CbItem item;
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(cb_remove(&buffer, &item));
        EXPECT_EQ(item, items[i]);
    }
}

// Test bulk remove
TEST_F(CircularBufferTest, BulkRemove) {
    // Add items
    fillBuffer(5);
    
    // Remove multiple items at once
    CbItem items[5];
    EXPECT_EQ(cb_remove_bulk(&buffer, items, 5), 5);
    EXPECT_EQ(cb_dataSize(&buffer), 0);
    
    // Verify removed items
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(items[i], i);
    }
}

// Test bulk operations with wrap-around
TEST_F(CircularBufferTest, BulkWrapAround) {
    // Fill buffer partially
    fillBuffer(5);
    
    // Remove some items
    CbItem item;
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(cb_remove(&buffer, &item));
    }
    
    // Add more items to force wrap-around
    CbItem items[7] = {101, 102, 103, 104, 105, 106, 107};
    EXPECT_EQ(cb_insert_bulk(&buffer, items, 7), 7);
    
    // Verify remaining original items
    for (int i = 0; i < 2; i++) {
        EXPECT_TRUE(cb_remove(&buffer, &item));
        EXPECT_EQ(item, i + 3);  // Items 3 and 4
    }
    
    // Verify new items
    for (int i = 0; i < 7; i++) {
        EXPECT_TRUE(cb_remove(&buffer, &item));
        EXPECT_EQ(item, 101 + i);
    }
}

// Test multi-threaded producer-consumer
TEST_F(CircularBufferTest, MultiThreaded) {
    const int ITEMS_TO_PRODUCE = 100;
    std::atomic<int> items_consumed(0);
    std::atomic<bool> producer_done(false);
    
    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < ITEMS_TO_PRODUCE; i++) {
            while (!cb_insert(&buffer, i)) {
                // Buffer full, yield to consumer
                std::this_thread::yield();
            }
        }
        producer_done = true;
    });
    
    // Consumer thread
    std::thread consumer([&]() {
        CbItem item;
        while (!producer_done || items_consumed < ITEMS_TO_PRODUCE) {
            if (cb_remove(&buffer, &item)) {
                // Just verify we can remove items successfully
                items_consumed++;
            } else {
                // Buffer empty, yield to producer
                std::this_thread::yield();
            }
        }
    });
    
    // Wait for threads to complete
    producer.join();
    consumer.join();
    
    // Verify all items were consumed
    EXPECT_EQ(items_consumed, ITEMS_TO_PRODUCE);
    EXPECT_EQ(cb_dataSize(&buffer), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
