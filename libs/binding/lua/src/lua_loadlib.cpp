#include "internal.hpp"
#include "lua_value.hpp"

#include <zabato/fs.hpp>

namespace zabato
{
extern "C"
{
static const char ZABATO_LOADED_TABLE[]  = "ZABATO_LOADED";
static const char ZABATO_PRELOAD_TABLE[] = "ZABATO_PRELOAD";
static const char DEFAULT_LUA_PATH[] =
    "./?.lua;./?/init.lua;embedded/scripts/?.lua;embedded/scripts/?/init.lua";

static int ll_require(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    lua_settop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, ZABATO_LOADED_TABLE);
    lua_getfield(L, 2, name); /* LOADED[name] */
    if (lua_toboolean(L, -1))
        return 1;  /* package is already loaded */
    lua_pop(L, 1); /* Remove 'getfield' result */

    /* Get Searchers */
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "searchers");
    if (!lua_istable(L, -1))
        return luaL_error(L, "'package.searchers' must be a table");

    int searchers_index = lua_gettop(L);

    luaL_Buffer msg;
    luaL_buffinit(L, &msg);
    luaL_addstring(&msg, "\n\t");

    int i = 1;
    while (true)
    {
        lua_rawgeti(L, searchers_index, i); /* Get searchers[i] */
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            luaL_buffsub(&msg, 2);
            luaL_pushresult(&msg);
            return luaL_error(
                L, "module '%s' not found:%s", name, lua_tostring(L, -1));
        }

        lua_pushstring(L, name);
        lua_call(L, 1, 2); /* call searcher(name) -> loader, data */

        if (lua_isfunction(L, -2))
            break;

        if (lua_isstring(L, -2))
        {
            lua_pop(L, 1);       /* pop 2nd result */
            luaL_addvalue(&msg); /* add error msg */
            luaL_addstring(&msg, "\n\t");
        }
        else
            lua_pop(L, 2); /* pop both results */
        i++;
    }

    /* Loader is at -2, data is at -1.*/
    /* Stack: [name, LOADED, package, searchers, loader, data] */
    lua_rotate(L, -2, 1); /* function <-> data */
    lua_pushvalue(L, 1);  /* name is 1st arg to loader */
    lua_pushvalue(L, -3); /* data is 2nd arg to loader */
    lua_call(L, 2, 1);    /* run loader */

    /* Store result */
    if (!lua_isnil(L, -1))
        lua_setfield(L, 2, name); /* LOADED[name] = returned value */
    else
    {
        lua_pop(L, 1);
        if (lua_getfield(L, 2, name) == LUA_TNIL) /* no value set? */
        {
            lua_pop(L, 1);
            lua_pushboolean(L, 1);
            lua_setfield(L, 2, name); /* LOADED[name] = true */
        }
        else
            lua_pop(L, 1); /* leave LOADED[name] on stack */
    }

    lua_getfield(L, 2, name);
    return 1;
}

static int searcher_preload(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, ZABATO_LOADED_TABLE);
    if (lua_getfield(L, -1, name) == LUA_TNIL)
    {
        lua_pushfstring(L, "no field package.preload['%s']", name);
        return 1;
    }
    lua_pushliteral(L, ":preload:");
    return 2;
}

static int loader(lua_State *L)
{
    fs::file_system *fs = get_filesystem(L);

    size_t modname_len  = 0;
    const char *modname = luaL_checklstring(L, 1, &modname_len);

    size_t path_len  = 0;
    const char *path = luaL_checklstring(L, 2, &path_len);

    fs::file *file = fs->open({path, path_len}, fs::open_mode::read);
    if (!file)
    {
        return luaL_error(
            L, "error loading module '%s' from '%s'", modname, path);
    }

    vector<uint8_t> buffer;
    if (!read_all(file, buffer))
    {
        file->close();
        delete file;
        return luaL_error(L, "empty module '%s'", modname);
    }
    file->close();
    delete file;

    string debug = "@";
    debug += string_view{path, path_len};

    if (luaL_loadbufferx(L,
                         (const char *)buffer.data(),
                         buffer.size(),
                         debug.c_str(),
                         "t") != LUA_OK)
        return lua_error(L);

    // Call the module
    lua_pushvalue(L, 1); // name
    lua_pushvalue(L, 2); // path
    lua_call(L, 2, 1);   // returns result

    return 1;
}

static string apply_template(const string_view &tpl, const string_view &sub)
{
    string res = tpl;
    size_t pos = res.find('?');
    if (pos != string::npos)
        res.replace(pos, 1, sub);
    return res;
}

static int searcher_lua(lua_State *L)
{
    fs::file_system *fs = get_filesystem(L);

    /* Get package.path from upvalue 1 */
    lua_getfield(L, lua_upvalueindex(1), "path");
    size_t path_template_len  = 0;
    const char *path_template = lua_tolstring(L, -1, &path_template_len);
    if (!path_template)
    {
        lua_pop(L, 1);
        return luaL_error(L, "'package.path' must be a string");
    }

    size_t len       = 0;
    const char *name = luaL_checklstring(L, 1, &len);

    // convert module name "foo.bar" => "foo/bar"
    string path = {name, len};
    for (size_t i = 0; i < len; ++i)
        if (path[i] == '.')
            path[i] = '/';

    string_view templates = {path_template, path_template_len};
    string error_msg;

    size_t start = 0;
    size_t end   = templates.find(';');

    while (end != string::npos || start < templates.length())
    {
        string_view tpl = templates.substr(start, end - start);
        if (!tpl.empty())
        {
            string full_path = apply_template(tpl, path);
            if (fs->exists((full_path)))
            {
                lua_pushcfunction(L, loader);
                lua_pushlstring(L, full_path.data(), full_path.size());
                return 2;
            }

            error_msg += "\n\tno file '";
            error_msg += full_path;
            error_msg += "' in file_system";
        }

        if (end == string::npos)
            break;

        start = end + 1;
        end   = templates.find(';', start);
    }

    lua_pushlstring(L, error_msg.data(), error_msg.size());
    return 1;
}

static void create_registry_table(lua_State *L, const char *name)
{
    lua_getfield(L, LUA_REGISTRYINDEX, name);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);                            /* pop nill */
        lua_newtable(L);                          /* create table */
        lua_pushvalue(L, -1);                     /* dup */
        lua_setfield(L, LUA_REGISTRYINDEX, name); /* registry[name] = table */
    }
    /* leave table on stack */
}

static const lua_CFunction searchers[] = {
    searcher_preload,
    searcher_lua,
};
static const size_t searchers_count = sizeof(searchers) / sizeof(searchers[0]);

static void create_searchers_table(lua_State *L)
{
    /* Creates table at top of stack */
    lua_createtable(L, (int)searchers_count, 0);

    for (size_t i = 0; i < searchers_count; ++i)
    {
        /* Push 'package' table as upvalue */
        lua_pushvalue(L, -2);

        /* create closure with 1 upvalue */
        lua_pushcclosure(L, searchers[i], 1);

        /* store in table: searchers[i+1] = closure*/
        lua_rawseti(L, -2, (int)i + 1);
    }
    lua_setfield(L, -2, "searchers");
}

int luaopen_zabato_package(lua_State *L)
{
    create_registry_table(L, ZABATO_LOADED_TABLE);
    lua_setfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_pop(L, 1);

    create_registry_table(L, ZABATO_PRELOAD_TABLE);
    lua_setfield(L, LUA_REGISTRYINDEX, "_PRELOAD");
    lua_pop(L, 1);

    luaL_Reg pk_funcs[] = {
        {"require", ll_require},
        {NULL, NULL},
    };
    luaL_newlib(L, pk_funcs);

    // Setup package fields
    lua_getfield(L, LUA_REGISTRYINDEX, ZABATO_LOADED_TABLE);
    lua_setfield(L, -2, "loaded");

    lua_getfield(L, LUA_REGISTRYINDEX, ZABATO_PRELOAD_TABLE);
    lua_setfield(L, -2, "preload");

    lua_pushstring(L, DEFAULT_LUA_PATH);
    lua_setfield(L, -2, "path");

    // Create searchers table
    create_searchers_table(L);

    // Register globals
    lua_pushglobaltable(L);

    lua_pushcfunction(L, ll_require);
    lua_setfield(L, -2, "require");

    lua_pop(L, 1); // Pop global table

    return 1; // Return 'package' table
}
} // extern "C"
} // namespace zabato