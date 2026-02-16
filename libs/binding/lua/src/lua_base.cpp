#include "internal.hpp"
#include "lua_value.hpp"

#include <zabato/lua/openlibs.hpp>

namespace zabato
{
extern "C"
{

static int l_print(lua_State *L)
{
    console *c = lua_get_console(L);
    if (!c)
        return luaL_error(L, "console not found");

    int n = lua_gettop(L);
    lua_getglobal(L, "tostring");

    string buffer = "";
    for (int i = 1; i <= n; i++)
    {
        const char *s = nullptr;
        size_t len    = 0;

        lua_pushvalue(L, -1); /* push function to string */
        lua_pushvalue(L, i);  /* push arg value */
        lua_call(L, 1, 1);    /* call tostring(arg) */

        s = lua_tolstring(L, -1, &len);
        if (s == NULL)
            return luaL_error(L, "'tostring' must return a string to 'print'");

        if (i > 1)
            buffer += "\t";

        buffer += string_view{s, len};
        lua_pop(L, 1);
    }

    c->log_info(buffer);
    return 0;
}

static void l_warning_hook(void *ud, const char *msg, int tocont)
{
    console *c = (console *)ud;
    if (c)
        c->log_warning(msg);
}

static int l_warn(lua_State *L)
{
    console *c = lua_get_console(L);
    if (!c)
        return 0;

    int n = lua_gettop(L);

    luaL_checkstring(L, 1);
    for (int i = 2; i <= n; ++i)
        luaL_checkstring(L, i);

    string buffer = "";
    for (int i = 1; i <= n; ++i)
    {
        size_t len    = 0;
        const char *s = lua_tolstring(L, i, &len);
        buffer += string_view{s, len};
    }

    c->log_warning(buffer);
    return 0;
}

static int aux_load(lua_State *L,
                    fs::file_system *fs,
                    const string &path,
                    const char *mode)
{
    fs::file *file = fs->open(path, fs::open_mode::read);
    if (!file)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "cannot open %s", path.c_str());
        return 2;
    }

    vector<uint8_t> buffer;
    read_all(file, buffer);
    file->close();
    delete file;

    string debug = "@";
    debug += path;
    if (luaL_loadbufferx(L,
                         (const char *)buffer.data(),
                         buffer.size(),
                         debug.c_str(),
                         mode) != LUA_OK)
    {
        lua_pushnil(L);
        lua_insert(L, -2);
        return 2;
    }

    return 1;
}

static int l_load(lua_State *L)
{
    if (!lua_isstring(L, 1))
        return luaL_error(L, "argument must be a string");

    size_t len            = 0;
    const char *s         = lua_tolstring(L, 1, &len);
    const char *chunkname = luaL_optstring(L, 2, s);
    const char *mode      = luaL_optstring(L, 3, "t");

    if (strchr(mode, 'b') != nullptr)
        return luaL_error(L, "attempt to load binary chunk");

    /* load the chunk */
    int status = luaL_loadbufferx(L, s, len, chunkname, "t");
    if (status != LUA_OK)
    {
        lua_pushnil(L);
        lua_insert(L, -2); /* put nil before error message */
        return 2;          /* return nil, error message */
    }

    /* set environment (optional 4th argument) */
    if (!lua_isnone(L, 4))
    {
        lua_pushvalue(L, 4);
        if (!lua_setupvalue(L, -2, 1)) // Set _ENV (upvalue 1)
            lua_pop(L, 1);
    }

    return 1; /* return the compiled chunk */
}

static int l_loadfile(lua_State *L)
{
    fs::file_system *fs = get_filesystem(L);

    size_t len       = 0;
    const char *name = luaL_checklstring(L, 1, &len);
    string path      = {name, len};

    return aux_load(L, fs, path, NULL);
}

static int l_dofile(lua_State *L)
{
    fs::file_system *fs = get_filesystem(L);

    const char *name = luaL_optstring(L, 1, NULL);
    string path      = {name};

    int top     = lua_gettop(L);
    int results = aux_load(L, fs, name ? name : "stdin", "t");
    if (results == 2 && lua_isnil(L, -2))
        return lua_error(L);

    lua_call(L, 0, LUA_MULTRET);
    return lua_gettop(L) - top;
}

int luaopen_zabato_base(lua_State *L)
{
    luaopen_base(L);
    lua_settop(L, 0);

    console *c = lua_get_console(L);
    if (c)
        lua_setwarnf(L, l_warning_hook, c);

    lua_pushglobaltable(L);

    lua_pushcfunction(L, l_print);
    lua_setfield(L, -2, "print");

    lua_pushcfunction(L, l_warn);
    lua_setfield(L, -2, "warn");

    lua_pushcfunction(L, l_load);
    lua_setfield(L, -2, "load");

    lua_pushcfunction(L, l_loadfile);
    lua_setfield(L, -2, "loadfile");

    lua_pushcfunction(L, l_dofile);
    lua_setfield(L, -2, "dofile");

    lua_pop(L, 1);

    return 1;
}
}
} // namespace zabato