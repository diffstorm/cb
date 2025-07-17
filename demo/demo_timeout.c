/*
    @file        demo_timeout.c
    @brief       Demonstration of timeout functionality in circular buffer
    @details     Shows how to use the timeout functions for producer-consumer scenarios
    @date        2025-07-16
    @version     1.0
    @author      Eray Ozturk
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "cb.h"

#define BUFFER_SIZE 10
#define PRODUCER_ITEMS 20
#define CONSUMER_TIMEOUT_MS 100
#define PRODUCER_TIMEOUT_MS 50

CbItem buffer[BUFFER_SIZE];
cb circular_buffer;

/* Producer thread function */
void* producer_thread(void* arg) {
    int i;
    int success_count = 0;
    int timeout_count = 0;
    
    printf("Producer: Starting to produce %d items\n", PRODUCER_ITEMS);
    
    for (i = 0; i < PRODUCER_ITEMS; i++) {
        CbItem item = (CbItem)i;
        
        /* Try to insert with timeout */
        if (cb_insert_timeout(&circular_buffer, item, PRODUCER_TIMEOUT_MS)) {
            printf("Producer: Inserted item %d\n", i);
            success_count++;
        } else {
            cb_error_info_t error = cb_get_last_error(&circular_buffer);
            if (error.code == CB_ERROR_TIMEOUT) {
                printf("Producer: Timeout inserting item %d\n", i);
                timeout_count++;
            } else {
                printf("Producer: Error inserting item %d: %s\n", i, cb_error_string(error.code));
            }
            
            /* Sleep a bit to let consumer catch up */
            usleep(10000); /* 10ms */
        }
    }
    
    printf("Producer: Finished. Inserted %d items, %d timeouts\n", success_count, timeout_count);
    return NULL;
}

/* Consumer thread function */
void* consumer_thread(void* arg) {
    int i;
    int success_count = 0;
    int timeout_count = 0;
    CbItem item;
    
    printf("Consumer: Starting to consume items\n");
    
    /* Consume items with random delays */
    for (i = 0; i < PRODUCER_ITEMS; i++) {
        /* Add some random delay to simulate processing */
        usleep((rand() % 20) * 1000); /* 0-20ms */
        
        /* Try to remove with timeout */
        if (cb_remove_timeout(&circular_buffer, &item, CONSUMER_TIMEOUT_MS)) {
            printf("Consumer: Removed item %d\n", (int)item);
            success_count++;
        } else {
            cb_error_info_t error = cb_get_last_error(&circular_buffer);
            if (error.code == CB_ERROR_TIMEOUT) {
                printf("Consumer: Timeout waiting for item\n");
                timeout_count++;
            } else {
                printf("Consumer: Error removing item: %s\n", cb_error_string(error.code));
            }
        }
    }
    
    printf("Consumer: Finished. Consumed %d items, %d timeouts\n", success_count, timeout_count);
    return NULL;
}

int main() {
    pthread_t producer, consumer;
    cb_stats_t stats;
    
    /* Initialize the buffer */
    cb_init(&circular_buffer, buffer, BUFFER_SIZE);
    
    /* Create producer and consumer threads */
    pthread_create(&producer, NULL, producer_thread, NULL);
    pthread_create(&consumer, NULL, consumer_thread, NULL);
    
    /* Wait for threads to finish */
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    
    /* Display statistics */
    stats = cb_get_stats(&circular_buffer);
    printf("\nBuffer Statistics:\n");
    printf("Peak Usage: %llu\n", stats.peak_usage);
    printf("Total Inserts: %llu\n", stats.total_inserts);
    printf("Total Removes: %llu\n", stats.total_removes);
    printf("Overflow Count: %llu\n", stats.overflow_count);
    printf("Underflow Count: %llu\n", stats.underflow_count);
    
    return 0;
}
