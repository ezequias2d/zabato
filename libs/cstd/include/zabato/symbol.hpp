#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace zabato
{

/**
 * @struct symbol
 * @brief An opaque handle to an interned string.
 *
 * Symbols provide a fast way to compare strings by comparing their pointers
 * and are managed by a global, reference-counted symbol table.
 */
struct symbol;

/**
 * @brief Gets a unique, interned symbol for a given string.
 * If the symbol already exists, its reference count is incremented.
 * If it does not exist, a new symbol is created with a reference count of 1.
 * @param name The null-terminated string to get a symbol for.
 * @return A pointer to the unique symbol.
 */
symbol *get_symbol(const char *name);

/**
 * @brief Increments the reference count of a symbol.
 * @param s The symbol to reference.
 * @return The same symbol pointer passed in.
 */
symbol *ref_symbol(symbol *s);

/**
 * @brief Decrements the reference count of a symbol.
 * If the reference count reaches zero, the symbol is freed from the table.
 * @param s The symbol to release.
 */
void release_symbol(symbol *s);

/**
 * @brief Gets the string representation of a symbol.
 * @param s The symbol to query.
 * @return A const pointer to the null-terminated string.
 */
const char *get_symbol_name(const symbol *s);

/**
 * @brief Gets the pre-calculated hash of a symbol.
 * @param s The symbol to query.
 * @return The 32-bit hash value of the symbol's string.
 */
uint32_t get_symbol_hash(const symbol *s);

/**
 * @struct symbol_ref
 * @brief A wrapper for symbol pointers that handles reference counting.
 */
struct symbol_ref
{
    symbol *s = nullptr;

    symbol_ref() = default;

    symbol_ref(const char *name) : s(get_symbol(name)) {}

    symbol_ref(symbol *sym) : s(ref_symbol(sym)) {}

    symbol_ref(const symbol_ref &other) : s(ref_symbol(other.s)) {}

    symbol_ref(symbol_ref &&other) noexcept : s(other.s) { other.s = nullptr; }

    ~symbol_ref() { release_symbol(s); }

    symbol_ref &operator=(const symbol_ref &other)
    {
        if (this != &other)
        {
            release_symbol(s);
            s = ref_symbol(other.s);
        }
        return *this;
    }

    symbol_ref &operator=(symbol_ref &&other) noexcept
    {
        if (this != &other)
        {
            release_symbol(s);
            s       = other.s;
            other.s = nullptr;
        }
        return *this;
    }

    symbol *get() const { return s; }

    const char *c_str() const { return get_symbol_name(s); }

    bool empty() const { return s == nullptr || get_symbol_name(s)[0] == '\0'; }

    operator const char *() const { return c_str(); }

    operator symbol *() const { return s; }

    bool operator==(const symbol_ref &other) const { return s == other.s; }
    bool operator!=(const symbol_ref &other) const { return s != other.s; }
    bool operator==(const char *str) const { return strcmp(c_str(), str) == 0; }
    bool operator!=(const char *str) const { return strcmp(c_str(), str) != 0; }
};

} // namespace zabato
