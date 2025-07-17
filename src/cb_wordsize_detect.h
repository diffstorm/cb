#ifndef CB_WORDSIZE_DETECT_H
#define CB_WORDSIZE_DETECT_H

/**
 * @file cb_wordsize_detect.h
 * @brief CPU word size detection for circular buffer implementations
 * 
 * @note This header determines the native word size of the processor to optimize
 *       buffer index calculations. The detected type is used for efficient modulo
 *       operations in circular buffers.
 * 
 * Detection priority:
 * 1. Standard C/C++ methods (UINTPTR_MAX)
 * 2. Architecture-specific detection
 * 3. Compiler-specific detection
 * 4. RTOS/platform-specific detection
 * 5. Microcontroller-specific fallbacks
 */

#include <stdint.h>

#ifndef CB_WORD_INDEX_TYPE

/* ====================== STANDARD C/C++ DETECTION ====================== */
#if defined(UINTPTR_MAX)
    /* Standard-compliant compilers */
    #if UINTPTR_MAX == 0xFFFF
        #define CB_WORD_INDEX_TYPE uint16_t  /* 16-bit systems */
    #elif UINTPTR_MAX == 0xFFFFFFFF
        #define CB_WORD_INDEX_TYPE uint32_t  /* 32-bit systems */
    #elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
        #define CB_WORD_INDEX_TYPE uint64_t  /* 64-bit systems */
    #else
        #error "Unsupported CPU word size (UINTPTR_MAX)"
    #endif

/* ================= ARCHITECTURE-SPECIFIC DETECTION =================== */
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__LP64__)
    #define CB_WORD_INDEX_TYPE uint64_t  /* ARM64, 64-bit Linux/BSD */

#elif defined(__arm__) || defined(_M_ARM) || defined(__ARM_ARCH_7M__) || \
      defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
    #define CB_WORD_INDEX_TYPE uint32_t  /* ARM Cortex-M/R/A */

#elif defined(__riscv)
    #if __riscv_xlen == 64
        #define CB_WORD_INDEX_TYPE uint64_t
    #elif __riscv_xlen == 32
        #define CB_WORD_INDEX_TYPE uint32_t
    #else
        #error "Unsupported RISC-V XLEN"
    #endif

#elif defined(__xtensa__) || defined(ESP_PLATFORM)
    #define CB_WORD_INDEX_TYPE uint32_t  /* Xtensa (ESP32) */

#elif defined(__mips__)
    #if defined(_ABI64) || defined(__mips64)
        #define CB_WORD_INDEX_TYPE uint64_t
    #else
        #define CB_WORD_INDEX_TYPE uint32_t
    #endif

#elif defined(__PPC64__) || defined(_ARCH_PPC64)
    #define CB_WORD_INDEX_TYPE uint64_t  /* PowerPC 64-bit */

#elif defined(__powerpc__) || defined(_ARCH_PPC)
    #define CB_WORD_INDEX_TYPE uint32_t  /* PowerPC 32-bit */

#elif defined(__i386__) || defined(_M_IX86)
    #define CB_WORD_INDEX_TYPE uint32_t  /* x86 32-bit */

#elif defined(__x86_64__) || defined(_M_X64)
    #define CB_WORD_INDEX_TYPE uint64_t  /* x86-64 */

/* ================= COMPILER-SPECIFIC DETECTION ======================= */
#elif defined(__IAR_SYSTEMS_ICC__)
    /* IAR Compiler */
    #if __INT_WIDTH__ == 8
        #define CB_WORD_INDEX_TYPE uint8_t
    #elif __INT_WIDTH__ == 16
        #define CB_WORD_INDEX_TYPE uint16_t
    #elif __INT_WIDTH__ == 32
        #define CB_WORD_INDEX_TYPE uint32_t
    #elif __INT_WIDTH__ == 64
        #define CB_WORD_INDEX_TYPE uint64_t
    #else
        #error "Unsupported CPU word size (IAR)"
    #endif

#elif defined(__GNUC__) || defined(__clang__)
    /* GCC/Clang compiler builtins */
    #if defined(__SIZEOF_POINTER__)
        #if __SIZEOF_POINTER__ == 8
            #define CB_WORD_INDEX_TYPE uint64_t
        #elif __SIZEOF_POINTER__ == 4
            #define CB_WORD_INDEX_TYPE uint32_t
        #elif __SIZEOF_POINTER__ == 2
            #define CB_WORD_INDEX_TYPE uint16_t
        #endif
    #elif defined(__UINT8_TYPE__)
        #define CB_WORD_INDEX_TYPE __UINT8_TYPE__
    #elif defined(__UINT16_TYPE__)
        #define CB_WORD_INDEX_TYPE __UINT16_TYPE__
    #elif defined(__UINT32_TYPE__)
        #define CB_WORD_INDEX_TYPE __UINT32_TYPE__
    #elif defined(__UINT64_TYPE__)
        #define CB_WORD_INDEX_TYPE __UINT64_TYPE__
    #endif

#elif defined(_MSC_VER)
    /* Microsoft Visual C++ */
    #if defined(_WIN64)
        #define CB_WORD_INDEX_TYPE uint64_t
    #else
        #define CB_WORD_INDEX_TYPE uint32_t
    #endif

/* ================= RTOS/PLATFORM-SPECIFIC DETECTION ================== */
#elif defined(ESP_PLATFORM)
    #define CB_WORD_INDEX_TYPE uint32_t  /* ESP-IDF */

#elif defined(__ZEPHYR__)
    #define CB_WORD_INDEX_TYPE uint32_t  /* Zephyr RTOS */

#elif defined(FREERTOS)
    #if configUSE_64_BIT_INDEXES == 1
        #define CB_WORD_INDEX_TYPE uint64_t
    #else
        #define CB_WORD_INDEX_TYPE uint32_t
    #endif

/* ============= MICROCONTROLLER-SPECIFIC FALLBACKS ==================== */
#elif defined(__XC8) || defined(__PICC__) || defined(__C51__) || defined(__CX51__)
    #define CB_WORD_INDEX_TYPE uint8_t   /* 8-bit MCUs (PIC, 8051) */

#elif defined(__XC16) || defined(__PIC24__) || defined(__dsPIC30) || defined(__dsPIC33)
    #define CB_WORD_INDEX_TYPE uint16_t  /* 16-bit PIC */

#elif defined(__XC32) || defined(__PIC32MX__) || defined(__PIC32MZ__)
    #define CB_WORD_INDEX_TYPE uint32_t  /* 32-bit PIC */

#elif defined(__RL78__)
    #define CB_WORD_INDEX_TYPE uint16_t  /* Renesas RL78 */

#elif defined(__RX__)
    #define CB_WORD_INDEX_TYPE uint32_t  /* Renesas RX */

#elif defined(__TMS320C2000__)
    #define CB_WORD_INDEX_TYPE uint16_t  /* TI C2000 DSPs */

#elif defined(__TMS320C28X__)
    #define CB_WORD_INDEX_TYPE uint16_t  /* TI C28x DSPs */

#elif defined(__MSP430__)
    #define CB_WORD_INDEX_TYPE uint16_t  /* MSP430 */

#elif defined(__AVR__)
    #if __AVR_ARCH__ >= 5
        #define CB_WORD_INDEX_TYPE uint16_t
    #else
        #define CB_WORD_INDEX_TYPE uint8_t
    #endif

#elif defined(__ARM_SAMV71Q21__) || defined(__ARM_SAME70Q21__)
    #define CB_WORD_INDEX_TYPE uint32_t  /* Atmel SAME70/SAMV71 */

/* ====================== GENERIC FALLBACK ============================= */
#else
    #warning "Falling back to default word size (32-bit)"
    #define CB_WORD_INDEX_TYPE uint32_t

#endif /* Detection methods */

#endif /* CB_WORD_INDEX_TYPE */

/* Sanity check */
#ifndef CB_WORD_INDEX_TYPE
    #error "Failed to detect CPU word size"
#endif

#endif /* CB_WORDSIZE_DETECT_H */