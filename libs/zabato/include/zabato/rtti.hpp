#pragma once

#include <zabato/string.hpp>
#include <zabato/symbol.hpp>

namespace zabato
{
class reflection;

/**
 * @brief Run-Time Type Information (RTTI) system for the Cryolite engine.
 *
 * Provides a mechanism to store and query type information at runtime,
 * supporting single inheritance hierarchies. It allows for type comparison and
 * derivation checks.
 */
class rtti
{
public:
    using reflect_cb = void (*)(reflection &);

    /**
     * @brief Construct a new RTTI object.
     * @param name The name of the type.
     * @param base_type Pointer to the RTTI of the base class, or nullptr if
     * this is a root class.
     * @param setup Callback to populate reflection data.
     */
    rtti(const char *name, const rtti *base_type, reflect_cb setup = nullptr)
        : m_reflection(nullptr), m_setup(setup)
    {
        m_name      = get_symbol(name);
        m_base_type = base_type;
    }

    rtti(const rtti &)            = delete;
    rtti &operator=(const rtti &) = delete;

    ~rtti();

    /**
     * @brief Get the name of the type.
     * @return The type name.
     */
    const char *name() const
    {
        if (m_name)
            return get_symbol_name(m_name);
        return "";
    }

    /**
     * @brief Get the base type RTTI.
     * @return Pointer to base type RTTI or nullptr.
     */
    const rtti *base() const { return m_base_type; }

    /**
     * @brief Check if this type is exactly the same as another type.
     * @param type The type to compare with.
     * @return true if the types are identical (address comparison).
     */
    bool is_exactly(const rtti &type) const { return &type == this; }

    /**
     * @brief Check if this type is derived from another type.
     * @param type The base type to check against.
     * @return true if this type is derived from (or is exactly) type.
     */
    bool is_derived(const rtti &type) const
    {
        const rtti *search = this;
        while (search)
        {
            if (search == &type)
                return true;
            search = search->m_base_type;
        }
        return false;
    }

    reflection &ensure_reflection() const;

    const reflection *get_reflection() const { return m_reflection; }

private:
    symbol *m_name;
    const rtti *m_base_type;
    mutable reflection *m_reflection;
    reflect_cb m_setup;
};
} // namespace zabato