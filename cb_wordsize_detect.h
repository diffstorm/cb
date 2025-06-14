#ifndef CB_WORDSIZE_DETECT_H
#define CB_WORDSIZE_DETECT_H

#include <stdint.h>

#ifndef CB_WORD_INDEX_TYPE

    #if defined(UINTPTR_MAX)

        #if UINTPTR_MAX == 0xFFFF
            #define CB_WORD_INDEX_TYPE uint16_t  /* 16-bit CPU */
        #elif UINTPTR_MAX == 0xFFFFFFFF
            #define CB_WORD_INDEX_TYPE uint32_t  /* 32-bit CPU */
        #elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
            #define CB_WORD_INDEX_TYPE uint64_t  /* 64-bit CPU */
        #else
            #error "Unsupported CPU word size"
        #endif

    #else  /* Fallback if UINTPTR_MAX is not available */

        #if defined(__UINT8_TYPE__)
            #define CB_WORD_INDEX_TYPE __UINT8_TYPE__
        #elif defined(__UINT16_TYPE__)
            #define CB_WORD_INDEX_TYPE __UINT16_TYPE__
        #elif defined(__UINT32_TYPE__)
            #define CB_WORD_INDEX_TYPE __UINT32_TYPE__
        #elif defined(__UINT64_TYPE__)
            #define CB_WORD_INDEX_TYPE __UINT64_TYPE__
        #elif defined(__XC8)
            #define CB_WORD_INDEX_TYPE uint8_t
        #elif defined(__XC16)
            #define CB_WORD_INDEX_TYPE uint16_t
        #elif defined(__XC32)
            #define CB_WORD_INDEX_TYPE uint32_t
        #elif defined(__PICC__)
            #define CB_WORD_INDEX_TYPE uint8_t
        #elif defined(__PIC24__)
            #define CB_WORD_INDEX_TYPE uint16_t
        #elif defined(__PIC32MX__)
            #define CB_WORD_INDEX_TYPE uint32_t
        #elif defined(__C51__) || defined(__CX51__)
            #define CB_WORD_INDEX_TYPE uint8_t
        #elif defined(__IAR_SYSTEMS_ICC__)
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
        #else
            #error "Unsupported compiler or CPU architecture"
        #endif

    #endif

#endif /* CB_WORD_INDEX_TYPE */

#endif /* CB_WORDSIZE_DETECT_H */
