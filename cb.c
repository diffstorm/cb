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
       - `cb_sanity_check()`: Validate buffer integrity
       - `cb_peek()`       : Read item without removal
       - `cb_insert_bulk()`: Insert multiple items
       - `cb_remove_bulk()`: Remove multiple items
       - `cb_set_overwrite()`: Enable/disable overwrite mode

    @note This implementation is suitable for 1-producer, 1-consumer scenarios.
         Index types and memory fencing are adapted per platform for correctness.

    @note Requires compiler barriers for correct memory ordering in
         multi-threaded environments. Current implementation includes
         barriers for GCC-compatible compilers. For other compilers,
         use appropriate memory ordering intrinsics _ReadWriteBarrier(), __memory_changed() etc.

    @date 2023-04-01
    @version 1.1
    @author Eray Ozturk | erayozturk1@gmail.com
    @url github.com/diffstorm
    @license MIT License
*/

#include "cb.h"

// Initialize circular buffer
void cb_init(cb *const cb_ptr, CbItem bufferStorage[], CbIndex bufferLength) {
    if (cb_ptr && bufferStorage && bufferLength > 0) {
        cb_ptr->buf = bufferStorage;
        cb_ptr->size = bufferLength;
        CB_ATOMIC_STORE(&cb_ptr->in, 0);
        CB_ATOMIC_STORE(&cb_ptr->out, 0);
        CB_ATOMIC_STORE(&cb_ptr->overwrite, 0);  // Default: overwrite disabled
    }
}

// Get free space in buffer
CbIndex cb_freeSpace(cb *const cb_ptr) {
    if (!cb_ptr) return 0;
    
    CbIndex in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex out = CB_ATOMIC_LOAD(&cb_ptr->out);
    
    if (in >= out) {
        return (cb_ptr->size - 1) - (in - out);
    }
    return out - in - 1;
}

// Get occupied space in buffer
CbIndex cb_dataSize(cb *const cb_ptr) {
    if (!cb_ptr) return 0;
    
    CbIndex in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex out = CB_ATOMIC_LOAD(&cb_ptr->out);
    
    if (in >= out) {
        return in - out;
    }
    return cb_ptr->size - out + in;
}

// Validate buffer integrity
bool cb_sanity_check(const cb *cb_ptr) {
    if (!cb_ptr) return false;
    if (!cb_ptr->buf) return false;
    if (cb_ptr->size == 0) return false;
    
    CbIndex in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex out = CB_ATOMIC_LOAD(&cb_ptr->out);
    
    // Check index bounds
    if (in >= cb_ptr->size) return false;
    if (out >= cb_ptr->size) return false;
    
    // Check data size consistency
    CbIndex data_size = (in >= out) ? (in - out) : (cb_ptr->size - out + in);
    if (data_size >= cb_ptr->size) return false;
    
    return true;
}

// Insert single item
bool cb_insert(cb *const cb_ptr, CbItem const item) {
    if (!cb_ptr) return false;
    
    CbIndex current_in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex next_in = (current_in + 1) % cb_ptr->size;
    CbIndex current_out = CB_ATOMIC_LOAD(&cb_ptr->out);
    
    // Handle full buffer condition
    if (next_in == current_out) {
        if (CB_ATOMIC_LOAD(&cb_ptr->overwrite)) {
            // Advance out pointer (overwrite oldest item)
            CB_ATOMIC_STORE(&cb_ptr->out, (current_out + 1) % cb_ptr->size);
            CB_MEMORY_BARRIER();
        } else {
            return false;  // Buffer full and overwrite disabled
        }
    }
    
    // Insert new item
    cb_ptr->buf[current_in] = item;
    CB_MEMORY_BARRIER();
    CB_ATOMIC_STORE(&cb_ptr->in, next_in);
    return true;
}

// Remove single item
bool cb_remove(cb *const cb_ptr, CbItem *itemOut) {
    if (!cb_ptr || !itemOut) return false;
    
    CbIndex current_out = CB_ATOMIC_LOAD(&cb_ptr->out);
    CbIndex current_in = CB_ATOMIC_LOAD(&cb_ptr->in);
    
    if (current_out == current_in) {
        return false;  // Buffer empty
    }
    
    *itemOut = cb_ptr->buf[current_out];
    CB_MEMORY_BARRIER();
    CB_ATOMIC_STORE(&cb_ptr->out, (current_out + 1) % cb_ptr->size);
    return true;
}

// Peek at item without removal
bool cb_peek(const cb *cb_ptr, size_t offset, CbItem *itemOut) {
    if (!cb_ptr || !itemOut) return false;
    
    CbIndex current_out = CB_ATOMIC_LOAD(&cb_ptr->out);
    CbIndex current_in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex available = (current_in >= current_out) ? 
        (current_in - current_out) : 
        (cb_ptr->size - current_out + current_in);
    
    if (offset >= available) {
        return false;  // Offset beyond available data
    }
    
    CbIndex pos = (current_out + offset) % cb_ptr->size;
    *itemOut = cb_ptr->buf[pos];
    return true;
}

// Insert multiple items
size_t cb_insert_bulk(cb *cb_ptr, const CbItem *items, size_t count) {
    if (!cb_ptr || !items || count == 0) return 0;
    
    size_t inserted = 0;
    bool overwrite_enabled = CB_ATOMIC_LOAD(&cb_ptr->overwrite);
    
    while (inserted < count) {
        CbIndex current_in = CB_ATOMIC_LOAD(&cb_ptr->in);
        CbIndex next_in = (current_in + 1) % cb_ptr->size;
        CbIndex current_out = CB_ATOMIC_LOAD(&cb_ptr->out);
        
        // Handle buffer full condition
        if (next_in == current_out) {
            if (overwrite_enabled) {
                // Advance out pointer (overwrite oldest item)
				CB_ATOMIC_STORE(&cb_ptr->out, (current_out + 1) % cb_ptr->size);
                CB_MEMORY_BARRIER();
            } else {
                break;  // Stop if overwrite disabled
            }
        }
        
        // Insert item
        cb_ptr->buf[current_in] = items[inserted];
        inserted++;
        
        // Update in pointer
        CB_MEMORY_BARRIER();
        cb_ptr->in = next_in;
    }
    
    return inserted;
}

// Remove multiple items
size_t cb_remove_bulk(cb *cb_ptr, CbItem *items, size_t count) {
    if (!cb_ptr || !items || count == 0) return 0;
    
    size_t removed = 0;
    
    while (removed < count) {
        CbIndex current_out = CB_ATOMIC_LOAD(&cb_ptr->out);
        CbIndex current_in = CB_ATOMIC_LOAD(&cb_ptr->in);
        
        if (current_out == current_in) {
            break;  // Buffer empty
        }
        
        items[removed] = cb_ptr->buf[current_out];
        removed++;
        
        // Update out pointer
        CB_MEMORY_BARRIER();
		CB_ATOMIC_STORE(&cb_ptr->out, (current_out + 1) % cb_ptr->size);
    }
    
    return removed;
}

// Set overwrite mode
void cb_set_overwrite(cb *cb_ptr, bool enable) {
    if (cb_ptr) {
        CB_ATOMIC_STORE(&cb_ptr->overwrite, enable ? 1 : 0);
        CB_MEMORY_BARRIER();
    }
}

// Get overwrite mode
bool cb_get_overwrite(const cb *cb_ptr) {
    return cb_ptr ? (CB_ATOMIC_LOAD(&cb_ptr->overwrite) != 0) : false;
}

