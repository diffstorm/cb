/*
    @file    demo_enhanced.c
    @brief   Demonstration of enhanced error handling in cb circular buffer library.

    @date 2023-04-01
    @version 1.1
    @author Eray Ozturk | erayozturk1@gmail.com
    @url github.com/diffstorm
    @license MIT License
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cb.h"

#define BUFFER_SIZE 5

void print_error(const char* operation, cb_result_t result) {
    if (result != CB_SUCCESS) {
        printf("ERROR in %s: %s\n", operation, cb_error_string(result));
    } else {
        printf("SUCCESS: %s completed\n", operation);
    }
}

void demonstrate_enhanced_api() {
    printf("=== Enhanced Error Handling API Demo ===\n\n");
    
    cb buffer;
    CbItem storage[BUFFER_SIZE];
    cb_result_t result;
    
    // 1. Test initialization
    printf("1. Testing initialization:\n");
    result = cb_init_ex(&buffer, storage, BUFFER_SIZE);
    print_error("cb_init_ex", result);
    
    // Test null pointer initialization
    result = cb_init_ex(NULL, storage, BUFFER_SIZE);
    print_error("cb_init_ex with NULL buffer", result);
    
    // Test zero-size initialization
    result = cb_init_ex(&buffer, storage, 0);
    print_error("cb_init_ex with zero size", result);
    printf("\n");
    
    // Re-initialize properly for further tests
    cb_init_ex(&buffer, storage, BUFFER_SIZE);
    
    // 2. Test state functions
    printf("2. Testing state functions:\n");
    CbIndex value;
    result = cb_freeSpace_ex(&buffer, &value);
    print_error("cb_freeSpace_ex", result);
    if (result == CB_SUCCESS) {
        printf("   Free space: %llu\n", value);
    }
    
    result = cb_dataSize_ex(&buffer, &value);
    print_error("cb_dataSize_ex", result);
    if (result == CB_SUCCESS) {
        printf("   Data size: %llu\n", value);
    }
    
    bool isValid;
    result = cb_sanity_check_ex(&buffer, &isValid);
    print_error("cb_sanity_check_ex", result);
    if (result == CB_SUCCESS) {
        printf("   Buffer is %s\n", isValid ? "valid" : "invalid");
    }
    printf("\n");
    
    // 3. Test insert operations
    printf("3. Testing insert operations:\n");
    result = cb_insert_ex(&buffer, 42);
    print_error("cb_insert_ex", result);
    
    result = cb_insert_ex(&buffer, 43);
    print_error("cb_insert_ex", result);
    
    // Test null pointer insert
    result = cb_insert_ex(NULL, 44);
    print_error("cb_insert_ex with NULL buffer", result);
    printf("\n");
    
    // 4. Test remove operations
    printf("4. Testing remove operations:\n");
    CbItem item;
    result = cb_remove_ex(&buffer, &item);
    print_error("cb_remove_ex", result);
    if (result == CB_SUCCESS) {
        printf("   Removed item: %u\n", item);
    }
    
    // Test null pointer remove
    result = cb_remove_ex(&buffer, NULL);
    print_error("cb_remove_ex with NULL output", result);
    printf("\n");
    
    // 5. Test peek operations
    printf("5. Testing peek operations:\n");
    result = cb_peek_ex(&buffer, 0, &item);
    print_error("cb_peek_ex at offset 0", result);
    if (result == CB_SUCCESS) {
        printf("   Peeked item: %u\n", item);
    }
    
    // Test invalid offset
    result = cb_peek_ex(&buffer, 10, &item);
    print_error("cb_peek_ex with invalid offset", result);
    printf("\n");
    
    // 6. Test bulk operations
    printf("6. Testing bulk operations:\n");
    CbItem source[] = {10, 20, 30};
    CbIndex inserted;
    result = cb_insert_bulk_ex(&buffer, source, 3, &inserted);
    print_error("cb_insert_bulk_ex", result);
    if (result == CB_SUCCESS) {
        printf("   Inserted %llu items\n", inserted);
    }
    
    CbItem dest[3];
    CbIndex removed;
    result = cb_remove_bulk_ex(&buffer, dest, 3, &removed);
    print_error("cb_remove_bulk_ex", result);
    if (result == CB_SUCCESS) {
        printf("   Removed %llu items: ", removed);
        for (CbIndex i = 0; i < removed; i++) {
            printf("%u ", dest[i]);
        }
        printf("\n");
    }
    
    // Test invalid count
    result = cb_insert_bulk_ex(&buffer, source, 0, &inserted);
    print_error("cb_insert_bulk_ex with zero count", result);
    printf("\n");
    
    // 7. Test overwrite control
    printf("7. Testing overwrite control:\n");
    result = cb_set_overwrite_ex(&buffer, true);
    print_error("cb_set_overwrite_ex", result);
    
    bool enabled;
    result = cb_get_overwrite_ex(&buffer, &enabled);
    print_error("cb_get_overwrite_ex", result);
    if (result == CB_SUCCESS) {
        printf("   Overwrite mode: %s\n", enabled ? "enabled" : "disabled");
    }
    printf("\n");
    
    // 8. Test buffer full condition with enhanced error reporting
    printf("8. Testing buffer full condition:\n");
    cb_set_overwrite_ex(&buffer, false);  // Disable overwrite
    
    // Fill buffer
    for (int i = 0; i < BUFFER_SIZE - 1; i++) {
        result = cb_insert_ex(&buffer, i);
        if (result != CB_SUCCESS) {
            print_error("filling buffer", result);
            break;
        }
    }
    
    // Try to insert one more (should fail)
    result = cb_insert_ex(&buffer, 99);
    print_error("inserting into full buffer", result);
    printf("\n");
    
    printf("=== Demo completed successfully! ===\n");
}

void demonstrate_zero_size_handling() {
    printf("\n=== Zero-Size Buffer Handling Demo ===\n\n");
    
    cb zero_buffer;
    CbItem zero_storage[1];  // Storage exists but size will be 0
    cb_result_t result;
    
    // Initialize with zero size
    printf("1. Initializing zero-size buffer:\n");
    result = cb_init_ex(&zero_buffer, zero_storage, 0);
    print_error("zero-size initialization", result);
    
    // Test all operations with zero-size buffer
    printf("\n2. Testing operations on zero-size buffer:\n");
    
    CbIndex value;
    result = cb_freeSpace_ex(&zero_buffer, &value);
    print_error("cb_freeSpace_ex on zero-size", result);
    
    result = cb_dataSize_ex(&zero_buffer, &value);
    print_error("cb_dataSize_ex on zero-size", result);
    
    bool isValid;
    result = cb_sanity_check_ex(&zero_buffer, &isValid);
    print_error("cb_sanity_check_ex on zero-size", result);
    
    result = cb_insert_ex(&zero_buffer, 42);
    print_error("cb_insert_ex on zero-size", result);
    
    CbItem item;
    result = cb_remove_ex(&zero_buffer, &item);
    print_error("cb_remove_ex on zero-size", result);
    
    result = cb_peek_ex(&zero_buffer, 0, &item);
    print_error("cb_peek_ex on zero-size", result);
    
    printf("\n=== Zero-size handling completed! ===\n");
}

int main() {
    printf("Enhanced Circular Buffer Error Handling Demo\n");
    printf("============================================\n\n");
    
    demonstrate_enhanced_api();
    demonstrate_zero_size_handling();
    
    printf("\nAll demonstrations completed successfully!\n");
    printf("The enhanced API provides detailed error information\n");
    printf("while maintaining backward compatibility with the original API.\n");
    
    return 0;
}
