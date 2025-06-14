/*
    @file    test.cpp
    @brief   GoogleTest unit tests for the cb circular buffer library.
    @details Contains functional tests, parameterized size tests, multi-threaded
            stress tests, and ISR-style atomic visibility validation using fences.

    @date 2023-04-01
    @version 1.0
    @author Eray Ozturk | erayozturk1@gmail.com
    @url github.com/diffstorm
    @license MIT License
*/

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
extern "C" {
#include "cb.h"
}

using namespace std::chrono_literals;

// COMMONS

constexpr CbIndex DEFAULT_CAPACITY = 8;

class CircularBufferTest : public ::testing::Test
{
protected:
    cb buffer;
    CbItem storage[DEFAULT_CAPACITY];

    void SetUp() override
    {
        cb_init(&buffer, storage, DEFAULT_CAPACITY);
    }
};

// BASIC FUNCTIONAL TESTS

TEST_F(CircularBufferTest, InitIsEmpty)
{
    EXPECT_EQ(cb_dataSize(&buffer), 0U);
    EXPECT_EQ(cb_freeSpace(&buffer), DEFAULT_CAPACITY - 1U);
}

TEST_F(CircularBufferTest, InsertThenRemove)
{
    EXPECT_TRUE(cb_insert(&buffer, 42U));
    CbItem out = 0;
    EXPECT_TRUE(cb_remove(&buffer, &out));
    EXPECT_EQ(out, 42U);
}

TEST_F(CircularBufferTest, FillToCapacityFailsAfterwards)
{
    for(CbIndex i = 0; i < DEFAULT_CAPACITY - 1U; ++i)
    {
        EXPECT_TRUE(cb_insert(&buffer, static_cast<CbItem>(i)));
    }

    EXPECT_FALSE(cb_insert(&buffer, 0xFF));
}

TEST_F(CircularBufferTest, RemoveFromEmptyFails)
{
    CbItem out = 0xA5;
    EXPECT_FALSE(cb_remove(&buffer, &out));
    EXPECT_EQ(out, 0xA5);
}

TEST_F(CircularBufferTest, WrapAroundCorrectness)
{
    for(CbIndex i = 0; i < DEFAULT_CAPACITY - 1U; ++i)
    {
        EXPECT_TRUE(cb_insert(&buffer, static_cast<CbItem>(i)));
    }

    for(CbIndex i = 0; i < 4U; ++i)
    {
        CbItem out;
        EXPECT_TRUE(cb_remove(&buffer, &out));
        EXPECT_EQ(out, i);
    }

    for(CbIndex i = 100U; i < 104U; ++i)
    {
        EXPECT_TRUE(cb_insert(&buffer, static_cast<CbItem>(i)));
    }

    for(CbIndex i = 4U; i < DEFAULT_CAPACITY - 1U; ++i)
    {
        CbItem out;
        EXPECT_TRUE(cb_remove(&buffer, &out));
        EXPECT_EQ(out, i);
    }

    for(CbIndex i = 100U; i < 104U; ++i)
    {
        CbItem out;
        EXPECT_TRUE(cb_remove(&buffer, &out));
        EXPECT_EQ(out, i);
    }
}

TEST_F(CircularBufferTest, NullSafety)
{
    EXPECT_EQ(cb_dataSize(nullptr), 0U);
    EXPECT_EQ(cb_freeSpace(nullptr), 0U);
    EXPECT_FALSE(cb_insert(nullptr, 123U));
    EXPECT_FALSE(cb_remove(nullptr, nullptr));
    CbItem out = 0xBB;
    EXPECT_FALSE(cb_remove(&buffer, nullptr));
    EXPECT_FALSE(cb_remove(nullptr, &out));
}

// PARAMETERIZED TESTS
class CircularBufferParamTest : public ::testing::TestWithParam<CbIndex>
{
protected:
    cb buffer;
    std::vector<CbItem> storage;

    void SetUp() override
    {
        storage.resize(GetParam());
        cb_init(&buffer, storage.data(), GetParam());
    }
};

TEST_P(CircularBufferParamTest, FillDrainMatches)
{
    const CbIndex cap = GetParam();
    const CbIndex usable = cap - 1U;

    for(CbIndex i = 0; i < usable; ++i)
    {
        EXPECT_TRUE(cb_insert(&buffer, static_cast<CbItem>(i)));
    }

    for(CbIndex i = 0; i < usable; ++i)
    {
        CbItem out = 0;
        EXPECT_TRUE(cb_remove(&buffer, &out));
        EXPECT_EQ(out, static_cast<CbItem>(i));
    }
}

INSTANTIATE_TEST_SUITE_P(
    Sizes,
    CircularBufferParamTest,
    ::testing::Values(4, 8, 16, 32, 64)
);

// MULTI-THREADED TEST
TEST(CircularBufferThreadTest, StressTestNoLocks)
{
    constexpr CbIndex CAP = 64;
    constexpr int THREADS = 4;
    constexpr int PER_THREAD = 10000;
    const int TOTAL = THREADS * PER_THREAD;
    cb buffer;
    CbItem storage[CAP];
    cb_init(&buffer, storage, CAP);
    std::atomic_uint produced{0};
    std::atomic_uint consumed{0};
    std::atomic_bool producers_done{false};
    std::atomic_bool consumers_done{false};
    auto producer = [&]()
    {
        for(int i = 0; i < PER_THREAD; ++i)
        {
            while(!cb_insert(&buffer, static_cast<CbItem>(i & 0xFF)))
            {
                std::this_thread::yield();
            }

            produced.fetch_add(1, std::memory_order_relaxed);
        }
    };
    auto consumer = [&]()
    {
        CbItem item;

        while(!consumers_done.load())
        {
            if(cb_remove(&buffer, &item))
            {
                consumed.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                // Only yield if producers are still active
                if(!producers_done.load())
                {
                    std::this_thread::yield();
                }
            }
        }
    };
    std::vector<std::thread> producers;

    for(int i = 0; i < THREADS; ++i)
    {
        producers.emplace_back(producer);
    }

    std::vector<std::thread> consumers;

    for(int i = 0; i < THREADS; ++i)
    {
        consumers.emplace_back(consumer);
    }

    // Wait for producers
    for(auto &t : producers)
    {
        t.join();
    }

    producers_done = true;
    // Let consumers finish remaining items
    std::this_thread::sleep_for(50ms);
    // Signal consumers to exit
    consumers_done = true;

    // Wait for consumers
    for(auto &t : consumers)
    {
        t.join();
    }

    // Drain any remaining items
    CbItem item;
    unsigned drained = 0;

    while(cb_remove(&buffer, &item))
    {
        drained++;
    }

    // Calculate final consumed count
    const unsigned final_consumed = consumed.load() + drained;
    EXPECT_EQ(produced.load(), TOTAL);
    EXPECT_EQ(final_consumed, TOTAL);
    EXPECT_EQ(cb_dataSize(&buffer), 0U);
}

// ISR-SAFE SIMULATION TEST
TEST(CircularBufferISRTest, SimulateISRReader)
{
    constexpr CbIndex CAP = 32;
    constexpr int ITEMS = 50000;
    cb buffer;
    CbItem storage[CAP];
    cb_init(&buffer, storage, CAP);
    std::atomic_uint inserted{0};
    std::atomic_uint removed{0};
    std::atomic_bool done{false};
    std::thread isr_reader([&]()
    {
        CbItem out;

        while(!done.load())
        {
            while(cb_remove(&buffer, &out))
            {
                removed.fetch_add(1, std::memory_order_relaxed);
            }

            std::this_thread::sleep_for(1us);
        }

        while(cb_remove(&buffer, &out))
        {
            removed.fetch_add(1, std::memory_order_relaxed);
        }
    });

    for(int i = 0; i < ITEMS; ++i)
    {
        CbItem val = static_cast<CbItem>(i & 0xFF);

        while(!cb_insert(&buffer, val))
        {
            std::this_thread::yield();
        }

        inserted.fetch_add(1, std::memory_order_relaxed);
    }

    done.store(true);
    isr_reader.join();
    EXPECT_EQ(inserted.load(), ITEMS);
    EXPECT_EQ(removed.load(), ITEMS);
    EXPECT_EQ(cb_dataSize(&buffer), 0U);
}

// Memory fencing and volatile order validation
TEST(CircularBufferFenceTest, MemoryVisibilityWithRelaxedOrdering)
{
    constexpr CbIndex CAP = 16;
    cb buffer;
    CbItem storage[CAP];
    cb_init(&buffer, storage, CAP);
    std::atomic_bool ready{false};
    std::atomic_bool done{false};
    std::atomic_int errorCount{0};
    std::thread writer([&]()
    {
        while(!ready.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        for(int i = 0; i < 10000; ++i)
        {
            while(!cb_insert(&buffer, static_cast<CbItem>(i & 0xFF)))
            {
                std::this_thread::yield();
            }

            std::atomic_thread_fence(std::memory_order_release);
        }

        done.store(true, std::memory_order_release);
    });
    std::thread reader([&]()
    {
        ready.store(true, std::memory_order_release);
        int success = 0;
        CbItem out;

        while(!done.load(std::memory_order_acquire) || cb_dataSize(&buffer) > 0U)
        {
            std::atomic_thread_fence(std::memory_order_acquire);

            while(cb_remove(&buffer, &out))
            {
                ++success;
            }

            std::this_thread::yield();
        }

        if(success != 10000)
        {
            errorCount.store(10000 - success, std::memory_order_relaxed);
        }
    });
    writer.join();
    reader.join();
    EXPECT_EQ(errorCount.load(), 0);
}

// EDGE CASE: BUFFER SIZE 1 (SHOULD BE UNUSABLE)
TEST(CircularBufferEdgeTest, Size1BufferUnusable)
{
    cb buffer;
    CbItem storage[1];
    cb_init(&buffer, storage, 1);  // Size 1 buffer
    // Should be considered full immediately (size N-1 usable)
    EXPECT_EQ(cb_freeSpace(&buffer), 0U);
    EXPECT_FALSE(cb_insert(&buffer, 0xAA));
    CbItem out;
    EXPECT_FALSE(cb_remove(&buffer, &out));
}

// EDGE CASE: BUFFER SIZE 2 (MINIMAL WORKING BUFFER)
TEST(CircularBufferEdgeTest, Size2BufferOperations)
{
    cb buffer;
    CbItem storage[2];
    cb_init(&buffer, storage, 2);
    // Should have 1 usable slot
    EXPECT_EQ(cb_freeSpace(&buffer), 1U);
    // Fill the single slot
    EXPECT_TRUE(cb_insert(&buffer, 0xAA));
    EXPECT_FALSE(cb_insert(&buffer, 0xBB)); // Should fail
    // Retrieve item
    CbItem out;
    EXPECT_TRUE(cb_remove(&buffer, &out));
    EXPECT_EQ(out, 0xAA);
    EXPECT_FALSE(cb_remove(&buffer, &out)); // Should fail
    // Test wrap-around
    EXPECT_TRUE(cb_insert(&buffer, 0xCC));
    EXPECT_TRUE(cb_remove(&buffer, &out));
    EXPECT_EQ(out, 0xCC);
}

// EDGE CASE: VERY LARGE BUFFER
TEST(CircularBufferEdgeTest, VeryLargeBuffer)
{
    constexpr CbIndex HUGE_SIZE = 65536;
    std::vector<CbItem> storage(HUGE_SIZE);
    cb buffer;
    cb_init(&buffer, storage.data(), HUGE_SIZE);

    // Fill to capacity (HUGE_SIZE - 1)
    for(CbIndex i = 0; i < HUGE_SIZE - 1; ++i)
    {
        EXPECT_TRUE(cb_insert(&buffer, static_cast<CbItem>(i & 0xFF)));
    }

    EXPECT_FALSE(cb_insert(&buffer, 0xFF)); // Should fail

    // Verify all items
    for(CbIndex i = 0; i < HUGE_SIZE - 1; ++i)
    {
        CbItem out;
        EXPECT_TRUE(cb_remove(&buffer, &out));
        EXPECT_EQ(out, static_cast<CbItem>(i & 0xFF));
    }

    EXPECT_EQ(cb_dataSize(&buffer), 0U);
}

// EDGE CASE: RAPID CONSUMER-PRODUCER CONTENTION
TEST(CircularBufferEdgeTest, RapidContention)
{
    constexpr CbIndex CAP = 4;
    constexpr int ITERATIONS = 1000;
    cb buffer;
    CbItem storage[CAP];
    cb_init(&buffer, storage, CAP);
    std::atomic_int counter{0};
    std::atomic_bool done{false};
    auto producer = [&]()
    {
        for(int i = 0; i < ITERATIONS; ++i)
        {
            while(!cb_insert(&buffer, static_cast<CbItem>(i & 0xFF)))
            {
                // Tight loop on purpose
            }

            counter.fetch_add(1, std::memory_order_relaxed);
        }
    };
    auto consumer = [&]()
    {
        CbItem item;

        for(int i = 0; i < ITERATIONS; ++i)
        {
            while(!cb_remove(&buffer, &item))
            {
                // Tight loop on purpose
            }

            counter.fetch_sub(1, std::memory_order_relaxed);
        }

        done = true;
    };
    std::thread prod_thread(producer);
    std::thread cons_thread(consumer);
    // Monitor buffer fill level during operation
    auto monitor = std::thread([&]()
    {
        while(!done)
        {
            CbIndex size = cb_dataSize(&buffer);
            EXPECT_LE(size, CAP - 1);  // Should never exceed capacity
            std::this_thread::sleep_for(1ms);
        }
    });
    prod_thread.join();
    cons_thread.join();
    done = true;
    monitor.join();
    EXPECT_EQ(counter.load(), 0);
    EXPECT_EQ(cb_dataSize(&buffer), 0U);
}

// EDGE CASE: CROSS-THREAD INDEX CORRUPTION
TEST(CircularBufferEdgeTest, CrossThreadIndexCorruption)
{
    constexpr CbIndex CAP = 8;
    constexpr int THREADS = 8;
    constexpr int OPERATIONS = 10000;
    cb buffer;
    CbItem storage[CAP];
    cb_init(&buffer, storage, CAP);
    std::vector<std::thread> threads;
    std::atomic_uint operations{0};
    std::atomic_bool running{true};

    // Create mixed producer/consumer threads
    for(int i = 0; i < THREADS; ++i)
    {
        threads.emplace_back([ &, i]()
        {
            CbItem item;

            while(running)
            {
                if(i % 2 == 0)     // Producers
                {
                    if(cb_insert(&buffer, static_cast<CbItem>(i)))
                    {
                        operations.fetch_add(1, std::memory_order_relaxed);
                    }
                }
                else      // Consumers
                {
                    if(cb_remove(&buffer, &item))
                    {
                        operations.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
        });
    }

    // Run for a fixed duration
    std::this_thread::sleep_for(500ms);
    running = false;

    for(auto &t : threads)
    {
        t.join();
    }

    // Verify buffer integrity after stress
    CbIndex size = cb_dataSize(&buffer);
    CbIndex free = cb_freeSpace(&buffer);
    EXPECT_EQ(size + free, CAP - 1);
    std::cout << "Performed " << operations.load() << " operations without corruption\n";
}

// EDGE CASE: SINGLE THREAD WRAP-AROUND
TEST(CircularBufferEdgeTest, SingleThreadWrapAround)
{
    constexpr CbIndex CAP = 4;
    cb buffer;
    CbItem storage[CAP];
    cb_init(&buffer, storage, CAP);

    // Fill buffer (3 items: 0,1,2)
    for(CbIndex i = 0; i < CAP - 1; ++i)
    {
        EXPECT_TRUE(cb_insert(&buffer, static_cast<CbItem>(i)));
    }

    // Remove 2 items
    CbItem out;
    EXPECT_TRUE(cb_remove(&buffer, &out));
    EXPECT_EQ(out, 0);
    EXPECT_TRUE(cb_remove(&buffer, &out));
    EXPECT_EQ(out, 1);
    // Add 2 new items
    EXPECT_TRUE(cb_insert(&buffer, 10));
    EXPECT_TRUE(cb_insert(&buffer, 11));
    // Verify contents - CORRECTED EXPECTATIONS
    EXPECT_TRUE(cb_remove(&buffer, &out));
    EXPECT_EQ(out, 2);
    EXPECT_TRUE(cb_remove(&buffer, &out));
    EXPECT_EQ(out, 10);
    EXPECT_TRUE(cb_remove(&buffer, &out));
    EXPECT_EQ(out, 11);
    EXPECT_FALSE(cb_remove(&buffer, &out));
}

// EDGE CASE: BUFFER FULL/EMPTY STATE TRANSITIONS
TEST(CircularBufferEdgeTest, FullEmptyTransitions)
{
    constexpr CbIndex CAP = 3;
    cb buffer;
    CbItem storage[CAP];
    cb_init(&buffer, storage, CAP);
    // Should be empty
    EXPECT_EQ(cb_dataSize(&buffer), 0U);
    EXPECT_EQ(cb_freeSpace(&buffer), CAP - 1);  // 2

    // Fill to capacity (2 items)
    for(CbIndex i = 0; i < CAP - 1; ++i)
    {
        EXPECT_TRUE(cb_insert(&buffer, static_cast<CbItem>(i)));
    }

    EXPECT_EQ(cb_dataSize(&buffer), CAP - 1);  // 2
    EXPECT_EQ(cb_freeSpace(&buffer), 0U);      // 0
    // Remove one item
    CbItem out;
    EXPECT_TRUE(cb_remove(&buffer, &out));
    EXPECT_EQ(out, 0);
    EXPECT_EQ(cb_dataSize(&buffer), 1U);
    EXPECT_EQ(cb_freeSpace(&buffer), 1U);
    // Add one item
    EXPECT_TRUE(cb_insert(&buffer, 10));
    EXPECT_EQ(cb_dataSize(&buffer), 2U);
    EXPECT_EQ(cb_freeSpace(&buffer), 0U);

    // Remove all items
    for(CbIndex i = 0; i < CAP - 1; ++i)
    {
        EXPECT_TRUE(cb_remove(&buffer, &out));
    }

    EXPECT_EQ(cb_dataSize(&buffer), 0U);
    EXPECT_EQ(cb_freeSpace(&buffer), CAP - 1);  // 2
}

TEST(CircularBufferEdgeTest, BufferSize1)
{
    cb buffer;
    CbItem storage[1];
    cb_init(&buffer, storage, 1);
    EXPECT_EQ(cb_dataSize(&buffer), 0U);
    EXPECT_EQ(cb_freeSpace(&buffer), 0U);
    EXPECT_FALSE(cb_insert(&buffer, 1));
    CbItem out;
    EXPECT_FALSE(cb_remove(&buffer, &out));
}

TEST(CircularBufferEdgeTest, BufferSize2)
{
    cb buffer;
    CbItem storage[2];
    cb_init(&buffer, storage, 2);
    EXPECT_TRUE(cb_insert(&buffer, 1));
    EXPECT_FALSE(cb_insert(&buffer, 2));  // Should fail
    CbItem out;
    EXPECT_TRUE(cb_remove(&buffer, &out));
    EXPECT_EQ(out, 1);
    EXPECT_FALSE(cb_remove(&buffer, &out));
}

// EDGE CASE: High Contention Stress Test
TEST(CircularBufferEdgeTest, HighContentionStressTest)
{
    constexpr CbIndex CAP = 8;
    constexpr int THREADS = 4;
    constexpr int ITERATIONS = 100000;
    const int TOTAL = THREADS * ITERATIONS;
    cb buffer;
    CbItem storage[CAP];
    cb_init(&buffer, storage, CAP);
    std::atomic<int> total_produced{0};
    std::atomic<int> total_consumed{0};
    std::atomic<bool> producers_done{false};
    std::atomic<bool> consumers_done{false};
    // Create producers
    std::vector<std::thread> producers;

    for(int i = 0; i < THREADS; i++)
    {
        producers.emplace_back([&]()
        {
            for(int j = 0; j < ITERATIONS; j++)
            {
                while(!cb_insert(&buffer, static_cast<CbItem>(j & 0xFF)))
                {
                    std::this_thread::yield();
                }

                total_produced.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Create consumers
    std::vector<std::thread> consumers;

    for(int i = 0; i < THREADS; i++)
    {
        consumers.emplace_back([&]()
        {
            CbItem item;

            while(!consumers_done.load())
            {
                if(cb_remove(&buffer, &item))
                {
                    total_consumed.fetch_add(1, std::memory_order_relaxed);
                }
                else
                {
                    // Only yield if producers are still active
                    if(!producers_done.load())
                    {
                        std::this_thread::yield();
                    }
                }
            }
        });
    }

    // Wait for producers
    for(auto &t : producers)
    {
        t.join();
    }

    producers_done = true;
    // Let consumers finish remaining items
    std::this_thread::sleep_for(50ms);
    // Signal consumers to exit
    consumers_done = true;

    // Wait for consumers
    for(auto &t : consumers)
    {
        t.join();
    }

    // Drain any remaining items
    CbItem item;
    int drained_count = 0;

    while(cb_remove(&buffer, &item))
    {
        drained_count++;
    }

    // Calculate final consumed count
    const int final_consumed = total_consumed.load() + drained_count;
    // Verify results
    EXPECT_EQ(total_produced.load(), TOTAL);
    EXPECT_EQ(final_consumed, TOTAL);
    EXPECT_EQ(cb_dataSize(&buffer), 0U);
    std::cout << "Buffer integrity: "
              << total_produced << " produced, "
              << total_consumed << " consumed by threads, "
              << drained_count << " drained, "
              << "total: " << final_consumed << "\n";
}
