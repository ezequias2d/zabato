#pragma once

#include <zabato/rtti.hpp>

namespace zabato
{
class base_object
{
public:
    base_object();
    virtual ~base_object();

#pragma region Type
    static const rtti TYPE;

    /**
     * @brief Get the Run-Time Type Information (RTTI) for this object.
     * @return The RTTI structure describing this object's type.
     */
    virtual const rtti &type() const { return TYPE; }

    /**
     * @brief Check if this object is exactly of the specified type.
     * @param t The type to check against.
     * @return true if the types match exactly, false otherwise.
     */
    bool is_exactly(const rtti &t) const { return type().is_exactly(t); }

    /**
     * @brief Check if this object is derived from the specified type.
     * @param t The base type to check against.
     * @return true if this object is derived from t, false otherwise.
     */
    bool is_derived(const rtti &t) const { return type().is_derived(t); }

    /**
     * @brief Check if this object is exactly the same type as another object.
     * @param obj The object to compare with.
     * @return true if both objects have exactly the same type.
     */
    bool is_exactly_typeof(const base_object *obj) const
    {
        return obj && is_exactly(obj->type());
    }

    /**
     * @brief Check if this object is of a type derived from the other object's
     * type.
     * @param obj The potential base object.
     * @return true if this object is derived from obj's type.
     */
    bool is_derived_typeof(const base_object *obj) const
    {
        return obj && is_derived(obj->type());
    }

    /**
     * @brief Populates the reflection data for this type.
     * @param r The reflection structure to populate.
     */
    static void reflect(reflection &r);
#pragma endregion Type

#pragma region Reference Count
    /**
     * @brief Increment the reference count of this object.
     */
    virtual void add_ref() { m_uiRefCount++; }

    /**
     * @brief Get the current reference count.
     * @return The reference count.
     */
    virtual unsigned int ref_count() const { return m_uiRefCount; }

    /**
     * @brief Decrement the reference count and delete the object if it reaches
     * zero.
     */
    virtual void release()
    {
        if (--m_uiRefCount == 0)
            delete this;
    }
#pragma endregion Reference Count

private:
    unsigned int m_uiRefCount;
};
} // namespace zabato