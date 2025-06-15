/*
    @file    demo_overwrite.c
    @brief   Overwrite feature usage demo for the cb (circular buffer) library.

    @date 2023-04-01
    @version 1.0
    @author Eray Ozturk | erayozturk1@gmail.com
    @url github.com/diffstorm
    @license MIT License
*/
#include <stdio.h>
#include <stdlib.h>
#include "cb.h"

#define BUFFER_SIZE 8

CbItem storage[BUFFER_SIZE];
cb my_buffer;

void print_buffer_state() {
    printf("Buffer state: [");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (i < cb_dataSize(&my_buffer)) {
            CbItem item;
            cb_peek(&my_buffer, i, &item);
            printf("%d", item);
        } else {
            printf("-");
        }
        
        if (i < BUFFER_SIZE - 1) printf(" ");
    }
    printf("] Free: %lu, Used: %lu\n", 
           cb_freeSpace(&my_buffer), cb_dataSize(&my_buffer));
}

int main() {
    cb_init(&my_buffer, storage, BUFFER_SIZE);
    printf("Overwrite Mode Demo\n");
    printf("Buffer size: %d\n\n", BUFFER_SIZE);

    // Fill buffer normally
    printf("Filling buffer...\n");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        cb_insert(&my_buffer, i);
        printf("Inserted %d: ", i);
        print_buffer_state();
    }

    // Try to insert without overwrite
    printf("\nTry to insert without overwrite...\n");
    bool success = cb_insert(&my_buffer, 99);
    printf("Insert result: %s\n", success ? "Success" : "Failed (expected)");

    // Enable overwrite
    cb_set_overwrite(&my_buffer, true);
    printf("\nOverwrite mode ENABLED\n");

    // Insert with overwrite
    for (int i = 0; i < 5; i++) {
        int value = 100 + i;
        cb_insert(&my_buffer, value);
        printf("Inserted %d: ", value);
        print_buffer_state();
    }

    // Disable overwrite
    cb_set_overwrite(&my_buffer, false);
    printf("\nOverwrite mode DISABLED\n");

    // Remove all items
    printf("\nRemoving all items...\n");
    CbItem item;
    while (cb_remove(&my_buffer, &item)) {
        printf("Removed %d: ", item);
        print_buffer_state();
    }

    // Verify empty
    printf("\nFinal state: ");
    print_buffer_state();
    printf("Overwrite status: %s\n", 
           cb_get_overwrite(&my_buffer) ? "ENABLED" : "DISABLED");
    printf("Sanity check: %s\n", 
           cb_sanity_check(&my_buffer) ? "PASS" : "FAIL");

    return 0;
}