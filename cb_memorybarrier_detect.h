#ifndef CB_MEMORYBARRIER_DETECT_H
#define CB_MEMORYBARRIER_DETECT_H

// Memory barrier implementations

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
		/* Apple Silicon (macOS on ARM64) */
		#if defined(__APPLE__) && defined(__aarch64__)
			#define CB_MEMORY_BARRIER() __asm__ __volatile__("dmb ish" ::: "memory")
		
		/* ARM64 (aarch64) */
		#elif defined(__aarch64__) && (defined(__GNUC__) || defined(__clang__))
			#define CB_MEMORY_BARRIER() __asm__ __volatile__("dmb ish" ::: "memory")
		
		/* ARM Cortex-M (All versions) */
		#elif (defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || \
			   defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)) && \
			  (defined(__GNUC__) || defined(__clang__))
			#define CB_MEMORY_BARRIER() __asm__ __volatile__("dmb" ::: "memory")
		
		/* Infineon AURIX (TriCore) */
		#elif defined(__HIGHTEC__) && (defined(__tricore__) || defined(__TC__))
			#define CB_MEMORY_BARRIER() __asm__ volatile("dsync")
		
		/* Altera/Intel Nios II */
		#elif defined(__NIOS2__) || defined(__NIOS2)
			#if defined(__GNUC__) || defined(__clang__)
				#define CB_MEMORY_BARRIER() __asm__ __volatile__("sync" ::: "memory")
			#endif
		
		/* Xtensa (ESP32) */
		#elif defined(__xtensa__) || defined(ESP_PLATFORM)
			#define CB_MEMORY_BARRIER() __asm__ __volatile__("memw" ::: "memory")
		
		/* Andes NDS32 */
		#elif defined(__nds32__)
			#define CB_MEMORY_BARRIER() __asm__ __volatile__("membar" ::: "memory")
		
		/* RISC-V */
		#elif defined(__riscv)
			#if (__riscv_xlen == 32)
				#define CB_MEMORY_BARRIER() __asm__ volatile("fence iorw, iorw" ::: "memory")
			#else
				#define CB_MEMORY_BARRIER() __asm__ volatile("fence rw, rw" ::: "memory")
			#endif
			
		/* RTEMS RTOS */
		#elif defined(__rtems__)
			#if defined(__GNUC__) || defined(__clang__)
				#define CB_MEMORY_BARRIER() __asm__ __volatile__("" ::: "memory")
			#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
				#include <stdatomic.h>
				#define CB_MEMORY_BARRIER() atomic_thread_fence(memory_order_seq_cst)
			#endif
		
		/* Android NDK */
		#elif defined(__ANDROID__)
			#if defined(__aarch64__)
				#define CB_MEMORY_BARRIER() __asm__ __volatile__("dmb ish" ::: "memory")
			#elif defined(__arm__)
				#define CB_MEMORY_BARRIER() __asm__ __volatile__("dmb" ::: "memory")
			#elif defined(__GNUC__) || defined(__clang__)
				#define CB_MEMORY_BARRIER() __asm__ __volatile__("" ::: "memory")
			#endif
			
		/* IAR for RISC-V */
		#elif defined(__IAR_SYSTEMS_ICC__) && defined(__riscv)
			#include <intrinsics.h>
			#define CB_MEMORY_BARRIER() __iar_builtin_MB()
		
		/* TI Compiler for C28x DSPs */
		#elif defined(__TI_COMPILER_VERSION__) && (defined(_TMS320C28X) || defined(__TMS320C28XX__))
			#define CB_MEMORY_BARRIER() asm(" CSYNC")
		
		/* embOS (SEGGER RTOS) */
		#elif defined(USE_EMBOS) || defined(OS_EMBOS_H) || defined(OS_Global_H) || defined(OS_MAIN_H)
			#include <RTOS.h>
			#define CB_MEMORY_BARRIER() OS_MemoryBarrier()
		
		/* Mbed OS */
		#elif defined(MBED_TOOLCHAIN_H) || defined(MBED_OS_H)
			#include <mbed_toolchain.h>
			#define CB_MEMORY_BARRIER() MBED_BARRIER()
		
		/* QNX RTOS */
		#elif defined(__QNX__)
			#if defined(__GNUC__)
				#define CB_MEMORY_BARRIER() __asm__ __volatile__("" ::: "memory")
			#else
				#include <sys/mfence.h>
				#define CB_MEMORY_BARRIER() __mfence()
			#endif
		
		/* MIPS */
		#elif defined(__mips__) && (defined(__GNUC__) || defined(__clang__))
			#define CB_MEMORY_BARRIER() __asm__ __volatile__("sync" ::: "memory")
		
		/* PowerPC */
		#elif (defined(__powerpc__) || defined(__ppc__) || defined(__PPC__)) && (defined(__GNUC__) || defined(__clang__))
			#define CB_MEMORY_BARRIER() __asm__ __volatile__("sync" ::: "memory")
		
		/* Azure RTOS (ThreadX) */
		#elif defined(__THREADX__) || defined(TX_INCLUDE_H)
			/* Use architecture-specific fallbacks */
			#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
				#define CB_MEMORY_BARRIER() __asm__ volatile ("dmb" ::: "memory")
			#elif defined(__riscv)
				#define CB_MEMORY_BARRIER() __asm__ volatile ("fence" ::: "memory")
			#elif defined(__GNUC__) || defined(__clang__)
				#define CB_MEMORY_BARRIER() __asm__ __volatile__("" ::: "memory")
			#else
				#define CB_MEMORY_BARRIER() ((void)0)
			#endif
		
		/* uC/OS-II & uC/OS-III - SIMPLIFIED */
		#elif defined(OS_CFG_H) || defined(OS_CPU_H)
			/* Generic implementation for all architectures */
			#if defined(__GNUC__) || defined(__clang__)
				#if defined(__arm__)
					#define CB_MEMORY_BARRIER() __asm__ volatile ("dmb" ::: "memory")
				#elif defined(__riscv)
					#define CB_MEMORY_BARRIER() __asm__ volatile ("fence" ::: "memory")
				#elif defined(__i386__) || defined(__x86_64__)
					#define CB_MEMORY_BARRIER() __asm__ __volatile__("mfence" ::: "memory")
				#else
					#define CB_MEMORY_BARRIER() __asm__ __volatile__("" ::: "memory")
				#endif
			#else
				#define CB_MEMORY_BARRIER() ((void)0)
			#endif

		/* ---------- COMPILER BARRIERS ---------- */
		#elif defined(__GNUC__) || defined(__clang__)
			#define CB_MEMORY_BARRIER() __asm__ __volatile__("" ::: "memory")

		#elif defined(_MSC_VER)
			#include <intrin.h>
			#pragma intrinsic(_ReadWriteBarrier)
			#define CB_MEMORY_BARRIER() _ReadWriteBarrier()

		#elif defined(__ICCARM__) || defined(__IAR_SYSTEMS_ICC__)
			#include <intrinsics.h>
			#define CB_MEMORY_BARRIER() __memory_barrier()

		#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
			#if __ARMCC_VERSION >= 6010050
				#define CB_MEMORY_BARRIER() __asm volatile("dmb ish" ::: "memory")
			#else
				#define CB_MEMORY_BARRIER() __schedule_barrier()
			#endif

		/* ---------- RTOS SPECIFIC ---------- */
		#elif defined(FREERTOS_H) || defined(INC_FREERTOS_H)
			#ifdef portMEMORY_BARRIER
				#define CB_MEMORY_BARRIER() portMEMORY_BARRIER()
			#else
				#define CB_MEMORY_BARRIER() vPortMemoryBarrier()
			#endif

		#elif defined(CONFIG_ZEPHYR)
			#include <sys/atomic.h>
			#define CB_MEMORY_BARRIER() atomic_thread_fence(memory_order_seq_cst)

		#elif defined(__VXWORKS__)
			#include <vxAtomicLib.h>
			#define CB_MEMORY_BARRIER() vxAtomicMemBarrier()

		/* ---------- GENERIC FALLBACK ---------- */
		#else
			#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
				#include <stdatomic.h>
				#define CB_MEMORY_BARRIER() atomic_thread_fence(memory_order_seq_cst)
			#else
				#warning "Using empty memory barrier"
				#define CB_MEMORY_BARRIER() ((void)0)
			#endif
		#endif
	#endif
#endif

#endif /* CB_MEMORYBARRIER_DETECT_H */