#ifndef CB_ATOMICINDEX_DETECT_H
#define CB_ATOMICINDEX_DETECT_H

#include "cb_wordsize_detect.h"  /* Ensure CB_WORD_INDEX_TYPE is defined */

#ifndef CB_ATOMIC_INDEX_TYPE

    #if defined(__linux__)
        #include <signal.h>
        #define CB_ATOMIC_INDEX_TYPE sig_atomic_t

    #elif defined(_WIN32)
        #include <windows.h>
        #define CB_ATOMIC_INDEX_TYPE LONG

    #elif defined(__APPLE__)
        #include <libkern/OSAtomic.h>
        #define CB_ATOMIC_INDEX_TYPE int32_t

    #elif defined(__ARM_ARCH)
        /**
         * @note On ARM Cortex-M (CMSIS), aligned 32-bit access is atomic.
         *       For FreeRTOS on ARM, critical sections are typically used.
         *       If using multi-core or sharing buffer between ISRs and tasks,
         *       user must ensure mutual exclusion externally.
         */
        #define CB_ATOMIC_INDEX_TYPE uint32_t

    #elif defined(__AVR__)
        #define CB_ATOMIC_INDEX_TYPE uint8_t

    #elif defined(__XTENSA__)
        #define CB_ATOMIC_INDEX_TYPE uint32_t

    #elif defined(__riscv)
        #define CB_ATOMIC_INDEX_TYPE uint32_t

    #elif defined(__MSP430__)
        #define CB_ATOMIC_INDEX_TYPE uint16_t

    #elif defined(__VXWORKS__)
        #include <vxAtomicLib.h>
        #define CB_ATOMIC_INDEX_TYPE int32_t

    #elif defined(CONFIG_ZEPHYR)
        #include <sys/atomic.h>
        #define CB_ATOMIC_INDEX_TYPE atomic_t

    #elif defined(TX_VERSION_ID)
        #define CB_ATOMIC_INDEX_TYPE ULONG

    #elif defined(FREERTOS_H) || defined(INC_FREERTOS_H)
        /**
         * @note Consider using cb_insert and cb_remove under critical section.
         */
        #define CB_ATOMIC_INDEX_TYPE uint32_t

    #else
        #define CB_ATOMIC_INDEX_TYPE CB_WORD_INDEX_TYPE
    #endif

#endif /* CB_ATOMIC_INDEX_TYPE */

#endif /* CB_ATOMICINDEX_DETECT_H */
