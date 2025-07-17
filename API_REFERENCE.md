# Circular Buffer (cb) Library API Reference

## Table of Contents

1. [Introduction](#introduction)
2. [API Design Philosophy](#api-design-philosophy)
3. [Data Types](#data-types)
4. [Error Handling](#error-handling)
5. [Initialization Functions](#initialization-functions)
6. [Core Operations](#core-operations)
7. [Bulk Operations](#bulk-operations)
8. [Timeout Operations](#timeout-operations)
9. [State Information Functions](#state-information-functions)
10. [Overwrite Control](#overwrite-control)
11. [Error String Utilities](#error-string-utilities)
12. [Statistics Functions](#statistics-functions)
13. [Configuration Options](#configuration-options)
14. [Memory Barriers](#memory-barriers)
15. [Thread Safety Considerations](#thread-safety-considerations)
16. [Performance Considerations](#performance-considerations)
17. [Usage Patterns](#usage-patterns)

## Introduction

The Circular Buffer (cb) library provides a lock-free, thread-safe implementation of a ring buffer suitable for embedded systems, real-time applications, and multi-threaded environments. The library is designed for single-producer, single-consumer scenarios and provides comprehensive error handling.

This API reference documents all functions, data types, and usage patterns for the library.

## API Design Philosophy

The library provides two complementary APIs to serve different use cases:

### Simple API

The Simple API provides boolean returns for basic operations and is designed for:
- Embedded systems with limited resources
- Prototyping and simple applications
- Legacy code integration
- Performance-critical operations (e.g., ISR handlers)

### Detailed API

The Detailed API provides specific error codes and is designed for:
- Production systems requiring robust error handling
- Safety-critical applications
- Debugging complex issues
- Handling partial success in bulk operations
- Systems requiring detailed error reporting

Both APIs operate on the same underlying data structures and can be used interchangeably within the same application.

## Data Types

### Core Types

```c
typedef CB_WORD_INDEX_TYPE     CbIndex;      // Platform-sized index type
typedef CB_ATOMIC_INDEX_TYPE   CbAtomicIndex; // Platform-safe atomic index
typedef CB_ITEM_TYPE           CbItem;       // Buffer item type (configurable)
```

### Buffer Structure

```c
typedef struct
{
    CbItem *buf;              // Pointer to buffer storage
    CbIndex size;             // Buffer size in items
    CbAtomicIndex in;         // Producer index (atomic)
    CbAtomicIndex out;        // Consumer index (atomic)
    CbAtomicIndex overwrite;  // Overwrite mode flag (atomic)
    cb_error_info_t last_error; // Last error information
} cb;
```

### Error Codes

```c
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
```

### Error Context Information

```c
typedef struct {
    cb_result_t code;               // Error code
    const char* function;           // Function where error occurred
    const char* parameter;          // Parameter that caused the error (if applicable)
    int line;                       // Line number where error occurred
} cb_error_info_t;
```

### Statistics Structure

```c
typedef struct {
    CbIndex peak_usage;             // Maximum number of items in buffer
    CbIndex total_inserts;          // Total number of successful inserts
    CbIndex total_removes;          // Total number of successful removes
    CbIndex overflow_count;         // Number of failed inserts due to buffer full
    CbIndex underflow_count;        // Number of failed removes due to buffer empty
} cb_stats_t;
```

## Error Handling

The library provides two approaches to error handling:

### Simple API Error Handling

Functions in the Simple API return boolean values:
- `true` indicates success
- `false` indicates failure

The specific cause of failure is not indicated, making this API suitable for simple use cases where detailed error information is not required.

### Detailed API Error Handling

Functions in the Detailed API return `cb_result_t` error codes:
- `CB_SUCCESS` indicates successful operation
- Other values indicate specific error conditions

This approach provides detailed information about the cause of failures, making it suitable for robust error handling in production systems.

### Error Context Information

The library provides detailed error context information through the `cb_error_info_t` structure:

```c
typedef struct {
    cb_result_t code;               // Error code
    const char* function;           // Function where error occurred
    const char* parameter;          // Parameter that caused the error (if applicable)
    int line;                       // Line number where error occurred
} cb_error_info_t;
```

You can retrieve the last error information using:

```c
cb_error_info_t cb_get_last_error(const cb *cb_ptr);
void cb_clear_error(cb *cb_ptr);
```

This provides much more detailed information about what went wrong, which is especially useful for debugging complex issues.

## Initialization Functions

### Simple API

```c
void cb_init(cb *const cb_ptr, CbItem buffer[], CbIndex length);
```

Initializes a circular buffer with the provided storage array.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer structure
- `buffer`: Array to use for buffer storage
- `length`: Size of the buffer array in items

**Returns:** None

**Notes:**
- If `cb_ptr` is NULL or `length` is 0, the function will silently fail
- The buffer size is limited to `CbIndex` maximum value minus 1

### Detailed API

```c
cb_result_t cb_init_ex(cb *const cb_ptr, CbItem buffer[], CbIndex length);
```

Initializes a circular buffer with the provided storage array, with detailed error reporting.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer structure
- `buffer`: Array to use for buffer storage
- `length`: Size of the buffer array in items

**Returns:**
- `CB_SUCCESS`: Buffer initialized successfully
- `CB_ERROR_NULL_POINTER`: `cb_ptr` or `buffer` is NULL
- `CB_ERROR_INVALID_SIZE`: `length` is 0

**Notes:**
- Even with invalid parameters, the function will initialize all fields of the buffer structure

## Core Operations

### Simple API

```c
bool cb_insert(cb *const cb_ptr, CbItem const item);
```

Inserts an item into the buffer.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `item`: Item to insert

**Returns:**
- `true`: Item inserted successfully
- `false`: Buffer is full (and overwrite mode is disabled) or invalid parameters

**Notes:**
- This function internally calls `cb_insert_ex` and returns true if the result is `CB_SUCCESS`

```c
bool cb_remove(cb *const cb_ptr, CbItem *itemOut);
```

Removes an item from the buffer.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `itemOut`: Pointer to store the removed item

**Returns:**
- `true`: Item removed successfully
- `false`: Buffer is empty or invalid parameters

**Notes:**
- This function internally calls `cb_remove_ex` and returns true if the result is `CB_SUCCESS`

```c
bool cb_peek(const cb *cb_ptr, CbIndex offset, CbItem *itemOut);
```

Reads an item from the buffer without removing it.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `offset`: Offset from the oldest item (0 = oldest item)
- `itemOut`: Pointer to store the peeked item

**Returns:**
- `true`: Item peeked successfully
- `false`: Offset beyond available data or invalid parameters

**Notes:**
- This function internally calls `cb_peek_ex` and returns true if the result is `CB_SUCCESS`

### Detailed API

```c
cb_result_t cb_insert_ex(cb *const cb_ptr, CbItem item);
```

Inserts an item into the buffer with detailed error reporting.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `item`: Item to insert

**Returns:**
- `CB_SUCCESS`: Item inserted successfully
- `CB_ERROR_NULL_POINTER`: `cb_ptr` is NULL
- `CB_ERROR_INVALID_SIZE`: Buffer size is 0
- `CB_ERROR_BUFFER_FULL`: Buffer is full and overwrite mode is disabled

**Notes:**
- If overwrite mode is enabled and the buffer is full, the oldest item will be overwritten

```c
cb_result_t cb_remove_ex(cb *const cb_ptr, CbItem *itemOut);
```

Removes an item from the buffer with detailed error reporting.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `itemOut`: Pointer to store the removed item

**Returns:**
- `CB_SUCCESS`: Item removed successfully
- `CB_ERROR_NULL_POINTER`: `cb_ptr` or `itemOut` is NULL
- `CB_ERROR_INVALID_SIZE`: Buffer size is 0
- `CB_ERROR_BUFFER_EMPTY`: Buffer is empty

```c
cb_result_t cb_peek_ex(const cb *cb_ptr, CbIndex offset, CbItem *itemOut);
```

Reads an item from the buffer without removing it, with detailed error reporting.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `offset`: Offset from the oldest item (0 = oldest item)
- `itemOut`: Pointer to store the peeked item

**Returns:**
- `CB_SUCCESS`: Item peeked successfully
- `CB_ERROR_NULL_POINTER`: `cb_ptr` or `itemOut` is NULL
- `CB_ERROR_INVALID_SIZE`: Buffer size is 0
- `CB_ERROR_INVALID_OFFSET`: Offset beyond available data

## Bulk Operations

```c
CbIndex cb_insert_bulk(cb *cb_ptr, const CbItem *items, CbIndex count);
```

Inserts multiple items into the buffer.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `items`: Array of items to insert
- `count`: Number of items to insert

**Returns:**
- Number of items successfully inserted (may be less than `count` if buffer fills up)
- 0 if invalid parameters

**Notes:**
- This function internally calls `cb_insert_bulk_ex` and returns the number of inserted items

```c
CbIndex cb_remove_bulk(cb *cb_ptr, CbItem *items, CbIndex count);
```

Removes multiple items from the buffer.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `items`: Array to store removed items
- `count`: Maximum number of items to remove

**Returns:**
- Number of items successfully removed (may be less than `count` if buffer empties)
- 0 if invalid parameters

**Notes:**
- This function internally calls `cb_remove_bulk_ex` and returns the number of removed items

### Detailed API

```c
cb_result_t cb_insert_bulk_ex(cb *cb_ptr, const CbItem *items, CbIndex count, CbIndex *inserted);
```

Inserts multiple items into the buffer with detailed error reporting.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `items`: Array of items to insert
- `count`: Number of items to insert
- `inserted`: Pointer to store the number of items successfully inserted

**Returns:**
- `CB_SUCCESS`: At least one item was inserted successfully
- `CB_ERROR_NULL_POINTER`: `cb_ptr`, `items` (when count > 0), or `inserted` is NULL
- `CB_ERROR_INVALID_SIZE`: Buffer size is 0
- `CB_ERROR_INVALID_COUNT`: `count` is 0
- `CB_ERROR_BUFFER_FULL`: Buffer is full and no items were inserted

```c
cb_result_t cb_remove_bulk_ex(cb *cb_ptr, CbItem *items, CbIndex count, CbIndex *removed);
```

Removes multiple items from the buffer with detailed error reporting.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `items`: Array to store removed items
- `count`: Maximum number of items to remove
- `removed`: Pointer to store the number of items successfully removed

**Returns:**
- `CB_SUCCESS`: At least one item was removed successfully
- `CB_ERROR_NULL_POINTER`: `cb_ptr`, `items` (when count > 0), or `removed` is NULL
- `CB_ERROR_INVALID_SIZE`: Buffer size is 0
- `CB_ERROR_INVALID_COUNT`: `count` is 0
- `CB_ERROR_BUFFER_EMPTY`: Buffer is empty and no items were removed

## Timeout Operations

The library provides timeout variants for insert and remove operations, allowing for non-blocking operations with a configurable timeout:

### Simple API

```c
bool cb_insert_timeout(cb *const cb_ptr, CbItem const item, uint32_t timeout_ms);
bool cb_remove_timeout(cb *const cb_ptr, CbItem *itemOut, uint32_t timeout_ms);
```

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `item`/`itemOut`: Item to insert or pointer to store removed item
- `timeout_ms`: Maximum time to wait in milliseconds (0 for immediate return)

**Returns:**
- `true`: Operation completed successfully within the timeout period
- `false`: Operation failed or timed out

### Detailed API

```c
cb_result_t cb_insert_timeout_ex(cb *const cb_ptr, CbItem const item, uint32_t timeout_ms);
cb_result_t cb_remove_timeout_ex(cb *const cb_ptr, CbItem *itemOut, uint32_t timeout_ms);
```

**Parameters:**
- Same as simple API

**Returns:**
- `CB_SUCCESS`: Operation completed successfully within the timeout period
- `CB_ERROR_TIMEOUT`: Operation timed out
- Other error codes as appropriate

### Usage Example

```c
// Try to insert with a 100ms timeout
if (cb_insert_timeout(&buffer, data, 100)) {
    // Success
} else {
    cb_error_info_t error = cb_get_last_error(&buffer);
    if (error.code == CB_ERROR_TIMEOUT) {
        // Handle timeout
    } else {
        // Handle other errors
    }
}
```

Timeout operations are particularly useful in real-time systems where operations must complete within a specific time window.

## State Information Functions

### Simple API

```c
CbIndex cb_freeSpace(cb *const cb_ptr);
```

Gets the number of free slots in the buffer.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer

**Returns:**
- Number of free slots
- 0 if invalid parameters or buffer size is 0

```c
CbIndex cb_dataSize(cb *const cb_ptr);
```

Gets the number of items currently in the buffer.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer

**Returns:**
- Number of items in the buffer
- 0 if invalid parameters or buffer size is 0

```c
bool cb_sanity_check(const cb *cb_ptr);
```

Performs an integrity check on the buffer.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer

**Returns:**
- `true`: Buffer is valid
- `false`: Buffer is corrupted or invalid parameters

### Detailed API

```c
cb_result_t cb_freeSpace_ex(cb *const cb_ptr, CbIndex *freeSpace);
```

Gets the number of free slots in the buffer with detailed error reporting.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `freeSpace`: Pointer to store the number of free slots

**Returns:**
- `CB_SUCCESS`: Free space calculated successfully
- `CB_ERROR_NULL_POINTER`: `cb_ptr` or `freeSpace` is NULL
- `CB_ERROR_INVALID_SIZE`: Buffer size is 0

```c
cb_result_t cb_dataSize_ex(cb *const cb_ptr, CbIndex *dataSize);
```

Gets the number of items currently in the buffer with detailed error reporting.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `dataSize`: Pointer to store the number of items

**Returns:**
- `CB_SUCCESS`: Data size calculated successfully
- `CB_ERROR_NULL_POINTER`: `cb_ptr` or `dataSize` is NULL
- `CB_ERROR_INVALID_SIZE`: Buffer size is 0

```c
cb_result_t cb_sanity_check_ex(const cb *cb_ptr, bool *isValid);
```

Performs an integrity check on the buffer with detailed error reporting.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `isValid`: Pointer to store the validity result

**Returns:**
- `CB_SUCCESS`: Buffer is valid
- `CB_ERROR_NULL_POINTER`: `cb_ptr` or `isValid` is NULL
- `CB_ERROR_INVALID_SIZE`: Buffer size is 0
- `CB_ERROR_BUFFER_CORRUPTED`: Buffer is corrupted

## Overwrite Control

### Simple API

```c
void cb_set_overwrite(cb *cb_ptr, bool enable);
```

Enables or disables overwrite mode.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `enable`: `true` to enable overwrite mode, `false` to disable

**Returns:** None

**Notes:**
- This function internally calls `cb_set_overwrite_ex` and ignores the return value

```c
bool cb_get_overwrite(const cb *cb_ptr);
```

Gets the current overwrite mode setting.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer

**Returns:**
- `true`: Overwrite mode is enabled
- `false`: Overwrite mode is disabled or invalid parameters

**Notes:**
- This function internally calls `cb_get_overwrite_ex` and returns the value stored in the output parameter

### Detailed API

```c
cb_result_t cb_set_overwrite_ex(cb *cb_ptr, bool enable);
```

Enables or disables overwrite mode with detailed error reporting.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `enable`: `true` to enable overwrite mode, `false` to disable

**Returns:**
- `CB_SUCCESS`: Overwrite mode set successfully
- `CB_ERROR_NULL_POINTER`: `cb_ptr` is NULL

```c
cb_result_t cb_get_overwrite_ex(const cb *cb_ptr, bool *enabled);
```

Gets the current overwrite mode setting with detailed error reporting.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer
- `enabled`: Pointer to store the overwrite mode setting

**Returns:**
- `CB_SUCCESS`: Overwrite mode retrieved successfully
- `CB_ERROR_NULL_POINTER`: `cb_ptr` or `enabled` is NULL

## Error String Utilities

```c
const char* cb_error_string(cb_result_t error);
```

Gets a human-readable string for an error code.

**Parameters:**
- `error`: Error code to convert

**Returns:**
- Pointer to a static string describing the error
- "Unknown error" for unrecognized error codes

```c
cb_error_info_t cb_get_last_error(const cb *cb_ptr);
```

Gets the last error information for a buffer.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer

**Returns:**
- Structure containing error code, function name, parameter name, and line number
- Empty structure with `CB_SUCCESS` if `cb_ptr` is NULL

```c
void cb_clear_error(cb *cb_ptr);
```

Clears the last error information for a buffer.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer

**Returns:** None

## Statistics Functions

```c
void cb_reset_stats(cb *cb_ptr);
```

Resets the statistics for a buffer.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer

**Returns:** None

```c
cb_stats_t cb_get_stats(const cb *cb_ptr);
```

Gets the statistics for a buffer.

**Parameters:**
- `cb_ptr`: Pointer to the circular buffer

**Returns:**
- Structure containing peak usage, insert/remove counts, and overflow/underflow counts
- Empty structure if `cb_ptr` is NULL or buffer not found in registry

## Configuration Options

### Buffer Item Type

The default item type is `uint8_t`. To use a different type, define `CB_ITEM_TYPE` before including the header:

```c
#define CB_ITEM_TYPE uint32_t
#include "cb.h"
```

### Memory Barriers

Memory barriers can be enabled or disabled:

```c
#define CB_USE_MEMORY_BARRIERS 1  // Enable memory barriers (default)
#define CB_USE_MEMORY_BARRIERS 0  // Disable memory barriers
```

### C11 Atomics

The library automatically detects and uses C11 atomics when available:

```c
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ATOMICS__)
    #define CB_HAS_C11_ATOMICS 1
    #include <stdatomic.h>
#else
    #define CB_HAS_C11_ATOMICS 0
#endif
```

## Memory Barriers

Memory barriers ensure correct memory ordering between threads, which is critical for lock-free data structures. The library uses platform-specific memory barrier implementations when available.

### When Memory Barriers Are Required

Memory barriers are required when:
- Running on multi-core systems
- Using the buffer across multiple threads
- Using the buffer between threads and ISRs

### When Memory Barriers Can Be Disabled

Memory barriers can be disabled when:
- Running on single-core systems with no preemption
- Using the buffer exclusively within a single thread
- Performance is critical and you understand the memory ordering implications

## Thread Safety Considerations

The circular buffer is designed for single-producer, single-consumer scenarios. This means:

- One thread/ISR can call `cb_insert()` and related functions
- One thread/ISR can call `cb_remove()` and related functions
- Both can happen concurrently without locks

### Not Thread-Safe Operations

The following operations are not thread-safe and should not be called concurrently:
- Multiple producers calling `cb_insert()` concurrently
- Multiple consumers calling `cb_remove()` concurrently
- Calling `cb_set_overwrite()` while other operations are in progress

### Thread-Safe Operations

The following operations are thread-safe:
- One producer calling `cb_insert()` while one consumer calls `cb_remove()`
- One thread calling `cb_insert()` while another calls `cb_dataSize()`
- One thread calling `cb_remove()` while another calls `cb_freeSpace()`

## Performance Considerations

### Optimization Tips

1. **Disable Memory Barriers** when not needed (single-core systems)
2. **Use Bulk Operations** for transferring multiple items
3. **Pre-check Buffer State** before operations in tight loops:
   ```c
   if (cb_dataSize(&my_buffer) >= needed_items) {
       cb_remove_bulk(&my_buffer, items, needed_items);
   }
   ```
4. **Buffer Size Power of 2** can improve performance on some platforms

### Performance Metrics

| Operation | x86 (ns) | ARM Cortex-M (ns) | RISC-V (ns) |
|-----------|----------|-------------------|-------------|
| Insert    | 15-25    | 40-60             | 50-80       |
| Remove    | 12-22    | 35-55             | 45-75       |
| Bulk (16) | 80-120   | 200-300           | 250-400     |

*Measured on 128-byte buffer, 3.2GHz x86, 72MHz ARM Cortex-M4, 100MHz RISC-V*

## Usage Patterns

### Producer-Consumer Pattern

```c
// Producer thread
void producer_task(void) {
    while (1) {
        CbItem item = produce_item();
        while (!cb_insert(&buffer, item)) {
            // Buffer full, wait or handle
        }
    }
}

// Consumer thread
void consumer_task(void) {
    while (1) {
        CbItem item;
        if (cb_remove(&buffer, &item)) {
            consume_item(item);
        } else {
            // Buffer empty, wait or handle
        }
    }
}
```

### ISR-Safe Usage

```c
// ISR (producer)
void isr_handler(void) {
    CbItem item = read_sensor();
    cb_insert(&buffer, item);  // Non-blocking
}

// Main thread (consumer)
void main_loop(void) {
    CbItem item;
    if (cb_remove(&buffer, &item)) {
        process_data(item);
    }
}
```

### Streaming Data Pattern with Overwrite

```c
// Initialize with overwrite mode
cb_init(&buffer, storage, BUFFER_SIZE);
cb_set_overwrite(&buffer, true);

// Producer (always succeeds)
cb_insert(&buffer, new_data);

// Consumer (gets most recent data if available)
CbItem latest_data;
if (cb_remove(&buffer, &latest_data)) {
    process_latest_data(latest_data);
}
```

### Batch Processing Pattern

```c
#define BATCH_SIZE 16
CbItem batch[BATCH_SIZE];

// Producer
CbIndex inserted = cb_insert_bulk(&buffer, batch, BATCH_SIZE);
if (inserted < BATCH_SIZE) {
    // Handle partial insertion
}

// Consumer
CbIndex available = cb_dataSize(&buffer);
if (available >= MIN_BATCH_SIZE) {
    CbIndex removed = cb_remove_bulk(&buffer, batch, MIN_BATCH_SIZE);
    process_batch(batch, removed);
}
```

### Timeout Pattern

```c
// Producer with timeout
if (!cb_insert_timeout(&buffer, item, 100)) {
    cb_error_info_t error = cb_get_last_error(&buffer);
    if (error.code == CB_ERROR_TIMEOUT) {
        // Handle timeout (e.g., discard data, log warning)
    } else {
        // Handle other errors
    }
}

// Consumer with timeout
CbItem item;
if (!cb_remove_timeout(&buffer, &item, 50)) {
    cb_error_info_t error = cb_get_last_error(&buffer);
    if (error.code == CB_ERROR_TIMEOUT) {
        // Handle timeout (e.g., use default value, skip processing)
    } else {
        // Handle other errors
    }
}
```
