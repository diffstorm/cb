/*
    @file    tests2.cpp
    @brief   Comprehensive GoogleTest unit tests for the cb circular buffer library.
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
#include <cstring>
extern "C" {
#include "cb.h"
}

// =============================================================================
// Test Suite 1: Basic Functionality Tests
// =============================================================================
class BasicFunctionalityTest : public ::testing::Test {
protected:
    static const size_t BUFFER_SIZE = 8;
    cb buffer;
    CbItem storage[BUFFER_SIZE];

    void SetUp() override {
        cb_init(&buffer, storage, BUFFER_SIZE);
    }
};

const size_t BasicFunctionalityTest::BUFFER_SIZE;

TEST_F(BasicFunctionalityTest, Initialization) {
    EXPECT_EQ(buffer.size, BUFFER_SIZE);
    EXPECT_EQ(CB_ATOMIC_LOAD(&buffer.in), 0);
    EXPECT_EQ(CB_ATOMIC_LOAD(&buffer.out), 0);
    EXPECT_EQ(cb_freeSpace(&buffer), BUFFER_SIZE - 1);
    EXPECT_EQ(cb_dataSize(&buffer), 0);
    EXPECT_TRUE(cb_sanity_check(&buffer));
    EXPECT_FALSE(cb_get_overwrite(&buffer));
}

TEST_F(BasicFunctionalityTest, SingleInsertRemove) {
    CbItem item = 42;
    EXPECT_TRUE(cb_insert(&buffer, item));
    EXPECT_EQ(cb_dataSize(&buffer), 1);
    EXPECT_EQ(cb_freeSpace(&buffer), BUFFER_SIZE - 2);
    
    CbItem retrieved;
    EXPECT_TRUE(cb_remove(&buffer, &retrieved));
    EXPECT_EQ(retrieved, item);
    EXPECT_EQ(cb_dataSize(&buffer), 0);
    EXPECT_EQ(cb_freeSpace(&buffer), BUFFER_SIZE - 1);
}

TEST_F(BasicFunctionalityTest, FillAndEmpty) {
    // Fill buffer to capacity
    for (CbItem i = 0; i < BUFFER_SIZE - 1; i++) {
        EXPECT_TRUE(cb_insert(&buffer, i));
        EXPECT_EQ(cb_dataSize(&buffer), i + 1);
    }
    
    // Buffer should be full
    EXPECT_EQ(cb_freeSpace(&buffer), 0);
    EXPECT_FALSE(cb_insert(&buffer, 99));
    
    // Empty buffer
    for (CbItem i = 0; i < BUFFER_SIZE - 1; i++) {
        CbItem retrieved;
        EXPECT_TRUE(cb_remove(&buffer, &retrieved));
        EXPECT_EQ(retrieved, i);
        EXPECT_EQ(cb_dataSize(&buffer), BUFFER_SIZE - 2 - i);
    }
    
    // Buffer should be empty
    EXPECT_EQ(cb_dataSize(&buffer), 0);
    CbItem dummy;
    EXPECT_FALSE(cb_remove(&buffer, &dummy));
}

// =============================================================================
// Test Suite 2: Edge Cases Tests  
// =============================================================================
class EdgeCasesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(EdgeCasesTest, BufferSize1) {
    cb buffer;
    CbItem storage[1];
    cb_init(&buffer, storage, 1);
    
    EXPECT_EQ(cb_freeSpace(&buffer), 0);
    EXPECT_FALSE(cb_insert(&buffer, 42));
}

TEST_F(EdgeCasesTest, BufferSize2) {
    cb buffer;
    CbItem storage[2];
    cb_init(&buffer, storage, 2);
    
    EXPECT_EQ(cb_freeSpace(&buffer), 1);
    EXPECT_TRUE(cb_insert(&buffer, 42));
    EXPECT_EQ(cb_freeSpace(&buffer), 0);
    EXPECT_FALSE(cb_insert(&buffer, 43));
    
    CbItem retrieved;
    EXPECT_TRUE(cb_remove(&buffer, &retrieved));
    EXPECT_EQ(retrieved, 42);
}

TEST_F(EdgeCasesTest, ZeroSizeBuffer) {
    cb buffer;
    CbItem storage[1];
    
    memset(&buffer, 0, sizeof(buffer));
    cb_init(&buffer, storage, 0);
    
    EXPECT_FALSE(cb_sanity_check(&buffer));
}

TEST_F(EdgeCasesTest, NullPointerHandling) {
    cb buffer;
    CbItem storage[5];
    cb_init(&buffer, storage, 5);
    
    EXPECT_FALSE(cb_insert(nullptr, 42));
    EXPECT_EQ(cb_freeSpace(nullptr), 0);
    EXPECT_EQ(cb_dataSize(nullptr), 0);
    EXPECT_FALSE(cb_sanity_check(nullptr));
    
    CbItem dummy;
    EXPECT_FALSE(cb_remove(nullptr, &dummy));
    EXPECT_FALSE(cb_remove(&buffer, nullptr));
}

// =============================================================================
// Test Suite 3: Bulk Operations Tests
// =============================================================================
class BulkOperationsTest : public ::testing::Test {
protected:
    static const size_t BUFFER_SIZE = 10;
    cb buffer;
    CbItem storage[BUFFER_SIZE];

    void SetUp() override {
        cb_init(&buffer, storage, BUFFER_SIZE);
    }
};

const size_t BulkOperationsTest::BUFFER_SIZE;

TEST_F(BulkOperationsTest, BulkInsertBasic) {
    CbItem source[] = {10, 20, 30, 40, 50};
    CbIndex inserted = cb_insert_bulk(&buffer, source, 5);
    
    EXPECT_EQ(inserted, 5);
    EXPECT_EQ(cb_dataSize(&buffer), 5);
    
    for (int i = 0; i < 5; i++) {
        CbItem retrieved;
        EXPECT_TRUE(cb_remove(&buffer, &retrieved));
        EXPECT_EQ(retrieved, source[i]);
    }
}

TEST_F(BulkOperationsTest, BulkRemoveBasic) {
    CbItem source[] = {1, 2, 3, 4, 5, 6, 7};
    cb_insert_bulk(&buffer, source, 7);
    
    CbItem destination[7];
    CbIndex removed = cb_remove_bulk(&buffer, destination, 7);
    
    EXPECT_EQ(removed, 7);
    EXPECT_EQ(cb_dataSize(&buffer), 0);
    
    for (int i = 0; i < 7; i++) {
        EXPECT_EQ(destination[i], source[i]);
    }
}

// =============================================================================
// Test Suite 4: Overwrite Mode Tests
// =============================================================================
class OverwriteModeTest : public ::testing::Test {
protected:
    static const size_t BUFFER_SIZE = 6;
    cb buffer;
    CbItem storage[BUFFER_SIZE];

    void SetUp() override {
        cb_init(&buffer, storage, BUFFER_SIZE);
    }
};

const size_t OverwriteModeTest::BUFFER_SIZE;

TEST_F(OverwriteModeTest, OverwriteModeToggle) {
    EXPECT_FALSE(cb_get_overwrite(&buffer));
    
    cb_set_overwrite(&buffer, true);
    EXPECT_TRUE(cb_get_overwrite(&buffer));
    
    cb_set_overwrite(&buffer, false);
    EXPECT_FALSE(cb_get_overwrite(&buffer));
}

TEST_F(OverwriteModeTest, OverwriteWhenFull) {
    cb_set_overwrite(&buffer, true);
    
    // Fill buffer completely
    for (CbItem i = 0; i < BUFFER_SIZE - 1; i++) {
        EXPECT_TRUE(cb_insert(&buffer, i));
    }
    
    EXPECT_EQ(cb_freeSpace(&buffer), 0);
    
    // Insert more items - should overwrite oldest
    EXPECT_TRUE(cb_insert(&buffer, 100));
    EXPECT_TRUE(cb_insert(&buffer, 101));
    
    EXPECT_EQ(cb_freeSpace(&buffer), 0);
    EXPECT_EQ(cb_dataSize(&buffer), BUFFER_SIZE - 1);
    
    CbItem retrieved;
    EXPECT_TRUE(cb_remove(&buffer, &retrieved));
    EXPECT_EQ(retrieved, 2); // Items 0,1 were overwritten
}

// =============================================================================
// Test Suite 5: Peek Functionality Tests
// =============================================================================
class PeekFunctionalityTest : public ::testing::Test {
protected:
    static const size_t BUFFER_SIZE = 8;
    cb buffer;
    CbItem storage[BUFFER_SIZE];

    void SetUp() override {
        cb_init(&buffer, storage, BUFFER_SIZE);
    }
};

const size_t PeekFunctionalityTest::BUFFER_SIZE;

TEST_F(PeekFunctionalityTest, PeekBasic) {
    CbItem items[] = {10, 20, 30, 40, 50};
    cb_insert_bulk(&buffer, items, 5);
    
    CbItem peeked;
    EXPECT_TRUE(cb_peek(&buffer, 0, &peeked));
    EXPECT_EQ(peeked, 10);
    
    EXPECT_TRUE(cb_peek(&buffer, 2, &peeked));
    EXPECT_EQ(peeked, 30);
    
    EXPECT_TRUE(cb_peek(&buffer, 4, &peeked));
    EXPECT_EQ(peeked, 50);
    
    EXPECT_EQ(cb_dataSize(&buffer), 5);
}

TEST_F(PeekFunctionalityTest, PeekOutOfBounds) {
    CbItem items[] = {1, 2, 3};
    cb_insert_bulk(&buffer, items, 3);
    
    CbItem peeked;
    EXPECT_TRUE(cb_peek(&buffer, 0, &peeked));
    EXPECT_TRUE(cb_peek(&buffer, 2, &peeked));
    
    EXPECT_FALSE(cb_peek(&buffer, 3, &peeked));
    EXPECT_FALSE(cb_peek(&buffer, 10, &peeked));
}

// =============================================================================
// Test Suite 6: Sanity Check Tests
// =============================================================================
class SanityCheckTest : public ::testing::Test {
protected:
    static const size_t BUFFER_SIZE = 8;
    cb buffer;
    CbItem storage[BUFFER_SIZE];

    void SetUp() override {
        cb_init(&buffer, storage, BUFFER_SIZE);
    }
};

const size_t SanityCheckTest::BUFFER_SIZE;

TEST_F(SanityCheckTest, ValidBuffer) {
    EXPECT_TRUE(cb_sanity_check(&buffer));
    
    cb_insert(&buffer, 42);
    EXPECT_TRUE(cb_sanity_check(&buffer));
    
    for (int i = 0; i < BUFFER_SIZE - 2; i++) {
        cb_insert(&buffer, i);
    }
    EXPECT_TRUE(cb_sanity_check(&buffer));
}

TEST_F(SanityCheckTest, NullBuffer) {
    EXPECT_FALSE(cb_sanity_check(nullptr));
}

// =============================================================================
// Test Suite 7: Multi-threaded Tests
// =============================================================================
class MultiThreadedTest : public ::testing::Test {
protected:
    static const size_t BUFFER_SIZE = 32;
    cb buffer;
    CbItem storage[BUFFER_SIZE];
    std::atomic<int> producer_count{0};
    std::atomic<int> consumer_count{0};

    void SetUp() override {
        cb_init(&buffer, storage, BUFFER_SIZE);
        producer_count = 0;
        consumer_count = 0;
    }
};

const size_t MultiThreadedTest::BUFFER_SIZE;

TEST_F(MultiThreadedTest, SimpleProducerConsumer) {
    const int ITEMS_TO_PROCESS = 100;
    const auto TIMEOUT = std::chrono::seconds(5);
    
    std::atomic<bool> test_timeout{false};
    auto start_time = std::chrono::steady_clock::now();
    
    std::thread producer([&]() {
        for (int i = 0; i < ITEMS_TO_PROCESS && !test_timeout; i++) {
            CbItem item = i % 256;
            
            int attempts = 0;
            while (!cb_insert(&buffer, item) && !test_timeout && attempts < 1000) {
                attempts++;
                std::this_thread::yield();
                
                if (attempts % 100 == 0) {
                    auto elapsed = std::chrono::steady_clock::now() - start_time;
                    if (elapsed > TIMEOUT) {
                        test_timeout = true;
                        break;
                    }
                }
            }
            
            if (!test_timeout && attempts < 1000) {
                producer_count++;
            }
        }
    });
    
    std::thread consumer([&]() {
        CbItem item;
        while (consumer_count < ITEMS_TO_PROCESS && !test_timeout) {
            if (cb_remove(&buffer, &item)) {
                consumer_count++;
            } else {
                std::this_thread::yield();
                
                auto elapsed = std::chrono::steady_clock::now() - start_time;
                if (elapsed > TIMEOUT) {
                    test_timeout = true;
                    break;
                }
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    if (!test_timeout) {
        EXPECT_EQ(producer_count.load(), ITEMS_TO_PROCESS);
        EXPECT_EQ(consumer_count.load(), ITEMS_TO_PROCESS);
        EXPECT_EQ(cb_dataSize(&buffer), 0);
    } else {
        FAIL() << "Test timed out. Produced: " << producer_count.load() 
               << ", Consumed: " << consumer_count.load();
    }
}
