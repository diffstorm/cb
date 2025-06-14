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
#include "cb.h"

#define BUFFER_CAPACITY 128U
#define NUM_PRODUCERS   2
#define NUM_CONSUMERS   2
#define ITEMS_PER_THREAD 100000

static cb sharedBuffer;
static CbItem bufferStorage[BUFFER_CAPACITY];

atomic_uint producedCount = 0;
atomic_uint consumedCount = 0;
atomic_uint activeProducers = 0;
atomic_uint activeConsumers = 0;

void *producerThread(void *arg)
{
    const int thread_id = *(int *)arg;
    struct timespec sleepTime = {0, 1000}; // 1 microsecond
    printf("[Producer %d] Started\n", thread_id);
    atomic_fetch_add(&activeProducers, 1);

    for(uint32_t i = 0; i < ITEMS_PER_THREAD; ++i)
    {
        CbItem item = (CbItem)((i + thread_id * ITEMS_PER_THREAD) & 0xFFU);

        while(!cb_insert(&sharedBuffer, item))
        {
            nanosleep(&sleepTime, NULL); // Brief yield instead of busy-wait
        }

        atomic_fetch_add(&producedCount, 1U);

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

    for(uint32_t i = 0; i < ITEMS_PER_THREAD; ++i)
    {
        while(!cb_remove(&sharedBuffer, &item))
        {
            nanosleep(&sleepTime, NULL); // Brief yield instead of busy-wait
        }

        atomic_fetch_add(&consumedCount, 1U);

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
    cb_init(&sharedBuffer, bufferStorage, BUFFER_CAPACITY);
    printf("\nStarting lock-free circular buffer stress test\n");
    printf("---------------------------------------------\n");
    printf("Configuration:\n");
    printf("  Producers: %d\n", NUM_PRODUCERS);
    printf("  Consumers: %d\n", NUM_CONSUMERS);
    printf("  Items per thread: %d\n", ITEMS_PER_THREAD);
    printf("  Total expected items: %d\n\n",
           NUM_PRODUCERS * ITEMS_PER_THREAD);
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
    while(atomic_load(&activeProducers) > 0 || atomic_load(&activeConsumers) > 0)
    {
        printf("\n[Monitor] Buffer status: %lu/%u (used/total)\n",
               cb_dataSize(&sharedBuffer), BUFFER_CAPACITY);
        printf("[Monitor] Produced: %u, Consumed: %u\n",
               producedCount, consumedCount);
        sleep(1);
    }

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
    printf("  Produced: %u items\n", producedCount);
    printf("  Consumed: %u items\n", consumedCount);
    printf("  Buffer items remaining: %lu\n", cb_dataSize(&sharedBuffer));

    if(producedCount == consumedCount &&
            consumedCount == (uint32_t)(NUM_PRODUCERS * ITEMS_PER_THREAD))
    {
        printf("\nSUCCESS: All items processed correctly!\n");
    }
    else
    {
        printf("\nFAILURE: Item count mismatch detected!\n");
    }

    return 0;
}
