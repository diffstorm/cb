#ifndef CB_MEMORYBARRIER_DETECT_H
#define CB_MEMORYBARRIER_DETECT_H

#ifndef CB_USE_MEMORY_BARRIERS
#define CB_USE_MEMORY_BARRIERS 1  // Enable by default
#endif
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

#endif /* CB_MEMORYBARRIER_DETECT_H */
