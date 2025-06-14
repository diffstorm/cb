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

/**
    @brief Memory barrier control for lock-free operation

    @note Memory barriers are CRITICAL for correct lock-free behavior in concurrent environments.
        They enforce memory ordering and visibility between cores/threads. Disabling them should
        only be done with extreme caution and thorough validation.

    ===== WHEN TO ENABLE BARRIERS (RECOMMENDED DEFAULT) =====
    - Multi-core systems
    - Asymmetric multiprocessing (AMP) systems
    - Any system with both ISRs and threads accessing the buffer
    - Systems with weak memory ordering (ARM, PowerPC, RISC-V)
    - Preemptive RTOS environments (FreeRTOS, Zephyr, ThreadX)
    - When using multiple producers/consumers
    - When buffer is shared between cores with non-coherent caches

    ===== WHEN DISABLING MAY BE SAFE =====
    - Single-core bare-metal systems with no interrupts
    - Cooperative RTOS with explicit yield points
    - x86/x64 architectures in single-threaded scenarios
    - When using external synchronization (mutexes) already

    ===== VERIFICATION REQUIREMENTS FOR DISABLING =====
    1. Test on target hardware (not just simulator)
    2. Run long-duration stress tests (>1 million ops)
    3. Verify at highest optimization levels (-O3)
    4. Check with thread sanitizers (TSAN)
    5. Validate on all supported compiler versions
    6. Confirm no buffer corruption under power fluctuations

    ===== PERFORMANCE CONSIDERATIONS =====
    Architecture      | Barrier Cost | Safe to Disable?
    ------------------------------------------------
    ARM Cortex-M      | 1-2 cycles   | Only in single-thread
    x86/x64           | ~1 cycle     | Often safe in ST
    RISC-V            | 2-5 cycles   | Rarely safe to disable
    MIPS              | 3-7 cycles   | Not recommended

    ===== RISKS OF DISABLING =====
    !! Silent data corruption
    !! Heisenbugs appearing only in production
    !! Inconsistent behavior across compiler versions
    !! Broken ordering guarantees
    !! Security vulnerabilities from race conditions

    ===== DEFAULT RECOMMENDATION =====
    Keep barriers ENABLED unless:
      - You're on a resource-constrained embedded system
      - You've proven through hardware testing they're unnecessary
      - You accept full responsibility for potential races

    ===== Validation Checklist =====
    | Test Case                  | Pass | Fail |
    |----------------------------|------|------|
    | 1. Single-core stress test | [ ]  | [ ]  |
    | 2. Dual-core ping-pong     | [ ]  | [ ]  |
    | 3. ISR producer test       | [ ]  | [ ]  |
    | 4. Power cycle test        | [ ]  | [ ]  |
    | 5. -O3 optimization test   | [ ]  | [ ]  |
    | 6. TSAN clean report       | [ ]  | [ ]  |
    | 7. 72h burn-in             | [ ]  | [ ]  |
    Must pass ALL tests to consider disabling barriers
*/
#ifndef CB_USE_MEMORY_BARRIERS
#define CB_USE_MEMORY_BARRIERS 1  // Enable by default
#endif

#if CB_USE_MEMORY_BARRIERS
#ifndef CB_MEMORY_BARRIER
#if defined(__GNUC__) || defined(__clang__)
/* GCC, Clang, and compatible compilers (Linux, *BSD, macOS) */
#define CB_MEMORY_BARRIER() __asm__ __volatile__("" ::: "memory")

#elif defined(_MSC_VER)
/* Microsoft Visual C++ */
#include <intrin.h>
#define CB_MEMORY_BARRIER() _ReadWriteBarrier()

#elif defined(__ICCARM__) || defined(__IAR_SYSTEMS_ICC__)
/* IAR Embedded Workbench for ARM */
#include <intrinsics.h>
#define CB_MEMORY_BARRIER() __memory_barrier()

#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
/* Keil MDK-ARM */
#if __ARMCC_VERSION >= 6010050
/* ARM Compiler 6 (armclang) */
#define CB_MEMORY_BARRIER() __asm volatile("dmb ish" ::: "memory")
#else
/* ARM Compiler 5 */
#define CB_MEMORY_BARRIER() __schedule_barrier()
#endif

#elif defined(__TI_COMPILER_VERSION__)
/* Texas Instruments Code Composer Studio */
#define CB_MEMORY_BARRIER() asm("")

#elif defined(__ghs__)
/* Green Hills Compiler */
#define CB_MEMORY_BARRIER() __memory_changed()

#elif defined(__TASKING__)
/* TASKING VX-toolset */
#define CB_MEMORY_BARRIER() __memory_barrier()

#elif defined(FREERTOS_H) || defined(INC_FREERTOS_H)
/* FreeRTOS-specific barrier */
#ifdef portMEMORY_BARRIER
#define CB_MEMORY_BARRIER() portMEMORY_BARRIER()
#else
#define CB_MEMORY_BARRIER() vPortMemoryBarrier()
#endif

#elif defined(CONFIG_ZEPHYR)
/* Zephyr RTOS */
#include <sys/atomic.h>
#define CB_MEMORY_BARRIER() atomic_thread_fence(memory_order_seq_cst)

#elif defined(__VXWORKS__)
/* VxWorks RTOS */
#include <vxAtomicLib.h>
#define CB_MEMORY_BARRIER() vxAtomicMemBarrier()

#elif defined(__XC32) || defined(__XC16) || defined(__XC8)
/* Microchip XC Compilers */
#define CB_MEMORY_BARRIER() __builtin_memory_barrier()

#elif defined(__GNUC__) && (__riscv)
/* RISC-V GCC */
#define CB_MEMORY_BARRIER() __asm__ volatile("fence" ::: "memory")

#elif defined(__llvm__) && (__MIPS__)
/* MIPS LLVM */
#define CB_MEMORY_BARRIER() __sync_synchronize()

#else
/* Generic fallback - try to use C11 atomics */
#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
#define CB_MEMORY_BARRIER() atomic_thread_fence(memory_order_seq_cst)
#else
/* Last resort - empty implementation with warning */
#warning "No memory barrier implementation for this platform. Using empty barrier."
#define CB_MEMORY_BARRIER() ((void)0)
#endif
#endif
#endif
#endif

// Atomic operations methods
#if defined(__GNUC__) || defined(__clang__)
#define CB_ATOMIC_LOAD(ptr) __atomic_load_n((ptr), __ATOMIC_RELAXED)
#define CB_ATOMIC_STORE(ptr, val) __atomic_store_n((ptr), (val), __ATOMIC_RELAXED)
#elif defined(_MSC_VER)
#define CB_ATOMIC_LOAD(ptr) (*(ptr))
#define CB_ATOMIC_STORE(ptr, val) (*(ptr) = (val))
#else
#define CB_ATOMIC_LOAD(ptr) (*(ptr))
#define CB_ATOMIC_STORE(ptr, val) (*(ptr) = (val))
#endif

// Platform word size detection
#if defined(UINTPTR_MAX)

#if UINTPTR_MAX == 0xFFFF
typedef uint16_t CbIndex;  // 16-bit CPU
#elif UINTPTR_MAX == 0xFFFFFFFF
typedef uint32_t CbIndex;  // 32-bit CPU
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
typedef uint64_t CbIndex;  // 64-bit CPU
#else
#error "Unsupported CPU word size"
#endif

#else  // Fallback if UINTPTR_MAX is not available

#if defined(__UINT8_TYPE__)
typedef __UINT8_TYPE__ CbIndex;
#elif defined(__UINT16_TYPE__)
typedef __UINT16_TYPE__ CbIndex;
#elif defined(__UINT32_TYPE__)
typedef __UINT32_TYPE__ CbIndex;
#elif defined(__UINT64_TYPE__)
typedef __UINT64_TYPE__ CbIndex;
#elif defined(__XC8)
typedef uint8_t CbIndex;
#elif defined(__XC16)
typedef uint16_t CbIndex;
#elif defined(__XC32)
typedef uint32_t CbIndex;
#elif defined(__PICC__)
typedef uint8_t CbIndex;
#elif defined(__PIC24__)
typedef uint16_t CbIndex;
#elif defined(__PIC32MX__)
typedef uint32_t CbIndex;
#elif defined(__C51__) || defined(__CX51__)
typedef uint8_t CbIndex;
#elif defined(__IAR_SYSTEMS_ICC__)
#if __INT_WIDTH__ == 8
typedef uint8_t CbIndex;
#elif __INT_WIDTH__ == 16
typedef uint16_t CbIndex;
#elif __INT_WIDTH__ == 32
typedef uint32_t CbIndex;
#elif __INT_WIDTH__ == 64
typedef uint64_t CbIndex;
#else
#error "Unsupported CPU word size"
#endif
#else
#error "Unsupported compiler or CPU architecture"
#endif

#endif

// Platform-specific atomic type for index
#if defined(__linux__)
#include <signal.h>
typedef sig_atomic_t CbAtomicIndex;

#elif defined(_WIN32)
#include <windows.h>
typedef LONG CbAtomicIndex;

#elif defined(__APPLE__)
#include <libkern/OSAtomic.h>
typedef int32_t CbAtomicIndex;

#elif defined(__ARM_ARCH)
/**
    @note On ARM Cortex-M (CMSIS), aligned 32-bit access is atomic.
         For FreeRTOS on ARM, critical sections are typically used.
         If using multi-core or sharing buffer between ISRs and tasks,
         user must ensure mutual exclusion externally.
*/
typedef uint32_t CbAtomicIndex;

#elif defined(__AVR__)
typedef uint8_t CbAtomicIndex;

#elif defined(__XTENSA__)
typedef uint32_t CbAtomicIndex;

#elif defined(__riscv)
typedef uint32_t CbAtomicIndex;

#elif defined(__MSP430__)
typedef uint16_t CbAtomicIndex;

#elif defined(__VXWORKS__)
#include <vxAtomicLib.h>
typedef int32_t CbAtomicIndex;

#elif defined(CONFIG_ZEPHYR)
#include <sys/atomic.h>
typedef atomic_t CbAtomicIndex;

#elif defined(TX_VERSION_ID)
typedef ULONG CbAtomicIndex;

#elif defined(FREERTOS_H) || defined(INC_FREERTOS_H)
/**
    @note Consider using cb_insert and cb_remove under critical section.
*/
typedef uint32_t CbAtomicIndex;

#else
typedef CbIndex CbAtomicIndex;
#endif

typedef uint8_t CbItem;

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
