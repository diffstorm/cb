#include "test_common.h"

// Define EnhancedErrorTest fixture
class EnhancedErrorTest : public ::testing::Test {
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
};

// Test enhanced initialization
TEST_F(EnhancedErrorTest, EnhancedInitialization) {
    cb test_buffer;
    CbItem storage[TEST_BUFFER_SIZE_MEDIUM];
    
    // Valid initialization
    EXPECT_EQ(cb_init_ex(&test_buffer, storage, TEST_BUFFER_SIZE_MEDIUM), CB_SUCCESS);
    
    // Null buffer pointer
    EXPECT_EQ(cb_init_ex(nullptr, storage, TEST_BUFFER_SIZE_MEDIUM), CB_ERROR_NULL_POINTER);
    
    // Null storage pointer
    EXPECT_EQ(cb_init_ex(&test_buffer, nullptr, TEST_BUFFER_SIZE_MEDIUM), CB_ERROR_NULL_POINTER);
    
    // Zero size
    EXPECT_EQ(cb_init_ex(&test_buffer, storage, 0), CB_ERROR_INVALID_SIZE);
}

// Test enhanced insert operations
TEST_F(EnhancedErrorTest, EnhancedInsertOperations) {
    // Valid insert
    EXPECT_EQ(cb_insert_ex(&buffer, 42), CB_SUCCESS);
    
    // Fill buffer
    for (int i = 1; i < TEST_BUFFER_SIZE_MEDIUM - 1; i++) {
        EXPECT_EQ(cb_insert_ex(&buffer, i), CB_SUCCESS);
    }
    
    // Buffer full
    EXPECT_EQ(cb_insert_ex(&buffer, 99), CB_ERROR_BUFFER_FULL);
    
    // Enable overwrite mode
    EXPECT_EQ(cb_set_overwrite_ex(&buffer, true), CB_SUCCESS);
    
    // Insert should now succeed with overwrite
    EXPECT_EQ(cb_insert_ex(&buffer, 99), CB_SUCCESS);
    
    // Null buffer pointer
    EXPECT_EQ(cb_insert_ex(nullptr, 42), CB_ERROR_NULL_POINTER);
}

// Test enhanced remove operations
TEST_F(EnhancedErrorTest, EnhancedRemoveOperations) {
    // Empty buffer
    CbItem item;
    EXPECT_EQ(cb_remove_ex(&buffer, &item), CB_ERROR_BUFFER_EMPTY);
    
    // Add an item
    EXPECT_EQ(cb_insert_ex(&buffer, 42), CB_SUCCESS);
    
    // Valid remove
    EXPECT_EQ(cb_remove_ex(&buffer, &item), CB_SUCCESS);
    EXPECT_EQ(item, 42);
    
    // Null buffer pointer
    EXPECT_EQ(cb_remove_ex(nullptr, &item), CB_ERROR_NULL_POINTER);
    
    // Null item pointer
    EXPECT_EQ(cb_remove_ex(&buffer, nullptr), CB_ERROR_NULL_POINTER);
}

// Test enhanced peek operations
TEST_F(EnhancedErrorTest, EnhancedPeekOperations) {
    // Empty buffer
    CbItem item;
    EXPECT_EQ(cb_peek_ex(&buffer, 0, &item), CB_ERROR_INVALID_OFFSET);
    
    // Add items
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(cb_insert_ex(&buffer, i + 1), CB_SUCCESS);
    }
    
    // Valid peek
    EXPECT_EQ(cb_peek_ex(&buffer, 0, &item), CB_SUCCESS);
    EXPECT_EQ(item, 1);
    
    EXPECT_EQ(cb_peek_ex(&buffer, 2, &item), CB_SUCCESS);
    EXPECT_EQ(item, 3);
    
    // Invalid offset
    EXPECT_EQ(cb_peek_ex(&buffer, 5, &item), CB_ERROR_INVALID_OFFSET);
    
    // Null buffer pointer
    EXPECT_EQ(cb_peek_ex(nullptr, 0, &item), CB_ERROR_NULL_POINTER);
    
    // Null item pointer
    EXPECT_EQ(cb_peek_ex(&buffer, 0, nullptr), CB_ERROR_NULL_POINTER);
}

// Test enhanced bulk operations
TEST_F(EnhancedErrorTest, EnhancedBulkOperations) {
    CbItem items[5] = {10, 20, 30, 40, 50};
    CbIndex inserted;
    
    // Valid bulk insert
    EXPECT_EQ(cb_insert_bulk_ex(&buffer, items, 5, &inserted), CB_SUCCESS);
    EXPECT_EQ(inserted, 5);
    
    // Null buffer pointer
    EXPECT_EQ(cb_insert_bulk_ex(nullptr, items, 5, &inserted), CB_ERROR_NULL_POINTER);
    
    // Null items pointer
    EXPECT_EQ(cb_insert_bulk_ex(&buffer, nullptr, 5, &inserted), CB_ERROR_NULL_POINTER);
    
    // Null inserted pointer
    EXPECT_EQ(cb_insert_bulk_ex(&buffer, items, 5, nullptr), CB_ERROR_NULL_POINTER);
    
    // Zero count
    EXPECT_EQ(cb_insert_bulk_ex(&buffer, items, 0, &inserted), CB_ERROR_INVALID_COUNT);
    
    // Bulk remove
    CbItem removed[5];
    CbIndex removed_count;
    
    // Valid bulk remove
    EXPECT_EQ(cb_remove_bulk_ex(&buffer, removed, 5, &removed_count), CB_SUCCESS);
    EXPECT_EQ(removed_count, 5);
    
    // Empty buffer
    EXPECT_EQ(cb_remove_bulk_ex(&buffer, removed, 5, &removed_count), CB_ERROR_BUFFER_EMPTY);
    
    // Null buffer pointer
    EXPECT_EQ(cb_remove_bulk_ex(nullptr, removed, 5, &removed_count), CB_ERROR_NULL_POINTER);
    
    // Null items pointer
    EXPECT_EQ(cb_remove_bulk_ex(&buffer, nullptr, 5, &removed_count), CB_ERROR_NULL_POINTER);
    
    // Null removed_count pointer
    EXPECT_EQ(cb_remove_bulk_ex(&buffer, removed, 5, nullptr), CB_ERROR_NULL_POINTER);
    
    // Zero count
    EXPECT_EQ(cb_remove_bulk_ex(&buffer, removed, 0, &removed_count), CB_ERROR_INVALID_COUNT);
}

// Test enhanced state functions
TEST_F(EnhancedErrorTest, EnhancedStateFunctions) {
    CbIndex free_space;
    CbIndex data_size;
    bool is_valid;
    
    // Valid calls
    EXPECT_EQ(cb_freeSpace_ex(&buffer, &free_space), CB_SUCCESS);
    EXPECT_EQ(free_space, TEST_BUFFER_SIZE_MEDIUM - 1);
    
    EXPECT_EQ(cb_dataSize_ex(&buffer, &data_size), CB_SUCCESS);
    EXPECT_EQ(data_size, 0);
    
    EXPECT_EQ(cb_sanity_check_ex(&buffer, &is_valid), CB_SUCCESS);
    EXPECT_TRUE(is_valid);
    
    // Null buffer pointer
    EXPECT_EQ(cb_freeSpace_ex(nullptr, &free_space), CB_ERROR_NULL_POINTER);
    EXPECT_EQ(cb_dataSize_ex(nullptr, &data_size), CB_ERROR_NULL_POINTER);
    EXPECT_EQ(cb_sanity_check_ex(nullptr, &is_valid), CB_ERROR_NULL_POINTER);
    
    // Null output pointer
    EXPECT_EQ(cb_freeSpace_ex(&buffer, nullptr), CB_ERROR_NULL_POINTER);
    EXPECT_EQ(cb_dataSize_ex(&buffer, nullptr), CB_ERROR_NULL_POINTER);
    EXPECT_EQ(cb_sanity_check_ex(&buffer, nullptr), CB_ERROR_NULL_POINTER);
}

// Test enhanced overwrite control
TEST_F(EnhancedErrorTest, EnhancedOverwriteControl) {
    bool enabled;
    
    // Valid calls
    EXPECT_EQ(cb_set_overwrite_ex(&buffer, true), CB_SUCCESS);
    EXPECT_EQ(cb_get_overwrite_ex(&buffer, &enabled), CB_SUCCESS);
    EXPECT_TRUE(enabled);
    
    EXPECT_EQ(cb_set_overwrite_ex(&buffer, false), CB_SUCCESS);
    EXPECT_EQ(cb_get_overwrite_ex(&buffer, &enabled), CB_SUCCESS);
    EXPECT_FALSE(enabled);
    
    // Null buffer pointer
    EXPECT_EQ(cb_set_overwrite_ex(nullptr, true), CB_ERROR_NULL_POINTER);
    EXPECT_EQ(cb_get_overwrite_ex(nullptr, &enabled), CB_ERROR_NULL_POINTER);
    
    // Null enabled pointer
    EXPECT_EQ(cb_get_overwrite_ex(&buffer, nullptr), CB_ERROR_NULL_POINTER);
}

// Test error string function
TEST_F(EnhancedErrorTest, ErrorStringFunction) {
    EXPECT_STREQ(cb_error_string(CB_SUCCESS), "Success");
    EXPECT_STREQ(cb_error_string(CB_ERROR_NULL_POINTER), "Null pointer argument");
    EXPECT_STREQ(cb_error_string(CB_ERROR_INVALID_SIZE), "Invalid buffer size");
    EXPECT_STREQ(cb_error_string(CB_ERROR_BUFFER_FULL), "Buffer is full");
    EXPECT_STREQ(cb_error_string(CB_ERROR_BUFFER_EMPTY), "Buffer is empty");
    EXPECT_STREQ(cb_error_string(CB_ERROR_INVALID_OFFSET), "Invalid offset");
    EXPECT_STREQ(cb_error_string(CB_ERROR_INVALID_COUNT), "Invalid count parameter");
    EXPECT_STREQ(cb_error_string(CB_ERROR_BUFFER_CORRUPTED), "Buffer integrity check failed");
    EXPECT_STREQ(cb_error_string(CB_ERROR_TIMEOUT), "Operation timed out");
    EXPECT_STREQ(cb_error_string(CB_ERROR_INVALID_PARAMETER), "Invalid parameter value");
    EXPECT_STREQ(cb_error_string((cb_result_t)999), "Unknown error");
}

// Test backward compatibility
TEST_F(EnhancedErrorTest, BackwardCompatibility) {
    // Simple API should work with enhanced implementation
    EXPECT_TRUE(cb_insert(&buffer, 42));
    
    CbItem item;
    EXPECT_TRUE(cb_remove(&buffer, &item));
    EXPECT_EQ(item, 42);
    
    // Simple API should return false for errors
    EXPECT_FALSE(cb_remove(&buffer, &item));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
