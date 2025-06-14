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

#include "cb.h"

void cb_init(cb *const cb_ptr, CbItem bufferStorage[], CbIndex bufferLength)
{
    if(NULL != cb_ptr)
    {
        cb_ptr->buf = bufferStorage;
        cb_ptr->size = bufferLength;
        cb_ptr->in = 0U;
        cb_ptr->out = 0U;
    }
}

bool cb_insert(cb *const cb_ptr, CbItem const item)
{
    if(NULL == cb_ptr)
    {
        return false;
    }

    CbIndex current_in = cb_ptr->in;
    CbIndex next_in = (current_in + 1) % cb_ptr->size;
    CbIndex current_out = CB_ATOMIC_LOAD(&cb_ptr->out);

    if(next_in != current_out)
    {
        cb_ptr->buf[current_in] = item;
        CB_MEMORY_BARRIER();
        CB_ATOMIC_STORE(&cb_ptr->in, next_in);
        return true;
    }

    return false;  // Buffer full
}

bool cb_remove(cb *const cb_ptr, CbItem *itemOut)
{
    if((NULL == cb_ptr) || (NULL == itemOut))
    {
        return false;
    }

    CbIndex current_out = cb_ptr->out;
    CbIndex current_in = CB_ATOMIC_LOAD(&cb_ptr->in);

    if(current_in != current_out)
    {
        *itemOut = cb_ptr->buf[current_out];
        CbIndex next_out = (current_out + 1) % cb_ptr->size;
        CB_MEMORY_BARRIER();
        CB_ATOMIC_STORE(&cb_ptr->out, next_out);
        return true;
    }

    return false;  // Buffer empty
}

CbIndex cb_freeSpace(cb *const cb_ptr)
{
    if(NULL == cb_ptr)
    {
        return 0U;
    }

    if(cb_ptr->in >= cb_ptr->out)
    {
        return (cb_ptr->size - 1) - (cb_ptr->in - cb_ptr->out);
    }

    return (cb_ptr->out - cb_ptr->in - 1);
}

CbIndex cb_dataSize(cb *const cb_ptr)
{
    if(NULL == cb_ptr)
    {
        return 0U;
    }

    if(cb_ptr->in >= cb_ptr->out)
    {
        return cb_ptr->in - cb_ptr->out;
    }

    return cb_ptr->size - cb_ptr->out + cb_ptr->in;
}

