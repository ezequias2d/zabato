#pragma once

namespace zabato
{
template <typename T> class unique_ptr
{
public:
    explicit unique_ptr(T *ptr = nullptr) : m_ptr(ptr) {}

    ~unique_ptr() noexcept
    {
        if (m_ptr)
            delete m_ptr;
    }

    unique_ptr(const unique_ptr &)            = delete;
    unique_ptr &operator=(const unique_ptr &) = delete;

    unique_ptr(unique_ptr &&other) noexcept : m_ptr(other.m_ptr)
    {
        other.m_ptr = nullptr;
    }

    template <typename U>
    unique_ptr(unique_ptr<U> &&other) noexcept : m_ptr(other.release())
    {
    }

    T *release() noexcept
    {
        T *ptr = m_ptr;
        m_ptr  = nullptr;
        return ptr;
    }

    operator bool() const { return m_ptr != nullptr; }

    unique_ptr &operator=(unique_ptr &&other) noexcept
    {
        if (this != &other)
        {
            if (m_ptr)
                delete m_ptr;
            m_ptr       = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    T *operator->() const { return m_ptr; }
    T &operator*() const { return *m_ptr; }

    T *get() const { return m_ptr; }

private:
    T *m_ptr;
};
} // namespace zabato