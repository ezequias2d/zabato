#pragma once

#include <malloc.h>
#include <stddef.h>
#include <stdint.h>

namespace zabato
{
static size_t g_total_memory_allocated = 0;

// TODO: improve this allocator
template <class T> class allocator
{
public:
    using value_type      = T;
    using pointer         = T *;
    using const_pointer   = const T *;
    using reference       = T &;
    using const_reference = const T &;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    template <class U> class rebind
    {
    public:
        using other = allocator<U>;
    };

    allocator() = default;

    template <class U> constexpr allocator(const allocator<U> &) noexcept {}

    T *allocate(size_t n)
    {
        if (n > ((size_t)-1) / sizeof(T))
            // if (n > ((size_t)-1 - header_size()) / sizeof(T))
            return nullptr;

        size_t total_size = n * sizeof(T);

        uint8_t *block = (uint8_t *)malloc(total_size);
        // uint8_t *block = (uint8_t *)malloc(total_size + header_size());
        if (!block)
            return nullptr;

        // *(size_t *)block = total_size;
        // g_total_memory_allocated += total_size;

        // return (T *)(block + header_size());
        return (T *)block;
    }

    T *reallocate(T *p, size_t n)
    {
        if (!p)
        {
            return allocate(n);
        }

        // uint8_t *block  = (uint8_t *)p - header_size();
        // size_t old_size = *(size_t *)block;
        size_t new_size = n * sizeof(T);

        uint8_t *new_block =
            // (uint8_t *)realloc(block, new_size + header_size());
            (uint8_t *)realloc(p, new_size);
        if (!new_block)
            return nullptr;

        // *(size_t *)new_block = new_size;
        // g_total_memory_allocated += (new_size - old_size);

        // return (T *)(new_block + header_size());
        return (T *)new_block;
    }

    void deallocate(T *p, size_t n)
    {
        (void)n;
        if (!p)
            return;

        // uint8_t *block = (uint8_t *)p - header_size();
        // uint8_t *block     = (uint8_t *)p - header_size();
        // size_t stored_size = *(size_t *)block;
        // g_total_memory_allocated -= stored_size;

        free(p);
    }

private:
    /*
    static constexpr size_t header_size()
    {
        return ((sizeof(size_t) + alignof(max_align_t) - 1) /
                alignof(max_align_t)) *
               alignof(max_align_t);
    }
    */
};
} // namespace zabato
