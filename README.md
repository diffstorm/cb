# Circular Buffer (cb) Library [![Awesome](https://awesome.re/badge.svg)](https://github.com/diffstorm/cb)

[![Build Status](https://github.com/diffstorm/cb/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/diffstorm/cb/actions)
[![License](https://img.shields.io/github/license/diffstorm/cb)](https://github.com/diffstorm/cb/blob/main/LICENSE)
[![Language](https://img.shields.io/github/languages/top/diffstorm/cb)](https://github.com/diffstorm/cb)
[![Code Coverage](https://codecov.io/gh/diffstorm/cb/branch/main/graph/badge.svg)](https://codecov.io/gh/diffstorm/cb)
![GitHub Stars](https://img.shields.io/github/stars/diffstorm/cb?style=social)
![Platforms](https://img.shields.io/badge/platforms-Linux%20|%20Windows%20|%20macOS%20|%20Embedded-lightgrey)
![Version](https://img.shields.io/badge/version-1.1-blue)

A portable, lock-free circular buffer implementation designed for embedded systems, real-time applications, and multi-threaded environments. This library provides a robust ring buffer solution with ISR-safe operations, memory barriers for correct concurrency, and comprehensive test coverage.

## Features

- **Lock-free design**: Safe for single-producer, single-consumer scenarios
- **Cross-platform support**: Works on Linux, Windows, macOS, and embedded systems
- **Memory efficient**: No dynamic allocation, static buffer configuration
- **ISR-safe**: Suitable for interrupt service routines
- **Comprehensive barriers**: Built-in memory barriers for correct concurrency
- **Configurable**: Adapts to different CPU architectures and compilers
- **Thoroughly tested**: 90%+ test coverage with GoogleTest
- **MISRA-compatible**: Designed for safety-critical systems
- Bulk operations for efficient data transfers
- Overwrite mode for streaming applications
- Peek functionality for non-destructive reads
- Buffer validation API for integrity checks

## Getting Started

### Prerequisites

- C compiler (GCC, Clang, MSVC, or embedded toolchain)
- CMake (>= 3.10)
- (For tests) GoogleTest

### Building

```bash
# Clone the repository
git clone https://github.com/diffstorm/cb.git
cd cb

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the library, demos, and tests
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
```

### Tests

```bash
# Run basic tests (9 tests)
./tests

# Run comprehensive tests (16 tests)
./tests2
```

## API Reference

### Initialization
```c
void cb_init(cb * const cb_ptr, CbItem bufferStorage[], CbIndex bufferLength);
```
Initialize circular buffer with static storage

### Core Operations
```c
bool cb_insert(cb * const cb_ptr, CbItem const item);
```
Insert item into buffer (returns false if full)

```c
bool cb_remove(cb * const cb_ptr, CbItem *itemOut);
```
Remove item from buffer (returns false if empty)

### Advanced Operations
```c
size_t cb_insert_bulk(cb *cb_ptr, const CbItem *items, size_t count);
```
Insert multiple items (returns number inserted)

```c
size_t cb_remove_bulk(cb *cb_ptr, CbItem *items, size_t count);
```
Remove multiple items (returns number removed)

```c
bool cb_peek(const cb *cb_ptr, size_t offset, CbItem *itemOut);
```
Read item without removal (offset 0 = oldest item)

### State Information
```c
CbIndex cb_freeSpace(cb * const cb_ptr);
```
Get number of free slots in buffer

```c
CbIndex cb_dataSize(cb * const cb_ptr);
```
Get number of occupied slots in buffer

```c
bool cb_sanity_check(const cb *cb_ptr);
```
Validate buffer integrity (returns true if valid)

### Overwrite Control
```c
void cb_set_overwrite(cb *cb_ptr, bool enable);
```
Enable/disable overwrite mode

```c
bool cb_get_overwrite(const cb *cb_ptr);
```
Get current overwrite mode status

## Configuration Options

### Memory Barriers
Control memory barrier usage via `CB_USE_MEMORY_BARRIERS`:
```c
#define CB_USE_MEMORY_BARRIERS 1  // Enabled by default
```

| Scenario | Recommendation |
|----------|----------------|
| Multi-core systems | ✅ Enable |
| ISR + thread access | ✅ Enable |
| Single-core, no interrupts | ⚠️ Disable with caution |
| x86 single-threaded | ⚠️ Disable with caution |

### Overwrite Mode
Runtime-configurable via API:
```c
cb_set_overwrite(&my_buffer, true);  // Enable overwrite
```

| Mode | Behavior | Use Case |
|------|----------|----------|
| Disabled | Reject new data when full | Data integrity critical |
| Enabled | Overwrite oldest data | Streaming applications |

## Platform Support
Automatically detects and configures for:
- Linux (GCC, Clang)
- Windows (MSVC)
- Embedded (ARM Cortex, AVR, MSP430, RISC-V)
- RTOS (FreeRTOS, Zephyr, VxWorks)
- Compilers (IAR, Keil, GCC, Clang, MSVC)

## Performance Metrics

| Operation | x86 (ns) | ARM Cortex-M (ns) | RISC-V (ns) |
|-----------|----------|-------------------|-------------|
| Insert    | 15-25    | 40-60             | 50-80       |
| Remove    | 12-22    | 35-55             | 45-75       |
| Bulk (16 items) | 80-120   | 200-300           | 250-400     |
| Peek      | 8-15     | 20-35             | 25-40       |
| Overwrite toggle | 5-10 | 10-20 | 15-25 |

*Measured on 128-byte buffer, 3.2GHz x86, 72MHz ARM Cortex-M4, 100MHz RISC-V*

## Test Coverage

The library includes comprehensive tests covering:

- Basic functionality (100% coverage)
- Edge cases (buffer size 1, 2, zero-size buffers)
- Multi-threaded producer-consumer patterns
- Memory barrier validation
- Cross-thread index integrity
- Bulk operation edge cases
- Overwrite mode functionality
- Peek functionality validation
- Sanity check failure modes
- Null pointer handling

### Test Structure

The test suite is organized into two separate files:

- **Basic Tests** (`test.cpp`): 9 tests covering core functionality
- **Comprehensive Tests** (`tests2.cpp`): 16 tests across 7 test suites

```text
=== BASIC TESTS ===
[==========] Running 9 tests from 1 test suite.
[  PASSED  ] 9 tests.

=== COMPREHENSIVE TESTS ===
[==========] Running 16 tests from 7 test suites.
[  PASSED  ] 16 tests.

Total: 25 tests passed
```

## Design Principles

1. **Lock-free operations**: Atomic index updates for minimal latency
2. **Portability**: Adapts to different CPU architectures and compilers
3. **Memory safety**: No dynamic allocation, buffer overflow protection
4. **Concurrency correctness**: Memory barriers for proper ordering
5. **MISRA compatibility**: Designed for safety-critical applications
6. **Runtime safety**: Built-in integrity checks

## Memory Barriers

The library uses compiler-specific memory barriers to ensure correct memory ordering in concurrent environments. These barriers:

- Prevent instruction reordering
- Ensure memory visibility across cores/threads
- Guarantee consistent buffer state

```c
// Example barrier implementation
#define CB_MEMORY_BARRIER() __asm__ __volatile__("" ::: "memory")
```

## Usage Examples

### Basic Usage
```c
#include "cb.h"

#define BUFFER_SIZE 128

CbItem storage[BUFFER_SIZE];
cb my_buffer;

cb_init(&my_buffer, storage, BUFFER_SIZE);

// Producer
cb_insert(&my_buffer, 0x55);

// Consumer
CbItem data;
if (cb_remove(&my_buffer, &data)) {
    // Process data
}
```

### Bulk Operations
```c
// Producer (bulk insert)
CbItem sensor_data[16];
size_t inserted = cb_insert_bulk(&buffer, sensor_data, sizeof(sensor_data));

// Consumer (bulk remove)
CbItem processed[16];
size_t removed = cb_remove_bulk(&buffer, processed, sizeof(processed));
```

### Overwrite Mode
```c
// Enable overwrite for streaming application
cb_set_overwrite(&stream_buffer, true);

// Producer will never block
while (true) {
    cb_insert(&stream_buffer, get_sensor_sample());
}
```

### Peek and Validation
```c
// Check buffer integrity before access
if (!cb_sanity_check(&buffer)) {
    // Handle corruption
}

// Peek at next item without removing
CbItem next_value;
if (cb_peek(&buffer, 0, &next_value)) {
    // Preview data
}
```

## :snowman: Author

Eray Öztürk ([@diffstorm](https://github.com/diffstorm))

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/your-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin feature/your-feature`)
5. Open a pull request

## Support

For questions or issues, please [open an issue](https://github.com/diffstorm/cb/issues).
