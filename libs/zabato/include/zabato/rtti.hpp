#pragma once

#include <zabato/string.hpp>
#include <zabato/symbol.hpp>

namespace zabato
{
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
    /**
     * @brief Construct a new RTTI object.
     * @param name The name of the type.
     * @param base_type Pointer to the RTTI of the base class, or nullptr if
     * this is a root class.
     */
    rtti(const char *name, const rtti *base_type)
    {
        m_name      = get_symbol(name);
        m_base_type = base_type;
    }

    ~rtti()
    {
        if (m_name)
            release_symbol(m_name);
    }

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

private:
    symbol *m_name;
    const rtti *m_base_type;
};
} // namespace zabato