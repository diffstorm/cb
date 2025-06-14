/*
    @file        cb.h / cb.c
    @brief       Lock-free circular buffer (ring buffer) library
    @details
     - Portable, MISRA-aligned implementation of a circular (ring) buffer.
     - Supports concurrent usage with ISR-safe and lock-free access patterns.
     - `cb_insert` and `cb_remove` use volatile atomic index updates.
     - No dynamic memory, works in bare-metal, RTOS, or OS environments.
     - Configurable platform-aware atomic index type (`CbAtomicIndex`).
     - Buffer operates with one writer and one reader without locks.
     - Platform-aware atomic index selection.

     Custom types:
       - `CbItem`         : item type (default: uint8_t)
       - `CbIndex`        : platform-sized index type
       - `CbAtomicIndex`  : platform-safe atomic index (sig_atomic_t, LONG, etc.)

     Public API:
       - `cb_init()`       : Initialize buffer with static storage
       - `cb_insert()`     : Add item (non-blocking, returns false if full)
       - `cb_remove()`     : Retrieve item (non-blocking, returns false if empty)
       - `cb_freeSpace()`  : Get number of free slots
       - `cb_dataSize()`   : Get number of occupied slots

    @note This implementation is suitable for 1-producer, 1-consumer scenarios.
         Index types and memory fencing are adapted per platform for correctness.

    @note Requires compiler barriers for correct memory ordering in
         multi-threaded environments. Current implementation includes
         barriers for GCC-compatible compilers. For other compilers,
         use appropriate memory ordering intrinsics _ReadWriteBarrier(), __memory_changed() etc.

    @date 2023-04-01
    @version 1.0
    @author Eray Ozturk | erayozturk1@gmail.com
    @url github.com/diffstorm
    @license MIT License
*/

#ifndef CB_H
#define CB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "cb_wordsize_detect.h"
#include "cb_atomicindex_detect.h"
#include "cb_memorybarrier_detect.h"
#include "cb_atomic_access.h"

typedef CB_WORD_INDEX_TYPE     CbIndex;
typedef CB_ATOMIC_INDEX_TYPE   CbAtomicIndex;
typedef uint8_t                CbItem;

/* Ensure atomic index size does not exceed index size */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    _Static_assert(sizeof(CbAtomicIndex) <= sizeof(CbIndex),
                   "CbAtomicIndex must not be larger than CbIndex");
#elif defined(__cplusplus) && (__cplusplus >= 201103L)
    static_assert(sizeof(CbAtomicIndex) <= sizeof(CbIndex),
                  "CbAtomicIndex must not be larger than CbIndex");
#else
    /* Fallback: will produce unused typedef error if the condition fails */
    typedef char cb_index_size_check[
        (sizeof(CbAtomicIndex) <= sizeof(CbIndex)) ? 1 : -1];
#endif

typedef struct
{
    CbItem *buf;
    CbIndex size;
    volatile CbAtomicIndex in;
    volatile CbAtomicIndex out;
} cb;

// Initialization
void cb_init(cb *const cb_ptr, CbItem bufferStorage[], CbIndex bufferLength);

// State
CbIndex cb_freeSpace(cb *const cb_ptr);
CbIndex cb_dataSize(cb *const cb_ptr);

// Operations
bool cb_insert(cb *const cb_ptr, CbItem const item);
bool cb_remove(cb *const cb_ptr, CbItem *itemOut);

#endif /* CB_H */
