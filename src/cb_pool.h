/**
    @file        cb_pool.h
    @brief       Memory pool / buffer arena support for the circular buffer library
    @details     Allows multiple buffers to share a memory pool
                 Useful for embedded systems with constrained memory
    
    @date 2025-11-08
    @version 1.3
    @author Eray Ozturk | erayozturk1@gmail.com
    @url github.com/diffstorm/cb
    @license MIT License
*/

#ifndef CB_POOL_H
#define CB_POOL_H

#include "cb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of buffers in a pool */
#ifndef CB_POOL_MAX_BUFFERS
    #define CB_POOL_MAX_BUFFERS 8
#endif

/* Memory pool structure */
typedef struct {
    CbItem *pool;           /* Shared memory pool */
    CbIndex pool_size;      /* Total size of the pool in items */
    CbIndex allocated;      /* Total allocated items across all buffers */
    cb buffers[CB_POOL_MAX_BUFFERS];  /* Buffer structures */
    bool in_use[CB_POOL_MAX_BUFFERS]; /* Buffer usage flags */
} cb_pool_t;

/* Result codes for pool operations */
typedef enum {
    CB_POOL_SUCCESS = 0,
    CB_POOL_ERROR_NULL_POINTER,
    CB_POOL_ERROR_INVALID_SIZE,
    CB_POOL_ERROR_NO_FREE_BUFFER,
    CB_POOL_ERROR_OUT_OF_MEMORY,
    CB_POOL_ERROR_BUFFER_CORRUPTED
} cb_pool_result_t;

/**
 * @brief Initialize a memory pool
 * @param pool Pointer to the pool structure
 * @param storage Pointer to the shared memory storage
 * @param size Total size of the memory pool in items
 * @return CB_POOL_SUCCESS on success, error code otherwise
 */
cb_pool_result_t cb_pool_init(cb_pool_t *pool, CbItem *storage, CbIndex size);

/**
 * @brief Create a buffer from the pool
 * @param pool Pointer to the pool structure
 * @param buffer Pointer to store the buffer pointer
 * @param size Requested buffer size
 * @return CB_POOL_SUCCESS on success, error code otherwise
 */
cb_pool_result_t cb_pool_create_buffer(cb_pool_t *pool, cb **buffer, CbIndex size);

/**
 * @brief Destroy a buffer and return its memory to the pool
 * @param pool Pointer to the pool structure
 * @param buffer Pointer to the buffer to destroy
 * @return CB_POOL_SUCCESS on success, error code otherwise
 */
cb_pool_result_t cb_pool_destroy_buffer(cb_pool_t *pool, cb *buffer);

/**
 * @brief Get the total allocated memory in the pool
 * @param pool Pointer to the pool structure
 * @return Total allocated items
 */
CbIndex cb_pool_allocated(const cb_pool_t *pool);

/**
 * @brief Get the remaining free memory in the pool
 * @param pool Pointer to the pool structure
 * @return Remaining free items
 */
CbIndex cb_pool_free_space(const cb_pool_t *pool);

/**
 * @brief Get the number of active buffers in the pool
 * @param pool Pointer to the pool structure
 * @return Number of active buffers
 */
size_t cb_pool_buffer_count(const cb_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif /* CB_POOL_H */