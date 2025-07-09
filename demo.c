/*
    @file    demo.c
    @brief   Stress test demo for the cb (circular buffer) library.
    @details Multi-threaded producer/consumer demo to validate lock-free behavior
            and correctness under high load.

    @date 2023-04-01
    @version 1.0
    @author Eray Ozturk | erayozturk1@gmail.com
    @url github.com/diffstorm
    @license MIT License
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include <time.h>
#include <signal.h>
#include "cb.h"

#define BUFFER_CAPACITY 128U
#define NUM_PRODUCERS   1  // SPSC design requires 1 producer
#define NUM_CONSUMERS   1  // SPSC design requires 1 consumer  
#define ITEMS_PER_THREAD 10000
#define MAX_RETRY_COUNT 1000000  // Maximum retries before giving up
#define TEST_TIMEOUT_SECONDS 30  // Maximum test duration

static cb sharedBuffer;
static CbItem bufferStorage[BUFFER_CAPACITY];

atomic_uint producedCount = 0;
atomic_uint consumedCount = 0;
atomic_uint activeProducers = 0;
atomic_uint activeConsumers = 0;
atomic_bool testTimedOut = false;

void timeout_handler(int sig) {
    printf("\n[TIMEOUT] Test exceeded %d seconds - terminating\n", TEST_TIMEOUT_SECONDS);
    atomic_store(&testTimedOut, true);
}

void *producerThread(void *arg)
{
    const int thread_id = *(int *)arg;
    struct timespec sleepTime = {0, 1000}; // 1 microsecond
    printf("[Producer %d] Started\n", thread_id);
    atomic_fetch_add(&activeProducers, 1);

    for(uint32_t i = 0; i < ITEMS_PER_THREAD && !atomic_load(&testTimedOut); ++i)
    {
        CbItem item = (CbItem)((i + thread_id * ITEMS_PER_THREAD) & 0xFFU);
        uint32_t retry_count = 0;
        bool inserted = false;

        while(!inserted && retry_count < MAX_RETRY_COUNT && !atomic_load(&testTimedOut))
        {
            inserted = cb_insert(&sharedBuffer, item);
            if (!inserted) {
                nanosleep(&sleepTime, NULL);
                retry_count++;
            }
        }

        if (inserted) {
            atomic_fetch_add(&producedCount, 1U);
        } else {
            printf("[Producer %d] FAILED to insert item %d after %d retries\n", 
                   thread_id, i, retry_count);
            break;
        }

        // Periodic progress report
        if((i + 1) % (ITEMS_PER_THREAD / 10) == 0)
        {
            printf("[Producer %d] Progress: %d/%d items\n",
                   thread_id, i + 1, ITEMS_PER_THREAD);
        }
    }

    printf("[Producer %d] Finished\n", thread_id);
    atomic_fetch_sub(&activeProducers, 1);
    return NULL;
}

void *consumerThread(void *arg)
{
    const int thread_id = *(int *)arg;
    struct timespec sleepTime = {0, 1000}; // 1 microsecond
    CbItem item;
    printf("[Consumer %d] Started\n", thread_id);
    atomic_fetch_add(&activeConsumers, 1);

    for(uint32_t i = 0; i < ITEMS_PER_THREAD && !atomic_load(&testTimedOut); ++i)
    {
        uint32_t retry_count = 0;
        bool removed = false;

        while(!removed && retry_count < MAX_RETRY_COUNT && !atomic_load(&testTimedOut))
        {
            removed = cb_remove(&sharedBuffer, &item);
            if (!removed) {
                nanosleep(&sleepTime, NULL);
                retry_count++;
            }
        }

        if (removed) {
            atomic_fetch_add(&consumedCount, 1U);
        } else {
            printf("[Consumer %d] FAILED to remove item %d after %d retries\n", 
                   thread_id, i, retry_count);
            break;
        }

        // Periodic progress report
        if((i + 1) % (ITEMS_PER_THREAD / 10) == 0)
        {
            printf("[Consumer %d] Progress: %d/%d items\n",
                   thread_id, i + 1, ITEMS_PER_THREAD);
        }
    }

    printf("[Consumer %d] Finished\n", thread_id);
    atomic_fetch_sub(&activeConsumers, 1);
    return NULL;
}

int main(void)
{
    // Set up timeout handler
    signal(SIGALRM, timeout_handler);
    alarm(TEST_TIMEOUT_SECONDS);

    cb_init(&sharedBuffer, bufferStorage, BUFFER_CAPACITY);
    printf("\nStarting lock-free circular buffer stress test\n");
    printf("-----------------------------------------------------\n");
    printf("Configuration:\n");
    printf("  Producers: %d (SPSC design)\n", NUM_PRODUCERS);
    printf("  Consumers: %d (SPSC design)\n", NUM_CONSUMERS);
    printf("  Items per thread: %d\n", ITEMS_PER_THREAD);
    printf("  Buffer capacity: %d\n", BUFFER_CAPACITY);
    printf("  Effective capacity: %d (size-1 due to full detection)\n", BUFFER_CAPACITY-1);
    printf("  Total expected items: %d\n", NUM_PRODUCERS * ITEMS_PER_THREAD);
    printf("  Timeout: %d seconds\n\n", TEST_TIMEOUT_SECONDS);

    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    int thread_ids[NUM_PRODUCERS + NUM_CONSUMERS];

    // Create producers
    for(uint32_t i = 0; i < NUM_PRODUCERS; ++i)
    {
        thread_ids[i] = i;
        pthread_create(&producers[i], NULL, producerThread, &thread_ids[i]);
    }

    // Create consumers
    for(uint32_t i = 0; i < NUM_CONSUMERS; ++i)
    {
        thread_ids[NUM_PRODUCERS + i] = i;
        pthread_create(&consumers[i], NULL, consumerThread,
                       &thread_ids[NUM_PRODUCERS + i]);
    }

    // Monitor progress while threads are running
    while((atomic_load(&activeProducers) > 0 || atomic_load(&activeConsumers) > 0) 
          && !atomic_load(&testTimedOut))
    {
        printf("\n[Monitor] Buffer status: %lu/%u (used/total)\n",
               cb_dataSize(&sharedBuffer), BUFFER_CAPACITY);
        printf("[Monitor] Produced: %u, Consumed: %u\n",
               atomic_load(&producedCount), atomic_load(&consumedCount));
        sleep(1);
    }

    // Cancel alarm
    alarm(0);

    // Wait for all threads to finish
    for(uint32_t i = 0; i < NUM_PRODUCERS; ++i)
    {
        pthread_join(producers[i], NULL);
    }

    for(uint32_t i = 0; i < NUM_CONSUMERS; ++i)
    {
        pthread_join(consumers[i], NULL);
    }

    printf("\nTest Results:\n");
    printf("  Expected items: %d\n", NUM_PRODUCERS * ITEMS_PER_THREAD);
    printf("  Produced: %u items\n", atomic_load(&producedCount));
    printf("  Consumed: %u items\n", atomic_load(&consumedCount));
    printf("  Buffer items remaining: %lu\n", cb_dataSize(&sharedBuffer));
    printf("  Test timed out: %s\n", atomic_load(&testTimedOut) ? "YES" : "NO");

    uint32_t expected = NUM_PRODUCERS * ITEMS_PER_THREAD;
    uint32_t produced = atomic_load(&producedCount);
    uint32_t consumed = atomic_load(&consumedCount);

    if(!atomic_load(&testTimedOut) && produced == consumed && consumed == expected)
    {
        printf("\nSUCCESS: All items processed correctly!\n");
        return 0;
    }
    else
    {
        printf("\nFAILURE: Item count mismatch detected!\n");
        if (atomic_load(&testTimedOut)) {
            printf("  - Test exceeded timeout limit\n");
        }
        return 1;
    }
}
