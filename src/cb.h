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

#ifndef CB_H
#define CB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Check for C11 atomics support */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ATOMICS__)
    #define CB_HAS_C11_ATOMICS 1
    #include <stdatomic.h>
#else
    #define CB_HAS_C11_ATOMICS 0
#endif

#include "cb_wordsize_detect.h"
#include "cb_atomicindex_detect.h"
#include "cb_memorybarrier_detect.h"
#include "cb_atomic_access.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Configurable item type */
#ifndef CB_ITEM_TYPE
    #define CB_ITEM_TYPE uint8_t
#endif

typedef CB_WORD_INDEX_TYPE     CbIndex;
typedef CB_ATOMIC_INDEX_TYPE   CbAtomicIndex;
typedef CB_ITEM_TYPE           CbItem;

/* Timeout value for non-blocking operations */
#define CB_NO_WAIT 0

/* Error codes for enhanced error handling */
typedef enum {
    CB_SUCCESS = 0,                 // Operation completed successfully
    CB_ERROR_NULL_POINTER,          // Null pointer passed as argument
    CB_ERROR_INVALID_SIZE,          // Invalid buffer size (zero or too large)
    CB_ERROR_BUFFER_FULL,           // Buffer is full and overwrite is disabled
    CB_ERROR_BUFFER_EMPTY,          // Buffer is empty
    CB_ERROR_INVALID_OFFSET,        // Peek offset is beyond available data
    CB_ERROR_INVALID_COUNT,         // Invalid count parameter for bulk operations
    CB_ERROR_BUFFER_CORRUPTED,      // Buffer integrity check failed
    CB_ERROR_TIMEOUT,               // Operation timed out
    CB_ERROR_INVALID_PARAMETER      // Invalid parameter value
} cb_result_t;

/* Error context information */
typedef struct {
    cb_result_t code;               // Error code
    const char* function;           // Function where error occurred
    const char* parameter;          // Parameter that caused the error (if applicable)
    int line;                       // Line number where error occurred
} cb_error_info_t;

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

/* Buffer structure */
typedef struct
{
    CbItem *buf;
    CbIndex size;
    
#if CB_HAS_C11_ATOMICS
    atomic_uint in;
    atomic_uint out;
    atomic_bool overwrite;
#else
    CbAtomicIndex in;
    CbAtomicIndex out;
    CbAtomicIndex overwrite;  // Overwrite mode flag
#endif

    /* Last error information */
    cb_error_info_t last_error;
} cb;

/* Initialization */
void cb_init(cb *const cb_ptr, CbItem bufferStorage[], CbIndex bufferLength);
cb_result_t cb_init_ex(cb *const cb_ptr, CbItem bufferStorage[], CbIndex bufferLength);

/* State */
CbIndex cb_freeSpace(cb *const cb_ptr);
CbIndex cb_dataSize(cb *const cb_ptr);
bool cb_sanity_check(const cb *cb_ptr);

/* Enhanced state functions with error codes */
cb_result_t cb_freeSpace_ex(cb *const cb_ptr, CbIndex *freeSpace);
cb_result_t cb_dataSize_ex(cb *const cb_ptr, CbIndex *dataSize);
cb_result_t cb_sanity_check_ex(const cb *cb_ptr, bool *isValid);

/* Operations */
bool cb_insert(cb *const cb_ptr, CbItem const item);
bool cb_remove(cb *const cb_ptr, CbItem *itemOut);
bool cb_peek(const cb *cb_ptr, CbIndex offset, CbItem *itemOut);

/* Enhanced operations with error codes */
cb_result_t cb_insert_ex(cb *const cb_ptr, CbItem const item);
cb_result_t cb_remove_ex(cb *const cb_ptr, CbItem *itemOut);
cb_result_t cb_peek_ex(const cb *cb_ptr, CbIndex offset, CbItem *itemOut);

/* Timeout operations */
bool cb_insert_timeout(cb *const cb_ptr, CbItem const item, uint32_t timeout_ms);
bool cb_remove_timeout(cb *const cb_ptr, CbItem *itemOut, uint32_t timeout_ms);
cb_result_t cb_insert_timeout_ex(cb *const cb_ptr, CbItem const item, uint32_t timeout_ms);
cb_result_t cb_remove_timeout_ex(cb *const cb_ptr, CbItem *itemOut, uint32_t timeout_ms);

/* Bulk operations */
CbIndex cb_insert_bulk(cb *cb_ptr, const CbItem *items, CbIndex count);
CbIndex cb_remove_bulk(cb *cb_ptr, CbItem *items, CbIndex count);

/* Enhanced bulk operations with error codes */
cb_result_t cb_insert_bulk_ex(cb *cb_ptr, const CbItem *items, CbIndex count, CbIndex *inserted);
cb_result_t cb_remove_bulk_ex(cb *cb_ptr, CbItem *items, CbIndex count, CbIndex *removed);

/* Overwrite control */
void cb_set_overwrite(cb *cb_ptr, bool enable);
bool cb_get_overwrite(const cb *cb_ptr);

/* Enhanced overwrite control with error codes */
cb_result_t cb_set_overwrite_ex(cb *cb_ptr, bool enable);
cb_result_t cb_get_overwrite_ex(const cb *cb_ptr, bool *enabled);

/* Error handling utilities */
const char* cb_error_string(cb_result_t error);
cb_error_info_t cb_get_last_error(const cb *cb_ptr);
void cb_clear_error(cb *cb_ptr);

/* Statistics */
typedef struct {
    CbIndex peak_usage;             // Maximum number of items in buffer
    CbIndex total_inserts;          // Total number of successful inserts
    CbIndex total_removes;          // Total number of successful removes
    CbIndex overflow_count;         // Number of failed inserts due to buffer full
    CbIndex underflow_count;        // Number of failed removes due to buffer empty
} cb_stats_t;

/* Statistics functions */
void cb_reset_stats(cb *cb_ptr);
cb_stats_t cb_get_stats(const cb *cb_ptr);

#ifdef __cplusplus
}
#endif

#endif /* CB_H */