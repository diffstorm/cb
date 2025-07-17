#include "test_common.h"

// Test basic initialization
TEST_F(CircularBufferTest, Initialization) {
    EXPECT_EQ(cb_dataSize(&buffer), 0);
    EXPECT_EQ(cb_freeSpace(&buffer), TEST_BUFFER_SIZE_MEDIUM - 1);
}

// Test single insert and remove
TEST_F(CircularBufferTest, SingleInsertRemove) {
    CbItem item = 42;
    EXPECT_TRUE(cb_insert(&buffer, item));
    EXPECT_EQ(cb_dataSize(&buffer), 1);
    
    CbItem retrieved;
    EXPECT_TRUE(cb_remove(&buffer, &retrieved));
    EXPECT_EQ(retrieved, item);
    EXPECT_EQ(cb_dataSize(&buffer), 0);
}

// Test filling the buffer
TEST_F(CircularBufferTest, BufferFull) {
    // Fill the buffer (leaving one slot free due to circular buffer design)
    fillBuffer(TEST_BUFFER_SIZE_MEDIUM - 1);
    
    // Buffer should be full
    EXPECT_EQ(cb_freeSpace(&buffer), 0);
    EXPECT_FALSE(cb_insert(&buffer, 99));
    
    // Remove one item
    CbItem item;
    EXPECT_TRUE(cb_remove(&buffer, &item));
    EXPECT_EQ(item, 0);
    
    // Now we should be able to insert again
    EXPECT_TRUE(cb_insert(&buffer, 99));
}

// Test wrap-around behavior
TEST_F(CircularBufferTest, WrapAround) {
    // Fill the buffer
    fillBuffer(TEST_BUFFER_SIZE_MEDIUM - 1);
    
    // Remove half the items
    CbItem item;
    for (int i = 0; i < (TEST_BUFFER_SIZE_MEDIUM - 1) / 2; i++) {
        EXPECT_TRUE(cb_remove(&buffer, &item));
        EXPECT_EQ(item, i);
    }
    
    // Add new items to force wrap-around
    for (int i = 0; i < (TEST_BUFFER_SIZE_MEDIUM - 1) / 2; i++) {
        EXPECT_TRUE(cb_insert(&buffer, 100 + i));
    }
    
    // Verify remaining original items
    for (int i = (TEST_BUFFER_SIZE_MEDIUM - 1) / 2; i < TEST_BUFFER_SIZE_MEDIUM - 1; i++) {
        EXPECT_TRUE(cb_remove(&buffer, &item));
        EXPECT_EQ(item, i);
    }
    
    // Verify new items
    for (int i = 0; i < (TEST_BUFFER_SIZE_MEDIUM - 1) / 2; i++) {
        EXPECT_TRUE(cb_remove(&buffer, &item));
        EXPECT_EQ(item, 100 + i);
    }
    
    // Buffer should be empty now
    EXPECT_EQ(cb_dataSize(&buffer), 0);
}

// Test null pointer handling
TEST_F(CircularBufferTest, NullPointerHandling) {
    CbItem item = 42;
    
    // Null buffer pointer
    EXPECT_FALSE(cb_insert(nullptr, item));
    
    // Null item pointer for remove
    EXPECT_FALSE(cb_remove(&buffer, nullptr));
}

// Define ZeroSizeBufferTest fixture
class ZeroSizeBufferTest : public ::testing::Test {
protected:
    cb buffer;
    CbItem storage[1];
    
    void SetUp() override {
        cb_init(&buffer, storage, 0);
    }
};

// Test zero-size buffer
TEST_F(ZeroSizeBufferTest, ZeroSizeBuffer) {
    CbItem item = 42;
    CbItem retrieved;
    
    // Operations on zero-size buffer should fail
    EXPECT_FALSE(cb_insert(&buffer, item));
    EXPECT_FALSE(cb_remove(&buffer, &retrieved));
    EXPECT_EQ(cb_dataSize(&buffer), 0);
    EXPECT_EQ(cb_freeSpace(&buffer), 0);
}

// Define SmallBufferTest fixture
class SmallBufferTest : public ::testing::Test {
protected:
    cb buffer1;
    cb buffer2;
    CbItem storage1[1];
    CbItem storage2[2];
    
    void SetUp() override {
        cb_init(&buffer1, storage1, 1);
        cb_init(&buffer2, storage2, 2);
    }
};

// Test small buffer sizes
TEST_F(SmallBufferTest, BufferSize1) {
    // Buffer of size 1 can't store any items (needs one empty slot)
    EXPECT_FALSE(cb_insert(&buffer1, 42));
    EXPECT_EQ(cb_freeSpace(&buffer1), 0);
    
    // Buffer of size 2 can store 1 item
    EXPECT_TRUE(cb_insert(&buffer2, 42));
    EXPECT_EQ(cb_freeSpace(&buffer2), 0);
    EXPECT_EQ(cb_dataSize(&buffer2), 1);
    
    // Can't insert more
    EXPECT_FALSE(cb_insert(&buffer2, 43));
    
    // Can remove the item
    CbItem item;
    EXPECT_TRUE(cb_remove(&buffer2, &item));
    EXPECT_EQ(item, 42);
}

// Test sanity check function
TEST_F(CircularBufferTest, SanityCheck) {
    EXPECT_TRUE(cb_sanity_check(&buffer));
    
    // Corrupt the buffer by directly modifying indices
    buffer.in = TEST_BUFFER_SIZE_MEDIUM + 1;  // Invalid index
    EXPECT_FALSE(cb_sanity_check(&buffer));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
