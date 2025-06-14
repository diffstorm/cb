#ifndef CB_ATOMIC_ACCESS_H
#define CB_ATOMIC_ACCESS_H

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

#endif /* CB_ATOMIC_ACCESS_H */
