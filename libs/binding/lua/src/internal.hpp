#pragma once

#include <zabato/fs.hpp>
#include <zabato/lua/c.hpp>
#include <zabato/vector.hpp>

namespace zabato
{
inline static bool read_all(fs::file *f, vector<uint8_t> &buffer)
{
    size_t current = f->tell();
    f->seek(0, origin::end);
    size_t end = f->tell();
    f->seek(current, origin::begin);

    size_t size = end - current;
    buffer.resize(size);

    size_t read_count = f->read(buffer);
    if (read_count != size)
        buffer.resize(read_count);

    return read_count > 0;
}

inline int readonly_Error(lua_State *L)
{
    return luaL_error(L, "attempt to modify a read-only library");
}

/**
 * @brief Protect a metatable at the top of the stack.
 * Prevents getmetatable/setmetatable modification.
 *
 * @param L The Lua state.
 */
inline void protect_metatable(lua_State *L)
{
    lua_pushliteral(L, "__metatable");
    lua_setfield(L, -2, "__metatable");
}

/**
 * @brief Wraps the table at top of stack in a Read-Only Proxy.
 * Replaces the table on the stack.
 */
inline void make_library_readonly(lua_State *L)
{
    /* Stack: [LibTable] */
    lua_newtable(L); /* Stack: [LibTable, Proxy] */
    lua_newtable(L); /* Stack: [LibTable, Proxy, Meta] */

    lua_pushvalue(L, -3); /* Stack: [LibTable, Proxy, Meta, LibTable] */
    lua_setfield(L, -2, "__index"); /* Meta.__index = LibTable */

    lua_pushcfunction(L, readonly_Error);
    lua_setfield(L, -2, "__newindex"); /* Meta.__newindex = Error */

    lua_pushliteral(L, "protected library");
    lua_setfield(L, -2, "__metatable"); /* Meta.__metatable = String */

    /* Stack: [LibTable, Proxy, Meta] */
    lua_setmetatable(L, -2); /* Proxy.__metatable = Meta */

    /* Replace original with proxy */
    lua_remove(L, -2); /* Stack: [Proxy] */
}
} // namespace zabato