# ARM GCC Toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Toolchain paths (modify based on your setup)
set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Compiler flags
set(CMAKE_C_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard" CACHE INTERNAL "C Compiler flags")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE INTERNAL "C++ Compiler flags")

# Disable compiler checks
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)