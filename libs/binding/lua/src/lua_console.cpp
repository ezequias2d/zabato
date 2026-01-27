#include "lua_value.hpp"

namespace zabato
{
extern "C"
{
static string lua_args_to_string(lua_State *L)
{
    int n = lua_gettop(L);
    lua_getglobal(L, "tostring");

    string buffer = "";

    for (int i = 1; i <= n; ++i)
    {
        const char *s = nullptr;
        size_t len    = 0;

        lua_pushvalue(L, -1); /* push function 'tostring' */
        lua_pushvalue(L, i);  /* push arg[i] */
        lua_call(L, 1, 1);    /* call tostring(arg[i]) */

        s = lua_tolstring(L, -1, &len);
        if (s == NULL)
            luaL_error(L, "'tostring' must return a string to console.log");

        if (i > 1)
            buffer += '\t';

        buffer += string_view(s, len);

        lua_pop(L, 1); /* pop result of tostring */
    }

    lua_pop(L, 1); /* pop function 'tostring' */

    return buffer;
}

static int l_console_log(lua_State *L)
{
    string msg       = lua_args_to_string(L);
    console *console = lua_get_console(L);
    if (!console)
        return luaL_error(L, "console not initialized");
    console->log(log_level::info, msg);
    return 0;
}

static int l_console_warn(lua_State *L)
{
    string msg       = lua_args_to_string(L);
    console *console = lua_get_console(L);
    if (!console)
        return luaL_error(L, "console not initialized");
    console->log(log_level::warning, msg);
    return 0;
}

static int l_console_error(lua_State *L)
{
    string msg       = lua_args_to_string(L);
    console *console = lua_get_console(L);
    if (!console)
        return luaL_error(L, "console not initialized");
    console->log(log_level::error, msg);
    return 0;
}

static int l_console_debug(lua_State *L)
{
    string msg       = lua_args_to_string(L);
    console *console = lua_get_console(L);
    if (!console)
        return luaL_error(L, "console not initialized");
    console->log(log_level::debug, msg);
    return 0;
}

static int l_console_success(lua_State *L)
{
    string msg       = lua_args_to_string(L);
    console *console = lua_get_console(L);
    if (!console)
        return luaL_error(L, "console not initialized");
    console->log(log_level::success, msg);
    return 0;
}

static int l_console_clear(lua_State *L)
{
    console *console = lua_get_console(L);
    if (!console)
        return luaL_error(L, "console not initialized");
    console->clear();
    return 0;
}

int luaopen_zabato_console(lua_State *L)
{
    luaL_Reg lib[] = {
        {"log", l_console_log},
        {"warn", l_console_warn},
        {"error", l_console_error},
        {"debug", l_console_debug},
        {"success", l_console_success},
        {"clear", l_console_clear},
        {NULL, NULL},
    };

    luaL_newlib(L, lib);
    make_library_readonly(L);
    return 1;
}
}

} // namespace zabato