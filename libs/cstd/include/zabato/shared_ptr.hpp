#pragma once

#include <zabato/utils.hpp>

#include <stdalign.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

namespace zabato
{
template <typename T> class shared_ptr;
template <typename T> class weak_ptr;

/**
 * @brief Base class for reference counting control blocks.
 *
 * Handles thread-safe reference counting for both strong and weak references.
 */
struct control_block_base
{
    /** @brief Strong reference count. */
    atomic_uint ref_count;
    /** @brief Weak reference count. */
    atomic_uint weak_count;

    /** @brief Initializes reference counts to 1 (strong) and 1 (weak). */
    control_block_base()
    {
        atomic_init(&ref_count, 1);
        atomic_init(&weak_count, 1); // 1 for the strong ref holders
    }

    /** @brief Virtual destructor. */
    virtual ~control_block_base() {}

    /** @brief Destroys the managed object. */
    virtual void destroy_object() = 0;
    /** @brief Destroys the control block itself. */
    virtual void destroy_self() = 0;

    /** @brief Atomically increments the strong reference count. */
    void add_ref()
    {
        atomic_fetch_add_explicit(&ref_count, 1, memory_order_relaxed);
    }

    /**
     * @brief Atomically decrements the strong reference count.
     *
     * If the count reaches zero, the managed object is destroyed and the weak
     * reference count is decremented.
     */
    void release_ref()
    {
        if (atomic_fetch_sub_explicit(&ref_count, 1, memory_order_acq_rel) == 1)
        {
            destroy_object();
            release_weak();
        }
    }

    /** @brief Atomically increments the weak reference count. */
    void add_weak()
    {
        atomic_fetch_add_explicit(&weak_count, 1, memory_order_relaxed);
    }

    /**
     * @brief Atomically decrements the weak reference count.
     *
     * If the count reaches zero, the control block itself is destroyed.
     */
    void release_weak()
    {
        if (atomic_fetch_sub_explicit(&weak_count, 1, memory_order_acq_rel) ==
            1)
        {
            destroy_self();
        }
    }

    /**
     * @brief Tries to atomically increment the strong reference count if it is
     * not zero.
     *
     * @return true providing the reference count was incremented, false if the
     * object is already destroyed.
     */
    bool try_add_ref()
    {
        unsigned int count =
            atomic_load_explicit(&ref_count, memory_order_relaxed);
        do
        {
            if (count == 0)
                return false;
        } while (!atomic_compare_exchange_weak_explicit(&ref_count,
                                                        &count,
                                                        count + 1,
                                                        memory_order_acquire,
                                                        memory_order_relaxed));
        return true;
    }
};

/**
 * @brief Control block for externally allocated pointers.
 *
 * Manages a pointer that was allocated separately from the control block.
 */
template <typename T> struct control_block_ptr : public control_block_base
{
    /** @brief Pointer to the managed object. */
    T *ptr;

    /** @brief Constructor taking ownership of p. */
    control_block_ptr(T *p) : ptr(p) {}

    /** @brief Deletes the managed object. */
    void destroy_object() override { delete ptr; }

    /** @brief Deletes the control block. */
    void destroy_self() override { delete this; }
};

/**
 * @brief Control block for objects allocated inplace.
 *
 * Uses a single allocation for both the control block and the object.
 * Used by make_shared.
 */
template <typename T> struct control_block_inplace : public control_block_base
{
    /** @brief Storage for the object, properly aligned. */
    alignas(T) unsigned char storage[sizeof(T)];

    /** @brief Constructs the object in-place. */
    template <typename... Args> control_block_inplace(Args &&...args)
    {
        new (storage) T(zabato::forward<Args>(args)...);
    }

    /** @brief Destroys the in-place object. */
    void destroy_object() override { reinterpret_cast<T *>(storage)->~T(); }

    /** @brief Deletes the entire control block allocation. */
    void destroy_self() override { delete this; }
};

/**
 * @brief Smart pointer class that maintains shared ownership of an object
 * through reference counting.
 *
 * @tparam T The type of the managed object.
 */
template <typename T> class shared_ptr
{
public:
    /** @brief The type of the managed object. */
    using element_type = T;

    /** @brief Constructs an empty shared_ptr. */
    shared_ptr() : m_ptr(nullptr), m_cb(nullptr) {}

    /** @brief Constructs an empty shared_ptr from nullptr. */
    shared_ptr(std::nullptr_t) : m_ptr(nullptr), m_cb(nullptr) {}

    /**
     * @brief Constructs a shared_ptr that owns the given pointer.
     *
     * @param ptr The pointer to take ownership of.
     */
    explicit shared_ptr(T *ptr) : m_ptr(ptr)
    {
        if (ptr)
        {
            try
            {
                m_cb = new control_block_ptr<T>(ptr);
            }
            catch (...)
            {
                delete ptr;
                throw;
            }
        }
        else
        {
            m_cb = nullptr;
        }
    }

    /** @brief Copy constructor. incrementing the reference count. */
    shared_ptr(const shared_ptr &other) : m_ptr(other.m_ptr), m_cb(other.m_cb)
    {
        if (m_cb)
            m_cb->add_ref();
    }

    /** @brief Copy constructor from a shared_ptr of a compatible type. */
    template <typename Y>
    shared_ptr(const shared_ptr<Y> &other)
        : m_ptr(other.get()), m_cb(other.m_cb)
    {
        if (m_cb)
            m_cb->add_ref();
    }

    /** @brief Move constructor. Transfers ownership. */
    shared_ptr(shared_ptr &&other) noexcept
        : m_ptr(other.m_ptr), m_cb(other.m_cb)
    {
        other.m_ptr = nullptr;
        other.m_cb  = nullptr;
    }

    /** @brief Move constructor from a shared_ptr of a compatible type. */
    template <typename Y>
    shared_ptr(shared_ptr<Y> &&other) noexcept
        : m_ptr(other.get()), m_cb(other.m_cb)
    {
        other.m_ptr = nullptr;
        other.m_cb  = nullptr;
    }

    /** @brief Destructor. Decrements the reference count. */
    ~shared_ptr()
    {
        if (m_cb)
            m_cb->release_ref();
    }

    /** @brief Copy assignment operator. */
    shared_ptr &operator=(const shared_ptr &other)
    {
        shared_ptr(other).swap(*this);
        return *this;
    }

    /** @brief Move assignment operator. */
    shared_ptr &operator=(shared_ptr &&other) noexcept
    {
        shared_ptr(zabato::move(other)).swap(*this);
        return *this;
    }

    /** @brief Copy assignment from a compatible shared_ptr. */
    template <typename Y> shared_ptr &operator=(const shared_ptr<Y> &other)
    {
        shared_ptr(other).swap(*this);
        return *this;
    }

    /** @brief Move assignment from a compatible shared_ptr. */
    template <typename Y> shared_ptr &operator=(shared_ptr<Y> &&other) noexcept
    {
        shared_ptr(zabato::move(other)).swap(*this);
        return *this;
    }

    /** @brief Swaps the contents of this shared_ptr with another. */
    void swap(shared_ptr &other) noexcept
    {
        zabato::swap(m_ptr, other.m_ptr);
        zabato::swap(m_cb, other.m_cb);
    }

    /** @brief Resets the shared_ptr to be empty. */
    void reset() { shared_ptr().swap(*this); }

    /** @brief Resets the shared_ptr to own the new pointer. */
    void reset(T *ptr) { shared_ptr(ptr).swap(*this); }

    /** @brief Returns the raw pointer. */
    T *get() const { return m_ptr; }
    /** @brief Dereferences the stored pointer. */
    T &operator*() const { return *m_ptr; }
    /** @brief Accesses members of the stored object. */
    T *operator->() const { return m_ptr; }
    /** @brief Checks if the shared_ptr is non-empty. */
    explicit operator bool() const { return m_ptr != nullptr; }

    /** @brief Returns the current reference count. */
    unsigned int use_count() const
    {
        return m_cb ? atomic_load_explicit(&m_cb->ref_count,
                                           memory_order_relaxed)
                    : 0;
    }

private:
    T *m_ptr;
    control_block_base *m_cb;

    // For make_shared
    shared_ptr(control_block_base *cb, T *ptr) : m_ptr(ptr), m_cb(cb) {}

    template <typename U> friend class shared_ptr;
    template <typename U> friend class weak_ptr;
    template <typename U, typename... Args>
    friend shared_ptr<U> make_shared(Args &&...args);

    template <typename T2, typename U>
    friend shared_ptr<T2> static_pointer_cast(const shared_ptr<U> &r);
};

template <typename T, typename U>
shared_ptr<T> static_pointer_cast(const shared_ptr<U> &r)
{
    auto p = static_cast<typename shared_ptr<T>::element_type *>(r.get());
    if (r.m_cb)
        r.m_cb->add_ref();
    return shared_ptr<T>(r.m_cb, p);
}

/**
 * @brief Smart pointer class that holds a non-owning ("weak") reference to an
 * object managed by shared_ptr.
 *
 * @tparam T The type of the managed object.
 */
template <typename T> class weak_ptr
{
public:
    /** @brief Constructs an empty weak_ptr. */
    weak_ptr() : m_ptr(nullptr), m_cb(nullptr) {}

    /** @brief Copy constructor. Increments weak reference count. */
    weak_ptr(const weak_ptr &other) : m_ptr(other.m_ptr), m_cb(other.m_cb)
    {
        if (m_cb)
            m_cb->add_weak();
    }

    /** @brief Constructs from a shared_ptr. */
    template <typename Y>
    weak_ptr(const shared_ptr<Y> &other) : m_ptr(other.get()), m_cb(other.m_cb)
    {
        if (m_cb)
            m_cb->add_weak();
    }

    /** @brief Copy constructor from a compatible weak_ptr. */
    template <typename Y>
    weak_ptr(const weak_ptr<Y> &other) : m_ptr(other.m_ptr), m_cb(other.m_cb)
    {
        if (m_cb)
            m_cb->add_weak();
    }

    /** @brief Move constructor. */
    weak_ptr(weak_ptr &&other) noexcept : m_ptr(other.m_ptr), m_cb(other.m_cb)
    {
        other.m_ptr = nullptr;
        other.m_cb  = nullptr;
    }

    /** @brief Move constructor from a compatible weak_ptr. */
    template <typename Y>
    weak_ptr(weak_ptr<Y> &&other) noexcept
        : m_ptr(other.m_ptr), m_cb(other.m_cb)
    {
        other.m_ptr = nullptr;
        other.m_cb  = nullptr;
    }

    /** @brief Destructor. Decrements weak reference count. */
    ~weak_ptr()
    {
        if (m_cb)
            m_cb->release_weak();
    }

    /** @brief Copy assignment. */
    weak_ptr &operator=(const weak_ptr &other)
    {
        weak_ptr(other).swap(*this);
        return *this;
    }

    /** @brief Copy assignment from compatible weak_ptr. */
    template <typename Y> weak_ptr &operator=(const weak_ptr<Y> &other)
    {
        weak_ptr(other).swap(*this);
        return *this;
    }

    /** @brief Move assignment. */
    weak_ptr &operator=(weak_ptr &&other) noexcept
    {
        weak_ptr(zabato::move(other)).swap(*this);
        return *this;
    }

    /** @brief Move assignment from compatible weak_ptr. */
    template <typename Y> weak_ptr &operator=(weak_ptr<Y> &&other) noexcept
    {
        weak_ptr(zabato::move(other)).swap(*this);
        return *this;
    }

    /** @brief Assignment from compatible shared_ptr. */
    template <typename Y> weak_ptr &operator=(const shared_ptr<Y> &other)
    {
        weak_ptr(other).swap(*this);
        return *this;
    }

    /** @brief Swaps contents with another weak_ptr. */
    void swap(weak_ptr &other) noexcept
    {
        zabato::swap(m_ptr, other.m_ptr);
        zabato::swap(m_cb, other.m_cb);
    }

    /** @brief Resets to empty. */
    void reset() { weak_ptr().swap(*this); }

    /** @brief Returns current use count of the managed object. */
    long use_count() const
    {
        return m_cb ? atomic_load_explicit(&m_cb->ref_count,
                                           memory_order_relaxed)
                    : 0;
    }

    /** @brief Checks if the managed object has been deleted. */
    bool expired() const { return use_count() == 0; }

    /**
     * @brief Creates a shared_ptr to the managed object.
     * @return shared_ptr<T> containing the object if it exists, otherwise
     * empty.
     */
    shared_ptr<T> lock() const
    {
        if (m_cb && m_cb->try_add_ref())
        {
            return shared_ptr<T>(m_cb, m_ptr);
        }
        return shared_ptr<T>();
    }

private:
    T *m_ptr;
    control_block_base *m_cb;

    template <typename U> friend class weak_ptr;
    template <typename U> friend class shared_ptr;
};

/**
 * @brief Constructs an object of type T and wraps it in a shared_ptr, using a
 * single allocation.
 *
 * @tparam T Type of object to create.
 * @tparam Args Argument types for T's constructor.
 * @param args Arguments to pass to T's constructor.
 * @return shared_ptr<T> owning the new object.
 */
template <typename T, typename... Args>
shared_ptr<T> make_shared(Args &&...args)
{
    auto *cb = new control_block_inplace<T>(zabato::forward<Args>(args)...);
    return shared_ptr<T>(cb, reinterpret_cast<T *>(cb->storage));
}

/** @name Comparison Operators */
///@{
template <typename T, typename U>
bool operator==(const shared_ptr<T> &lhs, const shared_ptr<U> &rhs)
{
    return lhs.get() == rhs.get();
}

template <typename T> bool operator==(const shared_ptr<T> &lhs, std::nullptr_t)
{
    return !lhs;
}

template <typename T> bool operator==(std::nullptr_t, const shared_ptr<T> &rhs)
{
    return !rhs;
}

template <typename T, typename U>
bool operator!=(const shared_ptr<T> &lhs, const shared_ptr<U> &rhs)
{
    return lhs.get() != rhs.get();
}

template <typename T> bool operator!=(const shared_ptr<T> &lhs, std::nullptr_t)
{
    return (bool)lhs;
}

template <typename T> bool operator!=(std::nullptr_t, const shared_ptr<T> &rhs)
{
    return (bool)rhs;
}
///@}

} // namespace zabato