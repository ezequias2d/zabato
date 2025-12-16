#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

namespace zabato
{
template <typename T, size_t N> class pool
{
private:
    union Slot
    {
        T data;
        uint32_t next_free_index;
    };

    Slot m_buffer[N];
    uint32_t m_free_head;
    uint32_t m_used_count;

    static constexpr uint32_t END_OF_LIST = 0xFFFFFFFF;

public:
    void init()
    {
        m_used_count = 0;
        m_free_head  = 0;

        for (uint32_t i = 0; i < N - 1; ++i)
            m_buffer[i].next_free_index = i + 1;

        m_buffer[N - 1].next_free_index = END_OF_LIST;
    }

    T *alloc()
    {
        if (m_free_head == END_OF_LIST)
        {
            assert(0);
            return nullptr;
        }
        uint32_t index = m_free_head;
        Slot *slot     = &m_buffer[index];

        m_free_head = slot->next_free_index;
        m_used_count++;
        memset(&slot->data, 0, sizeof(T));

        return &slot->data;
    }

    void free(void *ptr)
    {
        if (!ptr)
            return;

        Slot *slot = (Slot *)ptr;

        if (slot < m_buffer || slot >= m_buffer + N)
        {
            assert(0);
            return;
        }

        uint32_t index        = (uint32_t)(slot - m_buffer);
        slot->next_free_index = m_free_head;
        m_free_head           = index;
        m_used_count--;
    }

    size_t count() const { return m_used_count; }
    size_t capacity() const { return N; }
};
} // namespace zabato
