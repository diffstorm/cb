# Circular Buffer (cb) Library [![Awesome](https://awesome.re/badge.svg)](https://github.com/diffstorm/cb)

[![Build Status](https://github.com/diffstorm/cb/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/diffstorm/cb/actions)
[![License](https://img.shields.io/github/license/diffstorm/cb)](https://github.com/diffstorm/cb/blob/main/LICENSE)
[![Language](https://img.shields.io/github/languages/top/diffstorm/cb)](https://github.com/diffstorm/cb)
[![Code Coverage](https://codecov.io/gh/diffstorm/cb/branch/main/graph/badge.svg)](https://codecov.io/gh/diffstorm/cb)
![GitHub Stars](https://img.shields.io/github/stars/diffstorm/cb?style=social)
![Platforms](https://img.shields.io/badge/platforms-Linux%20|%20Windows%20|%20macOS%20|%20Embedded-lightgrey)
![Version](https://img.shields.io/badge/version-1.1-blue)

A lock-free circular buffer for embedded systems, real-time applications, and multi-threaded environments. This library provides a thread-safe ring buffer with ISR-safe operations and comprehensive error handling.

## Table of Contents

- [Features](#features)
- [Getting Started](#getting-started)
- [API Reference](#api-reference)
- [Usage Examples](#usage-examples)
- [Recent Enhancements](#recent-enhancements)
- [Configuration Options](#configuration-options)
- [Platform Support](#platform-support)
- [Performance](#performance)
- [Support](#support)

## Features

- **Lock-free design**: Works safely with one producer and one consumer
- **Dual API**: Simple boolean returns or detailed error codes ([see API Reference](API_REFERENCE.md#api-design-philosophy))
- **Cross-platform**: Runs on Linux, Windows, macOS, and embedded systems
- **Zero dynamic memory**: Uses only static buffer allocation
- **ISR-safe**: Safe for interrupt service routines
- **Memory barriers**: Ensures correct memory ordering between threads
- **Thoroughly tested**: 37 tests with 100% pass rate
- **Bulk operations**: Efficiently transfer multiple items at once
- **Overwrite mode**: Optional automatic overwrite of oldest data
- **Peek functionality**: Read data without removing it
- **Buffer validation**: Integrity checks to detect corruption

## Getting Started

### Prerequisites

- C compiler (GCC, Clang, MSVC, or embedded toolchain)
- CMake (>= 3.10)
- GoogleTest (for running tests)

### Building

```bash
# Clone the repository
git clone https://github.com/diffstorm/cb.git
cd cb

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make
```

### Demos

```bash
# Run lock-free demo
./demo

# Run mutex-protected demo
./demo_mutex

# Run bulk operations demo
./demo_bulk

# Run overwrite mode demo
./demo_overwrite

# Run enhanced error handling demo
./demo_enhanced

# Run statistics demo
./demo_stats

# Run timeout operations demo
./demo_timeout
```

### Tests

```bash
# Run basic tests
./tests/test_basic

# Run advanced tests
./tests/test_advanced

# Run error handling tests
./tests/test_error

# Run timeout functionality tests
./tests/test_timeout

# Run statistics tests
./tests/test_stats
```

## API Reference

For detailed API documentation, please see the [API Reference](API_REFERENCE.md) file.

The library provides two complementary APIs:

### Simple API vs Detailed API

| Simple API | Detailed API |
|------------|--------------|
| Returns `true`/`false` | Returns specific error codes |
| Minimal overhead | Comprehensive error information |
| Good for embedded systems | Good for production systems |
| Best for prototyping | Best for safety-critical applications |

For guidance on when to use each API style, see the [API Design Philosophy](API_REFERENCE.md#api-design-philosophy) section in the API Reference.

### Core Functions Overview

The library provides functions in these categories:

- **Initialization**: Initialize buffer with static storage
- **Basic Operations**: Insert, remove, and peek operations
- **Bulk Operations**: Efficiently transfer multiple items at once
- **State Information**: Get buffer status and validate integrity
- **Overwrite Control**: Configure automatic overwrite behavior
- **Error Handling**: Get human-readable error messages

Each function has both a simple version (e.g., `cb_insert()`) and a detailed version with error codes (e.g., `cb_insert_ex()`).

See the [API Reference](API_REFERENCE.md) for complete function signatures and detailed documentation.

## Usage Examples

The following examples demonstrate basic usage patterns. For more advanced usage patterns including thread-safe producer-consumer scenarios, ISR-safe usage, and batch processing, see the [Usage Patterns](API_REFERENCE.md#usage-patterns) section in the API Reference.

### Simple Example

```c
#include "cb.h"

#define BUFFER_SIZE 128
CbItem storage[BUFFER_SIZE];
cb my_buffer;

// Initialize buffer
cb_init(&my_buffer, storage, BUFFER_SIZE);

// Producer: Add data
if (cb_insert(&my_buffer, sensor_reading)) {
    // Success
} else {
    // Buffer full
}

// Consumer: Get data
CbItem data;
if (cb_remove(&my_buffer, &data)) {
    process_data(data);
}
```

### Production Example

```c
#include "cb.h"

#define BUFFER_SIZE 128
CbItem storage[BUFFER_SIZE];
cb my_buffer;

// Initialize with error checking
cb_result_t result = cb_init_ex(&my_buffer, storage, BUFFER_SIZE);
if (result != CB_SUCCESS) {
    log_error("Buffer init failed: %s", cb_error_string(result));
    return -1;
}

// Producer with detailed error handling
result = cb_insert_ex(&my_buffer, sensor_reading);
switch (result) {
    case CB_SUCCESS:
        break;
    case CB_ERROR_BUFFER_FULL:
        // Enable overwrite mode and retry
        cb_set_overwrite(&my_buffer, true);
        cb_insert_ex(&my_buffer, sensor_reading);
        break;
    default:
        log_error("Insert failed: %s", cb_error_string(result));
        break;
}
```

## Recent Enhancements

### Zero-Size Buffer Safety
- Fixed division by zero crashes
- Added comprehensive size validation
- Ensures crash-free operation

### Enhanced Error Handling
- Added detailed error codes
- Added parallel `*_ex()` functions
- Maintained 100% backward compatibility
- Added human-readable error strings
- Added error context information with function and line tracking

### Configurable Item Type
- Added support for custom item types via `CB_ITEM_TYPE` define
- Maintained backward compatibility with default `uint8_t` type

### Timeout Operations
- Added timeout variants for insert and remove operations
- Non-blocking with configurable timeout duration
- Both simple and extended API versions

### Statistics Tracking
- Added buffer usage statistics
- Track peak usage, insert/remove counts, and overflow/underflow events
- Reset and query functions for monitoring

### C11 Atomics Support
- Added automatic detection and use of C11 atomics when available
- Maintained backward compatibility with older compilers
- Improved memory model for modern platforms

### Expanded Test Coverage
- 44 comprehensive tests across four test suites
- 100% test pass rate

## Thread Safety

The circular buffer is designed for single-producer, single-consumer scenarios:
- One thread/ISR can safely call insert functions
- Another thread/ISR can safely call remove functions
- Both can happen concurrently without locks

For detailed thread safety guidelines and limitations, see the [Thread Safety Considerations](API_REFERENCE.md#thread-safety-considerations) section in the API Reference.

## Configuration Options

The library provides several configuration options:

- **Memory Barriers**: Enable/disable with `#define CB_USE_MEMORY_BARRIERS`
- **Overwrite Mode**: Control with `cb_set_overwrite()` function
- **Item Type**: Customize with `#define CB_ITEM_TYPE`

See the [API Reference](API_REFERENCE.md) for detailed configuration information.

## Platform Support
- Linux, Windows, macOS
- Embedded systems (ARM Cortex, AVR, MSP430, RISC-V)
- RTOS environments (FreeRTOS, Zephyr, VxWorks)
- Multiple compilers (GCC, Clang, MSVC, IAR, Keil)

For platform-specific considerations, see the [Memory Barriers](API_REFERENCE.md#memory-barriers) and [Thread Safety Considerations](API_REFERENCE.md#thread-safety-considerations) sections in the API Reference.

## Performance

| Operation | x86 (ns) | ARM Cortex-M (ns) | RISC-V (ns) |
|-----------|----------|-------------------|-------------|
| Insert    | 15-25    | 40-60             | 50-80       |
| Remove    | 12-22    | 35-55             | 45-75       |
| Bulk (16) | 80-120   | 200-300           | 250-400     |

*Measured on 128-byte buffer, 3.2GHz x86, 72MHz ARM Cortex-M4, 100MHz RISC-V*

For performance optimization tips, see the [Performance Considerations](API_REFERENCE.md#performance-considerations) section in the API Reference.

## :snowman: Author

Eray Öztürk ([@diffstorm](https://github.com/diffstorm))

## License

MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions welcome! Fork the repository, create a feature branch, and open a pull request.

## Support

For questions or issues, please [open an issue](https://github.com/diffstorm/cb/issues).
