#pragma once

#include <stddef.h>
#include <stdint.h>

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

} // namespace zabato
