/*
    @file    demo_bulk.c
    @brief   Bulk operations demo for the cb (circular buffer) library.

    @date 2023-04-01
    @version 1.0
    @author Eray Ozturk | erayozturk1@gmail.com
    @url github.com/diffstorm
    @license MIT License
*/

#include <stdio.h>
#include <stdlib.h>
#include "cb.h"

#define BUFFER_SIZE 32
#define TEST_DATA_SIZE 50

CbItem storage[BUFFER_SIZE];
cb my_buffer;

int main() {
    cb_init(&my_buffer, storage, BUFFER_SIZE);
    printf("Bulk Operations Demo\n");
    printf("Buffer size: %d, Test data size: %d\n\n", BUFFER_SIZE, TEST_DATA_SIZE);

    // Generate test data
    CbItem test_data[TEST_DATA_SIZE];
    for (int i = 0; i < TEST_DATA_SIZE; i++) {
        test_data[i] = i;
    }

    // Bulk insert
    size_t inserted = cb_insert_bulk(&my_buffer, test_data, TEST_DATA_SIZE);
    printf("Inserted %zu items (expected: %d)\n", inserted, BUFFER_SIZE - 1);
    printf("Free space: %llu, Data size: %llu\n", 
           cb_freeSpace(&my_buffer), cb_dataSize(&my_buffer));

    // Bulk remove
    CbItem received[BUFFER_SIZE];
    size_t removed = cb_remove_bulk(&my_buffer, received, BUFFER_SIZE);
    printf("\nRemoved %zu items\n", removed);
    
    // Verify received data
    printf("\nFirst 10 received items: ");
    for (int i = 0; i < (removed < 10 ? removed : 10); i++) {
        printf("%d ", received[i]);
    }
    
    // Insert second batch with partial insert
    printf("\n\nInserting second batch (partial)...\n");
    inserted = cb_insert_bulk(&my_buffer, test_data, 10);
    printf("Inserted %zu items\n", inserted);
    
    // Peek at data
    CbItem peek_item;
    if (cb_peek(&my_buffer, 0, &peek_item)) {
        printf("Peeked first item: %d\n", peek_item);
    }
    
    // Remove partial batch
    removed = cb_remove_bulk(&my_buffer, received, 5);
    printf("Removed %zu items\n", removed);
    
    // Verify buffer state
    printf("\nFinal state - Free space: %llu, Data size: %llu\n", 
           cb_freeSpace(&my_buffer), cb_dataSize(&my_buffer));
    
    // Sanity check
    bool sanity_ok = cb_sanity_check(&my_buffer);
    printf("\nBuffer sanity check: %s\n", sanity_ok ? "PASS" : "FAIL");

    return sanity_ok ? 0 : 1;
}