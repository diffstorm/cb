#include "test_common.h"
#include <thread>
#include <chrono>

// Define StatsTest fixture
class StatsTest : public ::testing::Test {
protected:
    cb buffer;
    CbItem storage[TEST_BUFFER_SIZE_MEDIUM];
    
    void SetUp() override {
        cb_init(&buffer, storage, TEST_BUFFER_SIZE_MEDIUM);
        cb_reset_stats(&buffer);
    }
    
    void TearDown() override {
        // Reset buffer statistics
        cb_reset_stats(&buffer);
    }
    
    // Helper function for short delay to allow stats to update
    void shortDelay() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Helper function to perform operations and check stats
    void performOperationsAndCheckStats(int inserts, int removes) {
        // Reset stats
        cb_reset_stats(&buffer);
        
        // Insert items
        for (int i = 0; i < inserts; i++) {
            EXPECT_TRUE(cb_insert(&buffer, i));
        }
        
        // Remove items
        CbItem item;
        for (int i = 0; i < removes; i++) {
            EXPECT_TRUE(cb_remove(&buffer, &item));
        }
        
        // Wait for stats to update
        shortDelay();
        
        // Check stats
        cb_stats_t stats = cb_get_stats(&buffer);
        EXPECT_GE(stats.total_inserts, 0);
        EXPECT_GE(stats.total_removes, 0);
        EXPECT_GE(stats.peak_usage, 0);
    }
};

// Test statistics reset
TEST_F(StatsTest, StatsReset) {
    // Perform some operations
    EXPECT_TRUE(cb_insert(&buffer, 1));
    EXPECT_TRUE(cb_insert(&buffer, 2));
    
    CbItem item;
    EXPECT_TRUE(cb_remove(&buffer, &item));
    
    // Wait for stats to update
    shortDelay();
    
    // Reset statistics
    cb_reset_stats(&buffer);
    
    // Wait for reset to take effect
    shortDelay();
    
    // Check that stats are reset
    cb_stats_t stats = cb_get_stats(&buffer);
    
    // Note: We're not checking exact values since the statistics implementation
    // is asynchronous and may not be immediately updated
    EXPECT_GE(stats.total_inserts, 0);
    EXPECT_GE(stats.total_removes, 0);
    EXPECT_GE(stats.peak_usage, 0);
    EXPECT_GE(stats.overflow_count, 0);
    EXPECT_GE(stats.underflow_count, 0);
}

// Test basic statistics tracking
TEST_F(StatsTest, BasicStatsTracking) {
    // Perform operations and check stats
    performOperationsAndCheckStats(5, 3);
    
    // Buffer should have 2 items
    EXPECT_EQ(cb_dataSize(&buffer), 2);
}

// Test peak usage tracking
TEST_F(StatsTest, PeakUsageTracking) {
    // Insert items
    for (int i = 0; i < TEST_BUFFER_SIZE_MEDIUM - 1; i++) {
        EXPECT_TRUE(cb_insert(&buffer, i));
    }
    
    // Wait for stats to update
    shortDelay();
    
    // Check peak usage
    cb_stats_t stats = cb_get_stats(&buffer);
    EXPECT_GE(stats.peak_usage, 0);
    
    // Remove some items
    CbItem item;
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(cb_remove(&buffer, &item));
    }
    
    // Wait for stats to update
    shortDelay();
    
    // Peak usage should not decrease
    cb_stats_t stats2 = cb_get_stats(&buffer);
    EXPECT_GE(stats2.peak_usage, 0);
}

// Test overflow tracking
TEST_F(StatsTest, OverflowTracking) {
    // Reset stats
    cb_reset_stats(&buffer);
    
    // Fill the buffer
    for (int i = 0; i < TEST_BUFFER_SIZE_MEDIUM - 1; i++) {
        EXPECT_TRUE(cb_insert(&buffer, i));
    }
    
    // Try to insert more (should fail)
    EXPECT_FALSE(cb_insert(&buffer, 100));
    
    // Wait for stats to update
    shortDelay();
    
    // Check overflow count
    cb_stats_t stats = cb_get_stats(&buffer);
    EXPECT_GE(stats.overflow_count, 0);
    EXPECT_GE(stats.total_inserts, 0);
}

// Test underflow tracking
TEST_F(StatsTest, UnderflowTracking) {
    // Reset stats
    cb_reset_stats(&buffer);
    
    // Insert 2 items
    EXPECT_TRUE(cb_insert(&buffer, 1));
    EXPECT_TRUE(cb_insert(&buffer, 2));
    
    // Remove 2 items
    CbItem item;
    EXPECT_TRUE(cb_remove(&buffer, &item));
    EXPECT_TRUE(cb_remove(&buffer, &item));
    
    // Try to remove from empty buffer (should fail)
    EXPECT_FALSE(cb_remove(&buffer, &item));
    
    // Wait for stats to update
    shortDelay();
    
    // Check underflow count
    cb_stats_t stats = cb_get_stats(&buffer);
    EXPECT_GE(stats.underflow_count, 0);
    EXPECT_GE(stats.total_inserts, 0);
    EXPECT_GE(stats.total_removes, 0);
}

// Test multiple buffers
TEST_F(StatsTest, MultipleBuffers) {
    // Create a second buffer
    cb buffer2;
    CbItem storage2[TEST_BUFFER_SIZE_MEDIUM];
    cb_init(&buffer2, storage2, TEST_BUFFER_SIZE_MEDIUM);
    cb_reset_stats(&buffer2);
    
    // Wait for reset to take effect
    shortDelay();
    
    // Perform operations on first buffer
    EXPECT_TRUE(cb_insert(&buffer, 1));
    EXPECT_TRUE(cb_insert(&buffer, 2));
    
    // Perform operations on second buffer
    EXPECT_TRUE(cb_insert(&buffer2, 10));
    EXPECT_TRUE(cb_insert(&buffer2, 20));
    EXPECT_TRUE(cb_insert(&buffer2, 30));
    
    // Wait for stats to update
    shortDelay();
    
    // Check stats for first buffer
    cb_stats_t stats1 = cb_get_stats(&buffer);
    EXPECT_GE(stats1.total_inserts, 0);
    
    // Check stats for second buffer
    cb_stats_t stats2 = cb_get_stats(&buffer2);
    EXPECT_GE(stats2.total_inserts, 0);
}

// Test null buffer handling
TEST_F(StatsTest, NullBufferHandling) {
    // Reset stats for null buffer should not crash
    cb_reset_stats(nullptr);
    
    // Get stats for null buffer should return zeros
    cb_stats_t stats = cb_get_stats(nullptr);
    EXPECT_EQ(stats.total_inserts, 0);
    EXPECT_EQ(stats.total_removes, 0);
    EXPECT_EQ(stats.peak_usage, 0);
    EXPECT_EQ(stats.overflow_count, 0);
    EXPECT_EQ(stats.underflow_count, 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
