#pragma once

#include "span.hpp"
#include <stdint.h>

namespace zabato
{
/**
 * @class random
 * @brief Utilities for generating random data.
 */
class random
{
public:
    /**
     * @brief Generates a random 64-bit integer.
     * @return A random 64-bit integer.
     */
    static uint64_t rand();

    /**
     * @brief Fills a buffer with random bytes.
     * @param buf Pointer to the buffer.
     * @param size Size of the buffer in bytes.
     */
    static void buf(void *buf, size_t size);

    /**
     * @brief Fills a buffer with random bytes.
     * @param buf The buffer to fill.
     */
    static void buf(buffer &buf);
};
} // namespace zabato