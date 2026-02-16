#pragma once

#include <zabato/base_object.hpp>

namespace zabato
{

/**
 * @brief Smart pointer class for automatic reference counting management.
 *
 * This class provides intrusive reference counting semantics for objects
 * derived from `object`. It automatically calls add_ref() when a pointer is
 * attached/copied and release() when the pointer is destroyed or reassigned.
 * This ensures objects are not deleted while valid references exist and are
 * automatically cleaned up when the last reference is dropped.
 *
 * @tparam T The type of object pointed to. Must inherit from `object`.
 */
template <class T> class pointer
{
public:
    /**
     * @brief Constructs a smart pointer from a raw pointer.
     *
     * Increments the reference count of the target object if it is not null.
     *
     * @param ptr The raw pointer to take ownership of. Defaults to nullptr.
     */
    pointer(T *ptr = nullptr)
    {
        m_object = ptr;
        if (m_object)
            m_object->add_ref();
    }

    /**
     * @brief Copy constructor.
     *
     * Shares ownership of the object pointed to by `ptr`. Increments the
     * reference count.
     *
     * @param ptr The other smart pointer to copy from.
     */
    pointer(const pointer &ptr)
    {
        m_object = ptr.m_object;
        if (m_object)
            m_object->add_ref();
    }

    /**
     * @brief Destructor.
     *
     * Decrements the reference count of the managed object. If the count
     * reaches zero, the object automatically deletes itself (via
     * `object::release`).
     */
    ~pointer()
    {
        if (m_object)
            m_object->release();
    }

    operator T *() const { return m_object; }
    T &operator*() const { return *m_object; }
    T *operator->() const { return m_object; }

    /**
     * @brief Assignment operator from raw pointer.
     *
     * Releases the currently held object (if any) and takes shared ownership of
     * the new object. Handles self-assignment checks implicitly via logic order
     * or explicit checks.
     *
     * @param obj The new raw pointer to manage.
     * @return Reference to this smart pointer.
     */
    pointer &operator=(T *obj)
    {
        if (m_object == obj)
            return *this;

        if (obj)
            obj->add_ref();

        if (m_object)
            m_object->release();

        m_object = obj;

        return *this;
    }

    /**
     * @brief Assignment operator from another smart pointer.
     *
     * Releases the currently held object (if any) and shares ownership of the
     * object held by `reference`.
     *
     * @param reference The other smart pointer to assign from.
     * @return Reference to this smart pointer.
     */
    pointer &operator=(const pointer &reference)
    {
        if (m_object == reference.m_object)
            return *this;

        if (reference.m_object)
            reference.m_object->add_ref();

        if (m_object)
            m_object->release();

        m_object = reference.m_object;

        return *this;
    }

    T *get() const { return m_object; }

    bool operator==(T *obj) const { return m_object == obj; }
    bool operator!=(T *obj) const { return m_object != obj; }

    bool operator==(const pointer &reference) const
    {
        return m_object == reference.m_object;
    }

    bool operator!=(const pointer &reference) const
    {
        return m_object != reference.m_object;
    }

protected:
    T *m_object;
};

} // namespace zabato
