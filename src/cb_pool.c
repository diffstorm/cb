/**
    @file        cb_pool.c
    @brief       Memory pool / buffer arena implementation for the circular buffer library
    
    @date 2025-11-08
    @version 1.3
    @author Eray Ozturk | erayozturk1@gmail.com
    @url github.com/diffstorm/cb
    @license MIT License
*/

#include "cb_pool.h"
#include <string.h>

cb_pool_result_t cb_pool_init(cb_pool_t *pool, CbItem *storage, CbIndex size) {
    if (!pool) {
        return CB_POOL_ERROR_NULL_POINTER;
    }
    
    if (!storage) {
        return CB_POOL_ERROR_NULL_POINTER;
    }
    
    if (size < CB_POOL_MAX_BUFFERS) {
        /* Need at least one slot per potential buffer */
        return CB_POOL_ERROR_INVALID_SIZE;
    }
    
    memset(pool, 0, sizeof(cb_pool_t));
    pool->pool = storage;
    pool->pool_size = size;
    pool->allocated = 0;
    
    return CB_POOL_SUCCESS;
}

cb_pool_result_t cb_pool_create_buffer(cb_pool_t *pool, cb **buffer, CbIndex size) {
    if (!pool) {
        return CB_POOL_ERROR_NULL_POINTER;
    }
    
    if (!buffer) {
        return CB_POOL_ERROR_NULL_POINTER;
    }
    
    if (!pool->pool) {
        return CB_POOL_ERROR_BUFFER_CORRUPTED;
    }
    
    if (size == 0) {
        return CB_POOL_ERROR_INVALID_SIZE;
    }
    
    if (size > pool->pool_size - pool->allocated) {
        return CB_POOL_ERROR_OUT_OF_MEMORY;
    }
    
    /* Find a free buffer slot */
    for (size_t i = 0; i < CB_POOL_MAX_BUFFERS; i++) {
        if (!pool->in_use[i]) {
            /* Initialize this buffer */
            CbIndex offset = pool->allocated;
            CbItem *buffer_storage = &pool->pool[offset];
            
            cb_init_ex(&pool->buffers[i], buffer_storage, size);
            
            pool->in_use[i] = true;
            pool->allocated += size;
            
            *buffer = &pool->buffers[i];
            
            return CB_POOL_SUCCESS;
        }
    }
    
    return CB_POOL_ERROR_NO_FREE_BUFFER;
}

cb_pool_result_t cb_pool_destroy_buffer(cb_pool_t *pool, cb *buffer) {
    if (!pool) {
        return CB_POOL_ERROR_NULL_POINTER;
    }
    
    if (!buffer) {
        return CB_POOL_ERROR_NULL_POINTER;
    }
    
    /* Find the buffer in the pool */
    for (size_t i = 0; i < CB_POOL_MAX_BUFFERS; i++) {
        if (pool->in_use[i] && &pool->buffers[i] == buffer) {
            /* Calculate the size of this buffer */
            CbIndex buffer_size = buffer->size;
            
            pool->in_use[i] = false;
            pool->allocated -= buffer_size;
            
            /* Clear the buffer structure (optional - for safety) */
            buffer->buf = NULL;
            buffer->size = 0;
            
            return CB_POOL_SUCCESS;
        }
    }
    
    return CB_POOL_ERROR_BUFFER_CORRUPTED;  /* Buffer not found in pool */
}

CbIndex cb_pool_allocated(const cb_pool_t *pool) {
    if (!pool) {
        return 0;
    }
    
    return pool->allocated;
}

CbIndex cb_pool_free_space(const cb_pool_t *pool) {
    if (!pool) {
        return 0;
    }
    
    if (pool->pool_size <= pool->allocated) {
        return 0;
    }
    
    return pool->pool_size - pool->allocated;
}

size_t cb_pool_buffer_count(const cb_pool_t *pool) {
    if (!pool) {
        return 0;
    }
    
    size_t count = 0;
    for (size_t i = 0; i < CB_POOL_MAX_BUFFERS; i++) {
        if (pool->in_use[i]) {
            count++;
        }
    }
    
    return count;
}