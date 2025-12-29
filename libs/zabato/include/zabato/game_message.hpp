#pragma once

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <zabato/symbol.hpp>
#include <zabato/value.hpp>

#include <zabato/uuid.hpp>

namespace zabato
{
const size_t QUEUE_SIZE = 256;

struct game_message
{
    symbol *msg_id;
    uuid sender_id;
    uuid receiver_id;
    value data;
};

class game_message_queue
{
public:
    game_message_queue() : m_buffer(), m_head(0), m_tail(0) {}

    bool push(const game_message &msg)
    {
        unsigned int current_head =
            atomic_load_explicit(&m_head, memory_order_relaxed);
        unsigned int current_tail =
            atomic_load_explicit(&m_tail, memory_order_acquire);

        unsigned int next_head = (current_head + 1) & (QUEUE_SIZE - 1);

        if (next_head == current_tail)
            return false; // Full

        m_buffer[current_head] = msg;

        atomic_store_explicit(&m_head, next_head, memory_order_release);
        return true;
    }

    bool pop(game_message &out_msg)
    {
        unsigned int current_tail =
            atomic_load_explicit(&m_tail, memory_order_relaxed);
        unsigned int current_head =
            atomic_load_explicit(&m_head, memory_order_acquire);

        if (current_head == current_tail)
            return false; // Empty

        out_msg                = m_buffer[current_tail];
        unsigned int next_tail = (current_tail + 1) & (QUEUE_SIZE - 1);

        atomic_store_explicit(&m_tail, next_tail, memory_order_release);
        return true;
    }

private:
    game_message m_buffer[QUEUE_SIZE];
    atomic_uint m_head;
    atomic_uint m_tail;
};
} // namespace zabato
