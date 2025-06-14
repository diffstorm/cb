# Circular Buffer (cb) Library

![License](https://img.shields.io/badge/license-MIT-blue)
![Platforms](https://img.shields.io/badge/platforms-Linux%20|%20Windows%20|%20macOS%20|%20Embedded-lightgrey)

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

# Run tests
./tests
```

### Demos

```bash
# Run lock-free demo
./demo

# Run mutex-protected demo
./demo_mutex
```

## API Reference

### Initialization
```c
void cb_init(cb * const cb_ptr, CbItem bufferStorage[], CbIndex bufferLength);
```
Initialize circular buffer with static storage

### Operations
```c
bool cb_insert(cb * const cb_ptr, CbItem const item);
```
Insert item into buffer (returns false if full)

```c
bool cb_remove(cb * const cb_ptr, CbItem *itemOut);
```
Remove item from buffer (returns false if empty)

### State Information
```c
CbIndex cb_freeSpace(cb * const cb_ptr);
```
Get number of free slots in buffer

```c
CbIndex cb_dataSize(cb * const cb_ptr);
```
Get number of occupied slots in buffer

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

### Platform Support
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
| Size check| 5-10     | 10-20             | 15-25       |

*Measured on 128-byte buffer, 3.2GHz x86, 72MHz ARM Cortex-M4, 100MHz RISC-V*

## Test Coverage

The library includes comprehensive tests covering:

- Basic functionality (100% coverage)
- Edge cases (buffer size 1, 2, large buffers)
- Multi-threaded stress tests
- ISR simulation tests
- Memory barrier validation
- Cross-thread index integrity

```text
[==========] 24 tests from 6 test suites ran. (18543 ms total)
[  PASSED  ] 24 tests.
```

## Design Principles

1. **Lock-free operations**: Atomic index updates for minimal latency
2. **Portability**: Adapts to different CPU architectures and compilers
3. **Memory safety**: No dynamic allocation, buffer overflow protection
4. **Concurrency correctness**: Memory barriers for proper ordering
5. **MISRA compatibility**: Designed for safety-critical applications

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

### Multi-threaded Producer-Consumer
```c
// Producer thread
void* producer(void* arg) {
    for (int i = 0; i < 1000; i++) {
        while (!cb_insert(&buffer, i % 256)) {
            // Wait or yield
        }
    }
    return NULL;
}

// Consumer thread
void* consumer(void* arg) {
    CbItem item;
    for (int i = 0; i < 1000; i++) {
        while (!cb_remove(&buffer, &item)) {
            // Wait or yield
        }
        // Process item
    }
    return NULL;
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

