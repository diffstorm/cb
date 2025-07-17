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
     - Configurable item type via CB_ITEM_TYPE define.
     - Optional C11 atomics support when available.

     Custom types:
       - `CbItem`         : item type (default: uint8_t, configurable via CB_ITEM_TYPE)
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
       - `cb_insert_timeout()`: Add item with timeout
       - `cb_remove_timeout()`: Retrieve item with timeout

    @note This implementation is suitable for 1-producer, 1-consumer scenarios.
         Index types and memory fencing are adapted per platform for correctness.

    @note Requires compiler barriers for correct memory ordering in
         multi-threaded environments. Current implementation includes
         barriers for GCC-compatible compilers. For other compilers,
         use appropriate memory ordering intrinsics _ReadWriteBarrier(), __memory_changed() etc.

    @date 2025-07-16
    @version 1.2
    @author Eray Ozturk | erayozturk1@gmail.com
    @url github.com/diffstorm
    @license MIT License
*/

#include "cb.h"
#include <string.h>  // For memset
#include <time.h>    // For timespec_get in timeout functions

/* Internal helper macros for error handling */
#define CB_SET_ERROR(cb_ptr, err_code, err_func, err_param, err_line) \
    do { \
        if (cb_ptr) { \
            (cb_ptr)->last_error.code = (err_code); \
            (cb_ptr)->last_error.function = (err_func); \
            (cb_ptr)->last_error.parameter = (err_param); \
            (cb_ptr)->last_error.line = (err_line); \
        } \
    } while(0)

#define CB_RETURN_ERROR(cb_ptr, err_code, err_param) \
    do { \
        CB_SET_ERROR(cb_ptr, err_code, __func__, err_param, __LINE__); \
        return err_code; \
    } while(0)

/* Atomic operations wrappers for C11 atomics */
#if CB_HAS_C11_ATOMICS
    #define CB_ATOMIC_LOAD_IN(cb_ptr)  atomic_load(&(cb_ptr)->in)
    #define CB_ATOMIC_LOAD_OUT(cb_ptr) atomic_load(&(cb_ptr)->out)
    #define CB_ATOMIC_LOAD_OW(cb_ptr)  atomic_load(&(cb_ptr)->overwrite)
    
    #define CB_ATOMIC_STORE_IN(cb_ptr, val)  atomic_store(&(cb_ptr)->in, (val))
    #define CB_ATOMIC_STORE_OUT(cb_ptr, val) atomic_store(&(cb_ptr)->out, (val))
    #define CB_ATOMIC_STORE_OW(cb_ptr, val)  atomic_store(&(cb_ptr)->overwrite, (val))
#else
    #define CB_ATOMIC_LOAD_IN(cb_ptr)  CB_ATOMIC_LOAD(&(cb_ptr)->in)
    #define CB_ATOMIC_LOAD_OUT(cb_ptr) CB_ATOMIC_LOAD(&(cb_ptr)->out)
    #define CB_ATOMIC_LOAD_OW(cb_ptr)  CB_ATOMIC_LOAD(&(cb_ptr)->overwrite)
    
    #define CB_ATOMIC_STORE_IN(cb_ptr, val)  CB_ATOMIC_STORE(&(cb_ptr)->in, (val))
    #define CB_ATOMIC_STORE_OUT(cb_ptr, val) CB_ATOMIC_STORE(&(cb_ptr)->out, (val))
    #define CB_ATOMIC_STORE_OW(cb_ptr, val)  CB_ATOMIC_STORE(&(cb_ptr)->overwrite, (val))
#endif

/* Statistics storage - one per buffer */
typedef struct {
    CbIndex peak_usage;
    CbIndex total_inserts;
    CbIndex total_removes;
    CbIndex overflow_count;
    CbIndex underflow_count;
} cb_internal_stats_t;

/* Global statistics storage - limited to 8 buffers for simplicity */
#define CB_MAX_BUFFERS 8

#if defined(CB_ENABLE_STATISTICS) && CB_ENABLE_STATISTICS
static cb_internal_stats_t cb_stats[CB_MAX_BUFFERS] = {0};
static cb* cb_instances[CB_MAX_BUFFERS] = {NULL};
static int cb_instance_count = 0;

/* Helper function to find or register a buffer for statistics */
static int cb_find_or_register_buffer(cb *const cb_ptr) {
    int i;
    
    /* First, check if this buffer is already registered */
    for (i = 0; i < cb_instance_count; i++) {
        if (cb_instances[i] == cb_ptr) {
            return i;
        }
    }
    
    /* If not registered and we have space, register it */
    if (cb_instance_count < CB_MAX_BUFFERS) {
        i = cb_instance_count++;
        cb_instances[i] = cb_ptr;
        memset(&cb_stats[i], 0, sizeof(cb_internal_stats_t));
        return i;
    }
    
    /* No space for new buffer, return -1 */
    return -1;
}

/* Helper function to update statistics */
static void cb_update_stats(cb *const cb_ptr, CbIndex data_size, bool is_insert, bool is_success) {
    int idx = cb_find_or_register_buffer(cb_ptr);
    if (idx < 0) return;
    
    if (is_insert) {
        if (is_success) {
            cb_stats[idx].total_inserts++;
            if (data_size > cb_stats[idx].peak_usage) {
                cb_stats[idx].peak_usage = data_size;
            }
        } else {
            cb_stats[idx].overflow_count++;
        }
    } else {
        if (is_success) {
            cb_stats[idx].total_removes++;
        } else {
            cb_stats[idx].underflow_count++;
        }
    }
}
#else
/* Stub functions when statistics are disabled */
static int cb_find_or_register_buffer(cb *const cb_ptr) {
    (void)cb_ptr; /* Unused parameter */
    return -1;
}

static void cb_update_stats(cb *const cb_ptr, CbIndex data_size, bool is_insert, bool is_success) {
    (void)cb_ptr;      /* Unused parameter */
    (void)data_size;   /* Unused parameter */
    (void)is_insert;   /* Unused parameter */
    (void)is_success;  /* Unused parameter */
}
#endif /* CB_ENABLE_STATISTICS */

void cb_init(cb *const cb_ptr, CbItem bufferStorage[], CbIndex bufferLength) {
    cb_init_ex(cb_ptr, bufferStorage, bufferLength);
}

cb_result_t cb_init_ex(cb *const cb_ptr, CbItem bufferStorage[], CbIndex bufferLength) {
    if (!cb_ptr) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!bufferStorage) {
        CB_SET_ERROR(cb_ptr, CB_ERROR_NULL_POINTER, __func__, "bufferStorage", __LINE__);
        return CB_ERROR_NULL_POINTER;
    }
    
    /* Initialize error information */
    cb_ptr->last_error.code = CB_SUCCESS;
    cb_ptr->last_error.function = NULL;
    cb_ptr->last_error.parameter = NULL;
    cb_ptr->last_error.line = 0;
    
    /* Register for statistics */
    cb_find_or_register_buffer(cb_ptr);
    
    if (bufferLength == 0) {
        /* Initialize all fields even for zero-size buffer */
        cb_ptr->buf = bufferStorage;
        cb_ptr->size = 0;
        
        #if CB_HAS_C11_ATOMICS
            atomic_store(&cb_ptr->in, 0);
            atomic_store(&cb_ptr->out, 0);
            atomic_store(&cb_ptr->overwrite, false);
        #else
            CB_ATOMIC_STORE(&cb_ptr->in, 0);
            CB_ATOMIC_STORE(&cb_ptr->out, 0);
            CB_ATOMIC_STORE(&cb_ptr->overwrite, 0);
        #endif
        
        CB_SET_ERROR(cb_ptr, CB_ERROR_INVALID_SIZE, __func__, "bufferLength", __LINE__);
        return CB_ERROR_INVALID_SIZE;
    }
    
    cb_ptr->buf = bufferStorage;
    cb_ptr->size = bufferLength;
    
    #if CB_HAS_C11_ATOMICS
        atomic_store(&cb_ptr->in, 0);
        atomic_store(&cb_ptr->out, 0);
        atomic_store(&cb_ptr->overwrite, false);
    #else
        CB_ATOMIC_STORE(&cb_ptr->in, 0);
        CB_ATOMIC_STORE(&cb_ptr->out, 0);
        CB_ATOMIC_STORE(&cb_ptr->overwrite, 0);
    #endif
    
    return CB_SUCCESS;
}

CbIndex cb_freeSpace(cb *const cb_ptr) {
    if (!cb_ptr || cb_ptr->size == 0) return 0;
    
    CbIndex in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex out = CB_ATOMIC_LOAD(&cb_ptr->out);
    
    if (in >= out) {
        return (cb_ptr->size - 1) - (in - out);
    }
    return out - in - 1;
}

cb_result_t cb_freeSpace_ex(cb *const cb_ptr, CbIndex *freeSpace) {
    if (!cb_ptr) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!freeSpace) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (cb_ptr->size == 0) {
        *freeSpace = 0;
        return CB_ERROR_INVALID_SIZE;
    }
    
    CbIndex in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex out = CB_ATOMIC_LOAD(&cb_ptr->out);
    
    if (in >= out) {
        *freeSpace = (cb_ptr->size - 1) - (in - out);
    } else {
        *freeSpace = out - in - 1;
    }
    
    return CB_SUCCESS;
}

CbIndex cb_dataSize(cb *const cb_ptr) {
    if (!cb_ptr || cb_ptr->size == 0) return 0;
    
    CbIndex in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex out = CB_ATOMIC_LOAD(&cb_ptr->out);
    
    if (in >= out) {
        return in - out;
    }
    return cb_ptr->size - out + in;
}

cb_result_t cb_dataSize_ex(cb *const cb_ptr, CbIndex *dataSize) {
    if (!cb_ptr) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!dataSize) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (cb_ptr->size == 0) {
        *dataSize = 0;
        return CB_ERROR_INVALID_SIZE;
    }
    
    CbIndex in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex out = CB_ATOMIC_LOAD(&cb_ptr->out);
    
    if (in >= out) {
        *dataSize = in - out;
    } else {
        *dataSize = cb_ptr->size - out + in;
    }
    
    return CB_SUCCESS;
}

bool cb_sanity_check(const cb *cb_ptr) {
    if (!cb_ptr) return false;
    if (!cb_ptr->buf) return false;
    if (cb_ptr->size == 0) return false;
    
    CbIndex in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex out = CB_ATOMIC_LOAD(&cb_ptr->out);
    
    if (in >= cb_ptr->size) return false;
    if (out >= cb_ptr->size) return false;
    
    CbIndex data_size = (in >= out) ? (in - out) : (cb_ptr->size - out + in);
    if (data_size >= cb_ptr->size) return false;
    
    return true;
}

cb_result_t cb_sanity_check_ex(const cb *cb_ptr, bool *isValid) {
    if (!isValid) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!cb_ptr) {
        *isValid = false;
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!cb_ptr->buf) {
        *isValid = false;
        return CB_ERROR_BUFFER_CORRUPTED;
    }
    
    if (cb_ptr->size == 0) {
        *isValid = false;
        return CB_ERROR_INVALID_SIZE;
    }
    
    CbIndex in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex out = CB_ATOMIC_LOAD(&cb_ptr->out);
    
    if (in >= cb_ptr->size || out >= cb_ptr->size) {
        *isValid = false;
        return CB_ERROR_BUFFER_CORRUPTED;
    }
    
    CbIndex data_size = (in >= out) ? (in - out) : (cb_ptr->size - out + in);
    if (data_size >= cb_ptr->size) {
        *isValid = false;
        return CB_ERROR_BUFFER_CORRUPTED;
    }
    
    *isValid = true;
    return CB_SUCCESS;
}

bool cb_insert(cb *const cb_ptr, CbItem const item) {
    return cb_insert_ex(cb_ptr, item) == CB_SUCCESS;
}

cb_result_t cb_insert_ex(cb *const cb_ptr, CbItem const item) {
    if (!cb_ptr) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (cb_ptr->size == 0) {
        return CB_ERROR_INVALID_SIZE;
    }
    
    CbIndex current_in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex next_in = (current_in + 1) % cb_ptr->size;
    CbIndex current_out = CB_ATOMIC_LOAD(&cb_ptr->out);
    
    if (next_in == current_out) {
        if (CB_ATOMIC_LOAD(&cb_ptr->overwrite)) {
            // Advance out pointer (overwrite oldest item)
            CB_ATOMIC_STORE(&cb_ptr->out, (current_out + 1) % cb_ptr->size);
            CB_MEMORY_BARRIER();
        } else {
            #if defined(CB_ENABLE_STATISTICS) && CB_ENABLE_STATISTICS
            CbIndex data_size = cb_dataSize(cb_ptr);
            cb_update_stats(cb_ptr, data_size, true, false);
            #endif
            return CB_ERROR_BUFFER_FULL;
        }
    }
    
    cb_ptr->buf[current_in] = item;
    CB_MEMORY_BARRIER();
    CB_ATOMIC_STORE(&cb_ptr->in, next_in);
    
    #if defined(CB_ENABLE_STATISTICS) && CB_ENABLE_STATISTICS
    CbIndex data_size = cb_dataSize(cb_ptr);
    cb_update_stats(cb_ptr, data_size, true, true);
    #endif
        
    return CB_SUCCESS;
}

bool cb_remove(cb *const cb_ptr, CbItem *itemOut) {
    return cb_remove_ex(cb_ptr, itemOut) == CB_SUCCESS;
}

cb_result_t cb_remove_ex(cb *const cb_ptr, CbItem *itemOut) {
    if (!cb_ptr) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!itemOut) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (cb_ptr->size == 0) {
        return CB_ERROR_INVALID_SIZE;
    }
    
    CbIndex current_out = CB_ATOMIC_LOAD(&cb_ptr->out);
    CbIndex current_in = CB_ATOMIC_LOAD(&cb_ptr->in);
    
    if (current_out == current_in) {
        #if defined(CB_ENABLE_STATISTICS) && CB_ENABLE_STATISTICS
        cb_update_stats(cb_ptr, 0, false, false);
        #endif
        return CB_ERROR_BUFFER_EMPTY;
    }
    
    *itemOut = cb_ptr->buf[current_out];
    CB_MEMORY_BARRIER();
    CB_ATOMIC_STORE(&cb_ptr->out, (current_out + 1) % cb_ptr->size);
    
    #if defined(CB_ENABLE_STATISTICS) && CB_ENABLE_STATISTICS
    CbIndex data_size = cb_dataSize(cb_ptr);
    cb_update_stats(cb_ptr, data_size, false, true);
    #endif
    
    return CB_SUCCESS;
}

bool cb_peek(const cb *cb_ptr, CbIndex offset, CbItem *itemOut) {
    return cb_peek_ex(cb_ptr, offset, itemOut) == CB_SUCCESS;
}

cb_result_t cb_peek_ex(const cb *cb_ptr, CbIndex offset, CbItem *itemOut) {
    if (!cb_ptr) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!itemOut) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (cb_ptr->size == 0) {
        return CB_ERROR_INVALID_SIZE;
    }
    
    CbIndex current_out = CB_ATOMIC_LOAD(&cb_ptr->out);
    CbIndex current_in = CB_ATOMIC_LOAD(&cb_ptr->in);
    CbIndex available = (current_in >= current_out) ? 
        (current_in - current_out) : 
        (cb_ptr->size - current_out + current_in);
    
    if (offset >= available) {
        return CB_ERROR_INVALID_OFFSET;
    }
    
    CbIndex pos = (current_out + offset) % cb_ptr->size;
    *itemOut = cb_ptr->buf[pos];
    return CB_SUCCESS;
}

CbIndex cb_insert_bulk(cb *cb_ptr, const CbItem *items, CbIndex count) {
    CbIndex inserted = 0;
    cb_insert_bulk_ex(cb_ptr, items, count, &inserted);
    return inserted;
}

cb_result_t cb_insert_bulk_ex(cb *cb_ptr, const CbItem *items, CbIndex count, CbIndex *inserted) {
    if (!cb_ptr) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!items && count > 0) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!inserted) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (cb_ptr->size == 0) {
        *inserted = 0;
        return CB_ERROR_INVALID_SIZE;
    }
    
    if (count == 0) {
        *inserted = 0;
        return CB_ERROR_INVALID_COUNT;
    }
    
    *inserted = 0;
    bool overwrite_enabled = CB_ATOMIC_LOAD(&cb_ptr->overwrite);
    cb_result_t last_error = CB_SUCCESS;
    
    while (*inserted < count) {
        cb_result_t result = cb_insert_ex(cb_ptr, items[*inserted]);
        
        if (result == CB_SUCCESS) {
            (*inserted)++;
        } else if (result == CB_ERROR_BUFFER_FULL && !overwrite_enabled) {
            last_error = CB_ERROR_BUFFER_FULL;
            break;
        } else {
            last_error = result;
            break;
        }
    }
    
    return (*inserted > 0) ? CB_SUCCESS : last_error;
}

CbIndex cb_remove_bulk(cb *cb_ptr, CbItem *items, CbIndex count) {
    CbIndex removed = 0;
    cb_remove_bulk_ex(cb_ptr, items, count, &removed);
    return removed;
}

cb_result_t cb_remove_bulk_ex(cb *cb_ptr, CbItem *items, CbIndex count, CbIndex *removed) {
    if (!cb_ptr) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!items && count > 0) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!removed) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (cb_ptr->size == 0) {
        *removed = 0;
        return CB_ERROR_INVALID_SIZE;
    }
    
    if (count == 0) {
        *removed = 0;
        return CB_ERROR_INVALID_COUNT;
    }
    
    *removed = 0;
    cb_result_t last_error = CB_SUCCESS;
    
    while (*removed < count) {
        cb_result_t result = cb_remove_ex(cb_ptr, &items[*removed]);
        
        if (result == CB_SUCCESS) {
            (*removed)++;
        } else if (result == CB_ERROR_BUFFER_EMPTY) {
            last_error = CB_ERROR_BUFFER_EMPTY;
            break;
        } else {
            last_error = result;
            break;
        }
    }
    
    return (*removed > 0) ? CB_SUCCESS : last_error;
}

void cb_set_overwrite(cb *cb_ptr, bool enable) {
    cb_set_overwrite_ex(cb_ptr, enable);
}

cb_result_t cb_set_overwrite_ex(cb *cb_ptr, bool enable) {
    if (!cb_ptr) {
        return CB_ERROR_NULL_POINTER;
    }
    
    CB_ATOMIC_STORE(&cb_ptr->overwrite, enable ? 1 : 0);
    CB_MEMORY_BARRIER();
    return CB_SUCCESS;
}

bool cb_get_overwrite(const cb *cb_ptr) {
    bool enabled = false;
    cb_get_overwrite_ex(cb_ptr, &enabled);
    return enabled;
}

cb_result_t cb_get_overwrite_ex(const cb *cb_ptr, bool *enabled) {
    if (!cb_ptr) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!enabled) {
        return CB_ERROR_NULL_POINTER;
    }
    
    *enabled = (CB_ATOMIC_LOAD(&cb_ptr->overwrite) != 0);
    return CB_SUCCESS;
}

const char* cb_error_string(cb_result_t error) {
    switch (error) {
        case CB_SUCCESS:
            return "Success";
        case CB_ERROR_NULL_POINTER:
            return "Null pointer argument";
        case CB_ERROR_INVALID_SIZE:
            return "Invalid buffer size";
        case CB_ERROR_BUFFER_FULL:
            return "Buffer is full";
        case CB_ERROR_BUFFER_EMPTY:
            return "Buffer is empty";
        case CB_ERROR_INVALID_OFFSET:
            return "Invalid offset";
        case CB_ERROR_INVALID_COUNT:
            return "Invalid count parameter";
        case CB_ERROR_BUFFER_CORRUPTED:
            return "Buffer integrity check failed";
        case CB_ERROR_TIMEOUT:
            return "Operation timed out";
        case CB_ERROR_INVALID_PARAMETER:
            return "Invalid parameter value";
        default:
            return "Unknown error";
    }
}

/* Sleep for a short duration in milliseconds */
static void cb_sleep_ms(unsigned long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

bool cb_insert_timeout(cb *const cb_ptr, CbItem const item, uint32_t timeout_ms) {
    cb_result_t result;
    uint32_t elapsed_ms = 0;
    const uint32_t sleep_interval_ms = 1; /* 1ms sleep interval */
    
    /* Try immediate insert first */
    result = cb_insert_ex(cb_ptr, item);
    if (result == CB_SUCCESS) {
        return true;
    }
    
    /* If timeout is 0 or not a buffer full error, return immediately */
    if (timeout_ms == CB_NO_WAIT || result != CB_ERROR_BUFFER_FULL) {
        return false;
    }
    
    /* Keep trying until timeout */
    while (elapsed_ms < timeout_ms) {
        cb_sleep_ms(sleep_interval_ms);
        elapsed_ms += sleep_interval_ms;
        
        result = cb_insert_ex(cb_ptr, item);
        if (result == CB_SUCCESS) {
            return true;
        }
        
        if (result != CB_ERROR_BUFFER_FULL) {
            return false;
        }
    }
    
    /* Timeout occurred */
    CB_SET_ERROR(cb_ptr, CB_ERROR_TIMEOUT, __func__, "timeout_ms", __LINE__);
    return false;
}

bool cb_remove_timeout(cb *const cb_ptr, CbItem *itemOut, uint32_t timeout_ms) {
    cb_result_t result;
    uint32_t elapsed_ms = 0;
    const uint32_t sleep_interval_ms = 1; /* 1ms sleep interval */
    
    /* Try immediate remove first */
    result = cb_remove_ex(cb_ptr, itemOut);
    if (result == CB_SUCCESS) {
        return true;
    }
    
    /* If timeout is 0 or not a buffer empty error, return immediately */
    if (timeout_ms == CB_NO_WAIT || result != CB_ERROR_BUFFER_EMPTY) {
        return false;
    }
    
    /* Keep trying until timeout */
    while (elapsed_ms < timeout_ms) {
        cb_sleep_ms(sleep_interval_ms);
        elapsed_ms += sleep_interval_ms;
        
        result = cb_remove_ex(cb_ptr, itemOut);
        if (result == CB_SUCCESS) {
            return true;
        }
        
        if (result != CB_ERROR_BUFFER_EMPTY) {
            return false;
        }
    }
    
    /* Timeout occurred */
    CB_SET_ERROR(cb_ptr, CB_ERROR_TIMEOUT, __func__, "timeout_ms", __LINE__);
    return false;
}

cb_result_t cb_insert_timeout_ex(cb *const cb_ptr, CbItem const item, uint32_t timeout_ms) {
    cb_result_t result;
    uint32_t elapsed_ms = 0;
    const uint32_t sleep_interval_ms = 1; /* 1ms sleep interval */
    
    /* Validate parameters */
    if (!cb_ptr) {
        return CB_ERROR_NULL_POINTER;
    }
    
    /* Try immediate insert first */
    result = cb_insert_ex(cb_ptr, item);
    if (result == CB_SUCCESS || timeout_ms == CB_NO_WAIT || result != CB_ERROR_BUFFER_FULL) {
        return result;
    }
    
    /* Keep trying until timeout */
    while (elapsed_ms < timeout_ms) {
        cb_sleep_ms(sleep_interval_ms);
        elapsed_ms += sleep_interval_ms;
        
        result = cb_insert_ex(cb_ptr, item);
        if (result == CB_SUCCESS || result != CB_ERROR_BUFFER_FULL) {
            return result;
        }
    }
    
    /* Timeout occurred */
    CB_SET_ERROR(cb_ptr, CB_ERROR_TIMEOUT, __func__, "timeout_ms", __LINE__);
    return CB_ERROR_TIMEOUT;
}

cb_result_t cb_remove_timeout_ex(cb *const cb_ptr, CbItem *itemOut, uint32_t timeout_ms) {
    cb_result_t result;
    uint32_t elapsed_ms = 0;
    const uint32_t sleep_interval_ms = 1; /* 1ms sleep interval */
    
    /* Validate parameters */
    if (!cb_ptr) {
        return CB_ERROR_NULL_POINTER;
    }
    
    if (!itemOut) {
        CB_SET_ERROR(cb_ptr, CB_ERROR_NULL_POINTER, __func__, "itemOut", __LINE__);
        return CB_ERROR_NULL_POINTER;
    }
    
    /* Try immediate remove first */
    result = cb_remove_ex(cb_ptr, itemOut);
    if (result == CB_SUCCESS || timeout_ms == CB_NO_WAIT || result != CB_ERROR_BUFFER_EMPTY) {
        return result;
    }
    
    /* Keep trying until timeout */
    while (elapsed_ms < timeout_ms) {
        cb_sleep_ms(sleep_interval_ms);
        elapsed_ms += sleep_interval_ms;
        
        result = cb_remove_ex(cb_ptr, itemOut);
        if (result == CB_SUCCESS || result != CB_ERROR_BUFFER_EMPTY) {
            return result;
        }
    }
    
    /* Timeout occurred */
    CB_SET_ERROR(cb_ptr, CB_ERROR_TIMEOUT, __func__, "timeout_ms", __LINE__);
    return CB_ERROR_TIMEOUT;
}

cb_error_info_t cb_get_last_error(const cb *cb_ptr) {
    static const cb_error_info_t empty_error = {
        .code = CB_SUCCESS,
        .function = NULL,
        .parameter = NULL,
        .line = 0
    };
    
    if (!cb_ptr) {
        return empty_error;
    }
    
    return cb_ptr->last_error;
}

void cb_clear_error(cb *cb_ptr) {
    if (!cb_ptr) {
        return;
    }
    
    cb_ptr->last_error.code = CB_SUCCESS;
    cb_ptr->last_error.function = NULL;
    cb_ptr->last_error.parameter = NULL;
    cb_ptr->last_error.line = 0;
}

void cb_reset_stats(cb *cb_ptr) {
#if defined(CB_ENABLE_STATISTICS) && CB_ENABLE_STATISTICS
    int idx = cb_find_or_register_buffer(cb_ptr);
    if (idx < 0) return;
    
    memset(&cb_stats[idx], 0, sizeof(cb_internal_stats_t));
#else
    (void)cb_ptr; /* Unused parameter */
#endif
}

cb_stats_t cb_get_stats(const cb *cb_ptr) {
    cb_stats_t stats = {0};
    
#if defined(CB_ENABLE_STATISTICS) && CB_ENABLE_STATISTICS
    int idx;
    
    if (!cb_ptr) {
        return stats;
    }
    
    /* Find the buffer in our registry */
    for (idx = 0; idx < cb_instance_count; idx++) {
        if (cb_instances[idx] == cb_ptr) {
            stats.peak_usage = cb_stats[idx].peak_usage;
            stats.total_inserts = cb_stats[idx].total_inserts;
            stats.total_removes = cb_stats[idx].total_removes;
            stats.overflow_count = cb_stats[idx].overflow_count;
            stats.underflow_count = cb_stats[idx].underflow_count;
            break;
        }
    }
#else
    (void)cb_ptr; /* Unused parameter */
#endif
    
    return stats;
}
