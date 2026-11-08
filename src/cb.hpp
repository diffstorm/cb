/**
    @file        cb.hpp
    @brief       C++ template wrapper for the circular buffer library
    @details     Provides type-safe C++ interface with templates while maintaining
                 the underlying C implementation for compatibility
    
    @date 2025-11-08
    @version 1.3
    @author Eray Ozturk | erayozturk1@gmail.com
    @url github.com/diffstorm/cb
    @license MIT License
*/

#ifndef CB_HPP
#define CB_HPP

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <array>
#include <memory>

// Include the C header
#include "cb.h"

namespace cb {

/**
 * @brief Base circular buffer class
 * @tparam T Item type
 * @tparam N Buffer size (number of items)
 * 
 * This class provides a type-safe C++ wrapper around the C cb library.
 * It manages buffer storage internally and provides RAII semantics.
 * 
 * @note This is a single-producer, single-consumer buffer
 * @note Thread-safe for one producer and one consumer
 */
template<typename T, std::size_t N>
class CircularBuffer {
public:
    static_assert(N > 0, "Buffer size must be greater than 0");
    
    using value_type = T;
    using size_type = std::size_t;
    using reference = T&;
    using const_reference = const T&;
    
    /**
     * @brief Construct a circular buffer
     */
    CircularBuffer() : buffer_(), storage_() {
        cb_init(&cb_, storage_.data(), N);
    }
    
    /**
     * @brief Copy constructor (deleted)
     */
    CircularBuffer(const CircularBuffer&) = delete;
    
    /**
     * @brief Copy assignment (deleted)
     */
    CircularBuffer& operator=(const CircularBuffer&) = delete;
    
    /**
     * @brief Move constructor (deleted)
     */
    CircularBuffer(CircularBuffer&&) = delete;
    
    /**
     * @brief Move assignment (deleted)
     */
    CircularBuffer& operator=(CircularBuffer&&) = delete;
    
    /**
     * @brief Destructor
     */
    ~CircularBuffer() = default;
    
    /**
     * @brief Check if buffer is empty
     * @return true if buffer is empty
     */
    bool empty() const noexcept {
        return cb_is_empty(&cb_);
    }
    
    /**
     * @brief Check if buffer is full
     * @return true if buffer is full
     */
    bool full() const noexcept {
        return cb_is_full(&cb_);
    }
    
    /**
     * @brief Get the number of items in the buffer
     * @return Number of items currently in the buffer
     */
    size_type size() const noexcept {
        return static_cast<size_type>(cb_dataSize(&cb_));
    }
    
    /**
     * @brief Get the maximum number of items the buffer can hold
     * @return Buffer capacity
     */
    constexpr size_type capacity() const noexcept {
        return N - 1;  // One slot reserved for full detection
    }
    
    /**
     * @brief Get number of free slots
     * @return Number of free slots in the buffer
     */
    size_type free_space() const noexcept {
        return static_cast<size_type>(cb_freeSpace(&cb_));
    }
    
    /**
     * @brief Insert an item into the buffer
     * @param item Item to insert
     * @return true if successful, false if buffer is full
     */
    bool push(const value_type& item) noexcept {
        return cb_insert(&cb_, item);
    }
    
    /**
     * @brief Insert an item into the buffer (move)
     * @param item Item to insert
     * @return true if successful, false if buffer is full
     */
    bool push(value_type&& item) noexcept {
        return cb_insert(&cb_, std::move(item));
    }
    
    /**
     * @brief Remove an item from the buffer
     * @param item Reference to store the removed item
     * @return true if successful, false if buffer is empty
     */
    bool pop(value_type& item) noexcept {
        return cb_remove(&cb_, &item);
    }
    
    /**
     * @brief Peek at an item without removing it
     * @param offset Offset from the oldest item (0 = oldest)
     * @param item Reference to store the peeked item
     * @return true if successful, false if offset is invalid
     */
    bool peek(size_type offset, value_type& item) const noexcept {
        return cb_peek(&cb_, static_cast<CbIndex>(offset), &item);
    }
    
    /**
     * @brief Clear the buffer (reset indices)
     */
    void clear() noexcept {
        cb_reset(&cb_);
    }
    
    /**
     * @brief Enable or disable overwrite mode
     * @param enable true to enable, false to disable
     */
    void set_overwrite(bool enable) noexcept {
        cb_set_overwrite(&cb_, enable);
    }
    
    /**
     * @brief Check if overwrite mode is enabled
     * @return true if overwrite mode is enabled
     */
    bool overwrite_enabled() const noexcept {
        return cb_get_overwrite(&cb_);
    }
    
    /**
     * @brief Get a pointer to the underlying C buffer structure
     * @return Pointer to the C cb structure
     */
    cb* c_buffer() noexcept {
        return &cb_;
    }
    
    /**
     * @brief Get a pointer to the underlying C buffer structure (const)
     * @return Pointer to the C cb structure
     */
    const cb* c_buffer() const noexcept {
        return &cb_;
    }
    
    /**
     * @brief Get the last error code
     * @return Last error code
     */
    cb_result_t last_error() const noexcept {
        return cb_get_last_error(&cb_).code;
    }
    
private:
    cb cb_;
    std::array<value_type, N> storage_;
};

/**
 * @brief Type alias for a circular buffer with a fixed-size array
 * @tparam T Item type
 * @tparam N Buffer size
 */
template<typename T, std::size_t N>
using ArrayBuffer = CircularBuffer<T, N>;

/**
 * @brief Type alias for a circular buffer backed by a C-style array
 * @tparam T Item type
 * @tparam N Buffer size
 */
template<typename T, std::size_t N>
using CArrayBuffer = CircularBuffer<T, N>;

/**
 * @brief Type alias for a circular buffer with dynamic storage
 * @tparam T Item type
 */
template<typename T>
using Buffer = CircularBuffer<T, 256>;  // Default size of 256

/**
 * @brief Custom allocator for memory pool support
 * @tparam T Item type
 * @tparam PoolSize Total items in pool
 */
template<typename T, std::size_t PoolSize>
class PoolAllocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    
    static constexpr size_type pool_size = PoolSize;
    
    PoolAllocator() : buffer_(), storage_(), used_(0) {
        // Initialize the default buffer
        cb_init(&buffer_, storage_.data(), PoolSize);
    }
    
    // Note: This is a simplified pool allocator for demonstration
    // A full implementation would support multiple buffers sharing pool memory
    
    cb* get_buffer() noexcept {
        return &buffer_;
    }
    
private:
    cb buffer_;
    std::array<T, PoolSize> storage_;
    size_type used_;
};

} // namespace cb

#endif // CB_HPP