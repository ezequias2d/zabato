#include "lua_value.hpp"

#include <ctime>

#include <zabato/console.hpp>
#include <zabato/fs.hpp>
#include <zabato/string.hpp>

namespace zabato
{
extern "C"
{

static int os_remove(lua_State *L)
{
    fs::file_system *fs = get_filesystem(L);
    if (!fs)
        return luaL_error(L, "file system not initialized");

    size_t len           = 0;
    const char *filename = luaL_checklstring(L, 1, &len);

    if (fs->remove({filename, len}))
    {
        lua_pushboolean(L, 1);
        return 1;
    }

    lua_pushnil(L);
    lua_pushstring(L, "unable to remove file");
    return 2;
}

static bool copy_file(fs::file_system *fs,
                      const string_view &src_path,
                      const string_view &dst_file)
{
    fs::file *f_src = fs->open(src_path, fs::open_mode::read);
    if (!f_src)
        return false;

    fs::file *f_dst = fs->open(dst_file, fs::open_mode::write);
    if (!f_dst)
    {
        f_src->close();
        delete f_dst;
        return false;
    }

    f_src->copy_to(f_dst);
    f_src->close();
    f_dst->close();
    delete f_src;
    delete f_dst;
    return true;
}

static bool move_item(fs::file_system *fs,
                      const string_view &src_path,
                      const string_view &dst_path);

static bool move_dir(fs::file_system *fs,
                     const string_view &src_dir,
                     const string_view &dst_dir)
{
    if (!fs->exists(dst_dir))
    {
        if (!fs->mkdir(dst_dir))
            return false;
    }
    else if (!fs->is_dir(dst_dir))
        return false;

    vector<fs::file_info> contents = fs->ls(src_dir);

    for (const auto &item : contents)
    {
        string sub_src = fs::join(src_dir, item.name);
        string sub_dst = fs::join(dst_dir, item.name);

        if (!move_item(fs, sub_src, sub_dst))
            return false;
    }

    return fs->remove(src_dir);
}

static bool move_item(fs::file_system *fs,
                      const string_view &src_path,
                      const string_view &dst_path)
{
    if (fs->is_dir(src_path))
        return move_dir(fs, src_path, dst_path);

    if (!copy_file(fs, src_path, dst_path))
        return false;
    return fs->remove(src_path);
}

static int os_rename(lua_State *L)
{
    fs::file_system *fs = get_filesystem(L);
    if (!fs)
        return luaL_error(L, "file system not initialized");

    size_t from_name_len  = 0;
    const char *from_name = luaL_checklstring(L, 1, &from_name_len);
    string_view from      = {from_name, from_name_len};

    size_t to_name_len  = 0;
    const char *to_name = luaL_checklstring(L, 2, &to_name_len);
    string_view to      = {to_name, to_name_len};

    if (!fs->exists(from))
    {
        lua_pushnil(L);
        lua_pushfstring(L, "%s: no such file or directory", from_name);
        return 2;
    }

    if (fs->is_read_only(from))
    {
        lua_pushnil(L);
        lua_pushfstring(L, "%s: source is read-only", from_name);
        return 2;
    }

    if (fs->exists(to) && fs->is_read_only(to))
    {
        lua_pushnil(L);
        lua_pushfstring(L, "%s: destination is read-only", to_name);
        return 2;
    }

    if (move_item(fs, from, to))
    {
        lua_pushboolean(L, 1);
        return 1;
    }

    lua_pushnil(L);
    lua_pushstring(L, "rename failed (copy/delete error)");
    return 2;
}

static int os_getenv(lua_State *L)
{
    lua_pushnil(L);
    return 1;
}

static int os_clock(lua_State *L)
{
    lua_Number rc = ((lua_Number)1.0) / (lua_Number)CLOCKS_PER_SEC;
    lua_pushnumber(L, (lua_Number)clock() * rc);
    return 1;
}

static int os_time(lua_State *L)
{
    time_t t = time(NULL);
    if (lua_isnoneornil(L, 1))
    {
        lua_pushinteger(L, (lua_Integer)t);
        return 1;
    }

    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);

    struct tm ts = {0};

    lua_getfield(L, 1, "sec");
    ts.tm_sec = (int)luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);

    lua_getfield(L, 1, "min");
    ts.tm_min = (int)luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);

    lua_getfield(L, 1, "hour");
    ts.tm_hour = (int)luaL_optinteger(L, -1, 12);
    lua_pop(L, 1);

    lua_getfield(L, 1, "day");
    ts.tm_mday = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "month");
    ts.tm_mon = (int)luaL_checkinteger(L, -1) - 1;
    lua_pop(L, 1);

    lua_getfield(L, 1, "year");
    ts.tm_year = (int)luaL_checkinteger(L, -1) - 1900;
    lua_pop(L, 1);

    lua_getfield(L, 1, "isdst");
    ts.tm_isdst = (int)luaL_optinteger(L, -1, -1);
    lua_pop(L, 1);

    lua_pushinteger(L, (lua_Integer)mktime(&ts));
    return 1;
}

static int os_difftime(lua_State *L)
{
    time_t t1 = (time_t)luaL_checkinteger(L, 1);
    time_t t2 = (time_t)luaL_checkinteger(L, 2);
    lua_pushnumber(L, (lua_Number)difftime(t1, t2));
    return 1;
}

static int os_date(lua_State *L)
{
    const char *s = luaL_optstring(L, 1, "%c");
    time_t t      = luaL_optinteger(L, 2, time(NULL));
    struct tm *stm;
    if (*s == '!')
    {
        stm = gmtime(&t);
        s++;
    }
    else
        stm = localtime(&t);

    if (strcmp(s, "*t") == 0)
    {
        lua_createtable(L, 0, 9);

        lua_pushinteger(L, stm->tm_sec);
        lua_setfield(L, -2, "sec");

        lua_pushinteger(L, stm->tm_min);
        lua_setfield(L, -2, "min");

        lua_pushinteger(L, stm->tm_hour);
        lua_setfield(L, -2, "hour");

        lua_pushinteger(L, stm->tm_mday);
        lua_setfield(L, -2, "day");

        lua_pushinteger(L, stm->tm_mon + 1);
        lua_setfield(L, -2, "month");

        lua_pushinteger(L, stm->tm_year + 1900);
        lua_setfield(L, -2, "year");

        lua_pushinteger(L, stm->tm_wday + 1);
        lua_setfield(L, -2, "wday");

        lua_pushinteger(L, stm->tm_yday + 1);
        lua_setfield(L, -2, "yday");

        lua_pushinteger(L, stm->tm_isdst);
        lua_setfield(L, -2, "isdst");

        return 1;
    }

    char cc[4];
    luaL_Buffer b;
    cc[0] = '%';
    luaL_buffinit(L, &b);

    while (*s)
    {
        if (*s != '%')
            luaL_addchar(&b, *s++);
        else
        {
            char buff[200];
            s++;
            cc[1]         = *s++;
            cc[2]         = '\0';
            size_t reslen = strftime(buff, sizeof(buff), cc, stm);
            luaL_addlstring(&b, buff, reslen);
        }
    }
    luaL_pushresult(&b);
    return 1;
}

static int os_exit(lua_State *L)
{
    bool is_success = true;
    if (lua_isboolean(L, 1))
        is_success = lua_toboolean(L, 1);
    else
        is_success = luaL_optinteger(L, 1, EXIT_SUCCESS) == EXIT_SUCCESS;

    if (lua_toboolean(L, 2))
        lua_close(L);

    if (is_success)
        return luaL_error(L, "os.exit() called (script terminated)");
    else
        return luaL_error(L, "os.exit() called with error code");
}

static int os_execute(lua_State *L)
{
    const char *cmd = luaL_optstring(L, 1, NULL);

    if (cmd == NULL)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    console *console = lua_get_console(L);
    if (console)
    {
        string msg = "Blocked unsafe os.execute: ";
        msg += string_view(cmd);
        console->log_warning(msg);
    }

    lua_pushnil(L);
    lua_pushliteral(L, "permission denied (sandboxed)");
    lua_pushinteger(L, 1);
    return 3;
}

static int os_setlocale(lua_State *L)
{
    const char *locale  = luaL_optstring(L, 1, NULL);
    const char *cat_str = luaL_optstring(L, 2, "all");

    int cat = LC_ALL;
    if (strcmp(cat_str, "all") == 0)
        cat = LC_ALL;
    else if (strcmp(cat_str, "numeric") == 0)
        cat = LC_NUMERIC;
    else if (strcmp(cat_str, "time") == 0)
        cat = LC_TIME;
    else if (strcmp(cat_str, "collate") == 0)
        cat = LC_COLLATE;
    else if (strcmp(cat_str, "monetary") == 0)
        cat = LC_MONETARY;
    else if (strcmp(cat_str, "ctype") == 0)
        cat = LC_CTYPE;

    if (locale == NULL)
        lua_pushstring(L, setlocale(cat, NULL));
    else
        lua_pushnil(L);

    return 1;
}

static int os_tmpname(lua_State *L)
{
    static bool seeded = false;
    if (!seeded)
    {
        srand((unsigned)time(NULL));
        seeded = true;
    }

    char buff[32];
    sprintf(buff, "/tmp/lua_%06x", rand() % 0xFFFFFF);

    lua_pushstring(L, buff);
    return 1;
}

int luaopen_zabato_os(lua_State *L)
{
    luaL_Reg syslib[] = {
        {"clock", os_clock},
        {"date", os_date},
        {"difftime", os_difftime},
        {"time", os_time},
        {"remove", os_remove},
        {"rename", os_rename},
        {"getenv", os_getenv},
        {"exit", NULL},
        {"execute", NULL},
        {"setlocale", NULL},
        {"tmpname", NULL},
        {NULL, NULL},
    };

    luaL_newlib(L, syslib);

    make_library_readonly(L);

    return 1;
}
}
} // namespace zabato