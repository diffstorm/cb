#ifndef CB_ATOMICINDEX_DETECT_H
#define CB_ATOMICINDEX_DETECT_H

/**
 * @file cb_atomicindex_detect.h
 * @brief Platform-specific atomic type selection for circular buffer indices
 * 
 * @note This header selects the optimal atomic type for circular buffer indices
 *       that meets these requirements:
 *       1. Naturally atomic on the target architecture
 *       2. Same size or smaller than CB_WORD_INDEX_TYPE
 *       3. Optimal for lock-free operation
 * 
 * Detection priority:
 * 1. Standard C11 atomics
 * 2. RTOS-specific atomic types
 * 3. Architecture-specific atomic guarantees
 * 4. Compiler-specific atomic implementations
 * 5. Size-based fallbacks
 */

#ifndef CB_ATOMIC_INDEX_TYPE

/* ======================== STANDARD C11 ATOMICS ======================== */
#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
    #include <stdatomic.h>
    #if defined(ATOMIC_POINTER_LOCK_FREE) && ATOMIC_POINTER_LOCK_FREE == 2
        #define CB_ATOMIC_INDEX_TYPE atomic_uintptr_t
    #else
        #define CB_ATOMIC_INDEX_TYPE atomic_uint_fast32_t
    #endif

/* ========================= RTOS-SPECIFIC TYPES ======================== */
#elif defined(CONFIG_ZEPHYR) || defined(__ZEPHYR__)
    #include <sys/atomic.h>
    #define CB_ATOMIC_INDEX_TYPE atomic_t

#elif defined(FREERTOS_H) || defined(INC_FREERTOS_H)
    #if (configUSE_16_BIT_TICKS == 1)
        #define CB_ATOMIC_INDEX_TYPE uint16_t
    #else
        #define CB_ATOMIC_INDEX_TYPE uint32_t
    #endif

#elif defined(__VXWORKS__)
    #include <vxAtomicLib.h>
    #define CB_ATOMIC_INDEX_TYPE vx_atomic_t

#elif defined(TX_VERSION_ID) || defined(__THREADX__)
    #define CB_ATOMIC_INDEX_TYPE ULONG

#elif defined(OS_EMBOS_H) || defined(USE_EMBOS)  /* SEGGER embOS */
    #include <RTOS.h>
    #define CB_ATOMIC_INDEX_TYPE OS_INT

#elif defined(OS_CFG_H)  /* uC/OS-II/III */
    #define CB_ATOMIC_INDEX_TYPE OS_CPU_SR

/* ================== ARCHITECTURE-SPECIFIC ATOMICS ==================== */
#elif defined(__aarch64__) || defined(__ARM_ARCH_8A__)
    /* ARMv8-A guarantees atomic 32/64-bit aligned accesses */
    #define CB_ATOMIC_INDEX_TYPE uint32_t

#elif defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || \
      defined(__ARM_ARCH_8M_MAIN__) || defined(__CORTEX_M)
    /* Cortex-M: Atomic 32-bit when aligned and not split (Cortex-M0: 16-bit) */
    #if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_8M_BASE__)
        #define CB_ATOMIC_INDEX_TYPE uint16_t
    #else
        #define CB_ATOMIC_INDEX_TYPE uint32_t
    #endif

#elif defined(__riscv)
    /* RISC-V: Atomic 32-bit for RV32, 64-bit for RV64 */
    #if __riscv_xlen == 64
        #define CB_ATOMIC_INDEX_TYPE uint64_t
    #else
        #define CB_ATOMIC_INDEX_TYPE uint32_t
    #endif

#elif defined(__xtensa__) || defined(ESP_PLATFORM)
    /* Xtensa (ESP32): Atomic 32-bit accesses */
    #define CB_ATOMIC_INDEX_TYPE uint32_t

#elif defined(__mips__)
    /* MIPS: Atomic 32-bit for O32, 64-bit for N64 */
    #if defined(_ABI64) || defined(__mips64)
        #define CB_ATOMIC_INDEX_TYPE uint64_t
    #else
        #define CB_ATOMIC_INDEX_TYPE uint32_t
    #endif

#elif defined(__AVR__)
    /* AVR: 8-bit atomic by default, 16-bit for modern AVRs */
    #if __AVR_ARCH__ >= 100 /* Modern AVR (AVRxt) */
        #define CB_ATOMIC_INDEX_TYPE uint16_t
    #else
        #define CB_ATOMIC_INDEX_TYPE uint8_t
    #endif

#elif defined(__MSP430__)
    /* MSP430: 16-bit atomic */
    #define CB_ATOMIC_INDEX_TYPE uint16_t

#elif defined(__TMS320C28X__) || defined(_TMS320C28X)
    /* TI C28x DSPs: 16-bit atomic */
    #define CB_ATOMIC_INDEX_TYPE uint16_t

/* ===================== OS-SPECIFIC IMPLEMENTATIONS ==================== */
#elif defined(__linux__) || defined(__gnu_linux__)
    #include <signal.h>
    #define CB_ATOMIC_INDEX_TYPE sig_atomic_t

#elif defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #if defined(_WIN64)
        #define CB_ATOMIC_INDEX_TYPE LONG64
    #else
        #define CB_ATOMIC_INDEX_TYPE LONG
    #endif

#elif defined(__APPLE__)
    #include <libkern/OSAtomic.h>
    #define CB_ATOMIC_INDEX_TYPE int32_t

#elif defined(__QNX__)
    #include <sys/neutrino.h>
    #define CB_ATOMIC_INDEX_TYPE uintptr_t

/* ===================== COMPILER-SPECIFIC ATOMICS ===================== */
#elif defined(__IAR_SYSTEMS_ICC__)
    /* IAR Compiler */
    #if defined(__ATOMIC_SUPPORT__)
        #define CB_ATOMIC_INDEX_TYPE __iar_atomic_uintptr_t
    #else
        /* Fallback to largest atomic size */
        #if __INT_WIDTH__ >= 32
            #define CB_ATOMIC_INDEX_TYPE uint32_t
        #else
            #define CB_ATOMIC_INDEX_TYPE uint16_t
        #endif
    #endif

#elif defined(__GNUC__) || defined(__clang__)
    /* GCC/Clang: Use __atomic types */
    #if defined(__SIZEOF_POINTER__)
        #if __SIZEOF_POINTER__ == 8
            #define CB_ATOMIC_INDEX_TYPE __atomic_uint_fast64_t
        #else
            #define CB_ATOMIC_INDEX_TYPE __atomic_uint_fast32_t
        #endif
    #else
        /* Conservative fallback */
        #define CB_ATOMIC_INDEX_TYPE unsigned int
    #endif

#elif defined(_MSC_VER)
    /* MSVC: Use volatile as atomic for compatible sizes */
    #if defined(_WIN64)
        #define CB_ATOMIC_INDEX_TYPE volatile uint64_t
    #else
        #define CB_ATOMIC_INDEX_TYPE volatile uint32_t
    #endif

/* ===================== SIZE-BASED FALLBACKS ========================== */
#elif defined(UINTPTR_MAX)
    /* Size-based selection when no other info available */
    #if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
        #define CB_ATOMIC_INDEX_TYPE uint64_t
    #elif UINTPTR_MAX == 0xFFFFFFFF
        #define CB_ATOMIC_INDEX_TYPE uint32_t
    #elif UINTPTR_MAX == 0xFFFF
        #define CB_ATOMIC_INDEX_TYPE uint16_t
    #else
        #define CB_ATOMIC_INDEX_TYPE uint8_t
    #endif

/* ====================== GENERIC FALLBACK ============================= */
#else
    #define CB_ATOMIC_INDEX_TYPE CB_WORD_INDEX_TYPE
    #warning "Using word index type as atomic fallback - verify atomicity"
#endif

#endif /* CB_ATOMIC_INDEX_TYPE */

#endif /* CB_ATOMICINDEX_DETECT_H */