/*
    @file    demo_mutex.c
    @brief   Mutex-protected stress test demo with atomic counters and yield for the cb (circular buffer) library.
    @details Multi-threaded producer/consumer demo to validate lock-free behavior
            and correctness under high load.
            Compile: gcc -o demo_mutex demo_mutex.c -pthread

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
#include "cb.h"

#define BUFFER_CAPACITY 256U
#define NUM_PRODUCERS   8
#define NUM_CONSUMERS   8
#define ITEMS_PER_THREAD 1000000

static cb sharedBuffer;
static CbItem bufferStorage[BUFFER_CAPACITY];
pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;

atomic_uint producedCount = 0;
atomic_uint consumedCount = 0;
atomic_int activeProducers = 0;

void *producerThread(void *arg)
{
    const int thread_id = *(int *)arg;
    printf("Producer %d started\n", thread_id);
    atomic_fetch_add(&activeProducers, 1);

    for(uint32_t i = 0; i < ITEMS_PER_THREAD; ++i)
    {
        CbItem item = (CbItem)((i + thread_id * ITEMS_PER_THREAD) & 0xFFU);

        while(1)
        {
            pthread_mutex_lock(&bufferMutex);
            const bool success = cb_insert(&sharedBuffer, item);
            pthread_mutex_unlock(&bufferMutex);

            if(success)
            {
                atomic_fetch_add(&producedCount, 1U);
                break;
            }

            sched_yield();  // Yield CPU when buffer is full
        }
    }

    printf("Producer %d finished\n", thread_id);
    atomic_fetch_sub(&activeProducers, 1);
    return NULL;
}

void *consumerThread(void *arg)
{
    const int thread_id = *(int *)arg;
    printf("Consumer %d started\n", thread_id);
    CbItem item;

    while(1)
    {
        pthread_mutex_lock(&bufferMutex);
        const bool success = cb_remove(&sharedBuffer, &item);
        pthread_mutex_unlock(&bufferMutex);

        if(success)
        {
            atomic_fetch_add(&consumedCount, 1U);
            continue;
        }

        // Check if we should exit (no active producers + buffer empty)
        if(atomic_load(&activeProducers) == 0)
        {
            break;
        }

        sched_yield();  // Yield CPU when buffer empty but producers active
    }

    printf("Consumer %d finished\n", thread_id);
    return NULL;
}

int main(void)
{
    cb_init(&sharedBuffer, bufferStorage, BUFFER_CAPACITY);
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

    // Wait for producers
    for(uint32_t i = 0; i < NUM_PRODUCERS; ++i)
    {
        pthread_join(producers[i], NULL);
    }

    // Wait for consumers
    for(uint32_t i = 0; i < NUM_CONSUMERS; ++i)
    {
        pthread_join(consumers[i], NULL);
    }

    printf("\nResults:\n");
    printf("  Expected items: %u\n", NUM_PRODUCERS * ITEMS_PER_THREAD);
    printf("  Produced: %u items\n", producedCount);
    printf("  Consumed: %u items\n", consumedCount);
    printf("  Buffer items remaining: %llu\n", cb_dataSize(&sharedBuffer));

    int ret_val = 0;
    if(producedCount == consumedCount &&
            consumedCount == NUM_PRODUCERS * ITEMS_PER_THREAD)
    {
        printf("\nStress test PASSED.\n");
        ret_val = 0;
    }
    else
    {
        printf("\nStress test FAILED.\n");
        ret_val = 1;
    }

    pthread_mutex_destroy(&bufferMutex);
    return ret_val;
}
