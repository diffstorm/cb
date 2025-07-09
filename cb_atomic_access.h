#ifndef CB_ATOMIC_ACCESS_H
#define CB_ATOMIC_ACCESS_H

/**
 * @file cb_atomic_access.h
 * @brief Atomic load/store implementations for circular buffer indices
 * 
 * @note Provides architecture-specific implementations for atomic memory operations
 *       with relaxed memory ordering (no synchronization, just atomicity guarantee)
 * 
 * Memory Ordering:
 *   Uses relaxed memory ordering for maximum performance since:
 *     - Circular buffer indices are modified by only one thread each (producer/consumer)
 *     - Memory barriers are handled separately in cb_memorybarrier_detect.h
 * 
 * Fallback Strategy:
 *   1. C11 standard atomics
 *   2. Compiler intrinsics (GCC, Clang, MSVC, IAR, etc)
 *   3. RTOS-specific atomics
 *   4. Architecture-specific inline assembly (GNU-compatible only)
 *   5. Volatile access (last resort)
 */

/* ======================== C11 Standard Atomics ======================== */
#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
    #include <stdatomic.h>
    /* Use appropriate atomic type based on CB_ATOMIC_INDEX_TYPE */
    #if defined(CB_ATOMIC_INDEX_TYPE) && CB_ATOMIC_INDEX_TYPE == uint32_t
        #define CB_ATOMIC_LOAD(ptr) atomic_load_explicit((_Atomic(uint32_t)*)(ptr), memory_order_relaxed)
        #define CB_ATOMIC_STORE(ptr, val) atomic_store_explicit((_Atomic(uint32_t)*)(ptr), (uint32_t)(val), memory_order_relaxed)
    #elif defined(CB_ATOMIC_INDEX_TYPE) && CB_ATOMIC_INDEX_TYPE == uint16_t
        #define CB_ATOMIC_LOAD(ptr) atomic_load_explicit((_Atomic(uint16_t)*)(ptr), memory_order_relaxed)
        #define CB_ATOMIC_STORE(ptr, val) atomic_store_explicit((_Atomic(uint16_t)*)(ptr), (uint16_t)(val), memory_order_relaxed)
    #elif defined(CB_ATOMIC_INDEX_TYPE) && CB_ATOMIC_INDEX_TYPE == uint64_t
        #define CB_ATOMIC_LOAD(ptr) atomic_load_explicit((_Atomic(uint64_t)*)(ptr), memory_order_relaxed)
        #define CB_ATOMIC_STORE(ptr, val) atomic_store_explicit((_Atomic(uint64_t)*)(ptr), (uint64_t)(val), memory_order_relaxed)
    #else
        /* Fallback to uintptr_t for compatibility */
        #define CB_ATOMIC_LOAD(ptr) atomic_load_explicit((_Atomic(uintptr_t)*)(ptr), memory_order_relaxed)
        #define CB_ATOMIC_STORE(ptr, val) atomic_store_explicit((_Atomic(uintptr_t)*)(ptr), (uintptr_t)(val), memory_order_relaxed)
    #endif

/* ====================== GCC/Clang Intrinsics ========================= */
#elif defined(__GNUC__) || defined(__clang__)
    #define CB_ATOMIC_LOAD(ptr) __atomic_load_n((ptr), __ATOMIC_RELAXED)
    #define CB_ATOMIC_STORE(ptr, val) __atomic_store_n((ptr), (val), __ATOMIC_RELAXED)

/* ========================== MSVC Intrinsics =========================== */
#elif defined(_MSC_VER)
    #include <intrin.h>
    #if defined(_M_IX86) || defined(_M_X64)
        #define CB_ATOMIC_LOAD(ptr) (*(ptr))
        #define CB_ATOMIC_STORE(ptr, val) (*(ptr) = (val))
    #elif defined(_M_ARM) || defined(_M_ARM64)
        #define CB_ATOMIC_LOAD(ptr) __iso_volatile_load32((volatile long*)(ptr))
        #define CB_ATOMIC_STORE(ptr, val) __iso_volatile_store32((volatile long*)(ptr), (val))
    #else
        #define CB_ATOMIC_LOAD(ptr) (*(ptr))
        #define CB_ATOMIC_STORE(ptr, val) (*(ptr) = (val))
    #endif

/* ========================= IAR Compiler ============================== */
#elif defined(__IAR_SYSTEMS_ICC__)
    #include <intrinsics.h>
    #if defined(__ATOMIC_SUPPORT__)
        #define CB_ATOMIC_LOAD(ptr) __iar_Atom_load_n((ptr), __ATOMIC_RELAXED)
        #define CB_ATOMIC_STORE(ptr, val) __iar_Atom_store_n((ptr), (val), __ATOMIC_RELAXED)
    #else
        /* Fallback to basic volatile */
        #define CB_ATOMIC_LOAD(ptr) (*(volatile uintptr_t*)(ptr))
        #define CB_ATOMIC_STORE(ptr, val) (*(volatile uintptr_t*)(ptr) = (val))
    #endif

/* ====================== RTOS-Specific Atomics ======================== */
#elif defined(CONFIG_ZEPHYR) || defined(__ZEPHYR__)
    #include <sys/atomic.h>
    #define CB_ATOMIC_LOAD(ptr) atomic_get((atomic_t*)(ptr))
    #define CB_ATOMIC_STORE(ptr, val) atomic_set((atomic_t*)(ptr), (val))

#elif defined(__VXWORKS__)
    #include <vxAtomicLib.h>
    #define CB_ATOMIC_LOAD(ptr) vxAtomicRead((vx_atomic_t*)(ptr))
    #define CB_ATOMIC_STORE(ptr, val) vxAtomicSet((vx_atomic_t*)(ptr), (val))

#elif defined(OS_EMBOS_H) || defined(USE_EMBOS)  /* SEGGER embOS */
    #include <RTOS.h>
    #define CB_ATOMIC_LOAD(ptr) OS_GetAtomic((OS_ATOMIC*)(ptr))
    #define CB_ATOMIC_STORE(ptr, val) OS_SetAtomic((OS_ATOMIC*)(ptr), (val))

/* ================== Architecture-Specific Assembly =================== */
/* Only for GNU-compatible compilers that support statement expressions */
#elif (defined(__GNUC__) || defined(__clang__)) && !defined(__cplusplus)
    #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || \
        defined(__ARM_ARCH_8M_MAIN__) || defined(__aarch64__)
        /* ARM: LDA/STL instructions provide atomicity */
        #define CB_ATOMIC_LOAD(ptr) ({ \
            uintptr_t val; \
            __asm__ volatile("lda %0, [%1]" : "=r"(val) : "r"(ptr) : "memory"); \
            val; \
        })
        #define CB_ATOMIC_STORE(ptr, val) \
            __asm__ volatile("stl %0, [%1]" :: "r"(val), "r"(ptr) : "memory")

    #elif defined(__riscv)
        /* RISC-V: Atomic extension (A) instructions */
        #if __riscv_xlen == 32
            #define CB_ATOMIC_LOAD(ptr) ({ \
                uint32_t val; \
                __asm__ volatile("amoadd.w zero, zero, %0" : "=r"(val) : "r"(ptr) : "memory"); \
                val; \
            })
            #define CB_ATOMIC_STORE(ptr, val) \
                __asm__ volatile("amoswap.w zero, %0, %1" :: "r"(val), "r"(ptr) : "memory")
        #else
            #define CB_ATOMIC_LOAD(ptr) ({ \
                uint64_t val; \
                __asm__ volatile("amoadd.d zero, zero, %0" : "=r"(val) : "r"(ptr) : "memory"); \
                val; \
            })
            #define CB_ATOMIC_STORE(ptr, val) \
                __asm__ volatile("amoswap.d zero, %0, %1" :: "r"(val), "r"(ptr) : "memory")
        #endif

    #elif defined(__xtensa__) || defined(ESP_PLATFORM)
        /* Xtensa (ESP32): MEMW instruction ensures completion */
        #define CB_ATOMIC_LOAD(ptr) ({ \
            uint32_t val; \
            __asm__ volatile("memw\n l32i %0, %1, 0" : "=r"(val) : "r"(ptr) : "memory"); \
            val; \
        })
        #define CB_ATOMIC_STORE(ptr, val) \
            __asm__ volatile("memw\n s32i %0, %1, 0" :: "r"(val), "r"(ptr) : "memory")
    #endif
#endif

/* ====================== Fallback Implementations ====================== */
#if !defined(CB_ATOMIC_LOAD) || !defined(CB_ATOMIC_STORE)
    /* Generic fallback - use volatile with explicit type */
    #define CB_ATOMIC_LOAD(ptr) (*(volatile uintptr_t*)(ptr))
    #define CB_ATOMIC_STORE(ptr, val) (*(volatile uintptr_t*)(ptr) = (val))
    
    #if !defined(__cplusplus)
        #warning "Using volatile fallback for atomic operations - verify platform atomicity"
    #endif
#endif

#endif /* CB_ATOMIC_ACCESS_H */