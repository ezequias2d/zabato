#include <assert.h>
#include <string.h>
#include <zabato/hash_set.hpp>
#include <zabato/string.hpp>
#include <zabato/symbol.hpp>

namespace zabato
{
struct symbol
{
    uint32_t hash;
    size_t length;
    size_t ref_count;
    char chars[];
};

struct symbol_lookup_key
{
    const char *str;
    uint32_t hash;
};

struct symbol_hasher
{
    size_t operator()(const symbol *s) const { return s->hash; }
    size_t operator()(const symbol_lookup_key &lookup) const
    {
        return lookup.hash;
    }
};

struct symbol_key_equal
{
    bool operator()(const symbol *a, const symbol *b) const
    {
        if (a->hash != b->hash || a->length != b->length)
            return false;
        return memcmp(a->chars, b->chars, a->length) == 0;
    }

    bool operator()(const symbol *a, const symbol_lookup_key &lookup) const
    {
        if (a->hash != lookup.hash || a->length != strlen(lookup.str))
            return false;
        return memcmp(a->chars, lookup.str, a->length) == 0;
    }
};

static hash_set<symbol *, symbol_hasher, symbol_key_equal> g_symbol_table;

symbol *get_symbol(const char *name)
{
    if (name == nullptr)
        name = "";

    size_t length = strlen(name);

    hash<const char *> hasher;
    uint32_t hash = hasher(name, length);

    symbol_lookup_key lookup{name, hash};
    symbol *existing_symbol = nullptr;

    if (g_symbol_table.try_get(lookup, existing_symbol))
    {
        existing_symbol->ref_count++;
        return existing_symbol;
    }

    size_t alloc_size = sizeof(symbol) + length + 1;
    symbol *new_sym   = static_cast<symbol *>(malloc(alloc_size));
    if (!new_sym)
        return nullptr;

    new_sym->hash      = hash;
    new_sym->length    = length;
    new_sym->ref_count = 1;
    memcpy(new_sym->chars, name, length + 1);

    g_symbol_table.add(new_sym);
    return new_sym;
}

symbol *ref_symbol(symbol *s)
{
    if (s)
    {
        s->ref_count++;
    }
    return s;
}

void release_symbol(symbol *s)
{
    if (!s)
        return;

    s->ref_count--;
    if (s->ref_count == 0)
    {
        g_symbol_table.remove(s);
        free(s);
    }
}

const char *get_symbol_name(const symbol *s) { return s ? s->chars : ""; }

uint32_t get_symbol_hash(const symbol *s) { return s ? s->hash : 0; }

} // namespace zabato
