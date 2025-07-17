#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "cb.h"

#define BUFFER_SIZE 10
#define PRODUCER_ITEMS 50
#define CONSUMER_ITEMS 45

CbItem buffer_storage[BUFFER_SIZE];
cb buffer;

void print_stats(const char* label) {
    cb_stats_t stats = cb_get_stats(&buffer);
    printf("\n%s:\n", label);
    printf("  Peak Usage:      %llu\n", stats.peak_usage);
    printf("  Total Inserts:   %llu\n", stats.total_inserts);
    printf("  Total Removes:   %llu\n", stats.total_removes);
    printf("  Overflow Count:  %llu\n", stats.overflow_count);
    printf("  Underflow Count: %llu\n", stats.underflow_count);
}

void* producer_thread(void* arg) {
    (void)arg; /* Unused parameter */
    
    for (int i = 0; i < PRODUCER_ITEMS; i++) {
        CbItem item = (CbItem)i;
        if (!cb_insert(&buffer, item)) {
            printf("P: Buffer full, couldn't insert item %d\n", i);
        } else {
            printf("P: Inserted item %d\n", i);
        }
        usleep(10000); /* 10ms */
    }
    
    return NULL;
}

void* consumer_thread(void* arg) {
    (void)arg; /* Unused parameter */
    
    for (int i = 0; i < CONSUMER_ITEMS; i++) {
        CbItem item;
        if (!cb_remove(&buffer, &item)) {
            printf("C: Buffer empty, couldn't remove item\n");
        } else {
            printf("C: Removed item %d\n", item);
        }
        usleep(12000); /* 12ms - slightly slower than producer */
    }
    
    return NULL;
}

int main() {
    pthread_t producer, consumer;
    
    printf("Circular Buffer Statistics Demo\n");
    printf("===============================\n");
    
    /* Initialize buffer */
    cb_init(&buffer, buffer_storage, BUFFER_SIZE);
    
    /* Print initial stats */
    print_stats("Initial Stats");
    
    /* Create producer and consumer threads */
    pthread_create(&producer, NULL, producer_thread, NULL);
    pthread_create(&consumer, NULL, consumer_thread, NULL);
    
    /* Wait for threads to complete */
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    
    /* Print final stats */
    print_stats("Final Stats");
    
    /* Reset stats */
    cb_reset_stats(&buffer);
    print_stats("After Reset");
    
    return 0;
}
