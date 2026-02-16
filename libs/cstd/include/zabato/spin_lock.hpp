#pragma once

#include <stdatomic.h>

namespace zabato
{
class spin_lock
{
public:
    spin_lock() { atomic_flag_clear(&m_lock_flag); }
    ~spin_lock() = default;

    void lock()
    {
        while (atomic_flag_test_and_set(&m_lock_flag))
            m_lock_flag.wait(true);
    }

    void unlock()
    {
        atomic_flag_clear(&m_lock_flag);
        m_lock_flag.notify_one();
    }

private:
    std::atomic_flag m_lock_flag;
};
} // namespace zabato

#undef SPIN_PAUSE