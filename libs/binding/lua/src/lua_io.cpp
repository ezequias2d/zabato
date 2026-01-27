#include "internal.hpp"
#include "lua_value.hpp"

#include <zabato/fs.hpp>

namespace zabato
{

extern "C"
{

static const char ZABATO_FILE[]      = "ZABATO_FILE";
static const char ZABATO_IO_INPUT[]  = "ZABATO_IO_INPUT";
static const char ZABATO_IO_OUTPUT[] = "ZABATO_IO_OUTPUT";

fs::file *get_io_file(lua_State *L, const char *registry_key)
{
    lua_getfield(L, LUA_REGISTRYINDEX, registry_key);
    fs::file **f = (fs::file **)luaL_testudata(L, -1, ZABATO_FILE);
    if (!f || (*f)->is_closed())
        luaL_error(L, "standard file is closed or invalid");
    return *f;
}

static fs::open_mode parse_mode(const char *mode_str)
{
    if (!mode_str || *mode_str == '\0')
        return fs::open_mode::read;

    fs::open_mode flags = static_cast<fs::open_mode>(0);

    char main_mode = mode_str[0];

    switch (main_mode)
    {
    case 'r':
        flags = fs::open_mode::read;
        break;
    case 'w':
        flags = fs::open_mode::write | fs::open_mode::create |
                fs::open_mode::truncate;
    case 'a':
        flags = fs::open_mode::write | fs::open_mode::create |
                fs::open_mode::append;
        break;
    }

    if (strchr(mode_str, '+'))
    {
        // "r+": read + write
        // "w+": read + write + create + truncate
        // "a+": read + write + create + append
        flags = flags | (fs::open_mode::read | fs::open_mode::write);
    }

    return flags;
}

static fs::file *check_file(lua_State *L, int idx = 1)
{
    fs::file **f = (fs::file **)luaL_testudata(L, idx, ZABATO_FILE);
    if (!f || !(*f) || (*f)->is_closed())
        luaL_error(L, "file is closed");
    return *f;
}

// Mode 0: Strip EOL (*l)
// Mode 1: Keep EOL (*L)
static bool read_line(fs::file *f, vector<uint8_t> &line, int mode)
{
    line.clear();
    uint8_t c     = 0;
    bool read_any = false;

    while (f->read({&c, 1}) == 1)
    {
        read_any = true;

        if (c == '\n')
        {
            if (mode == 1)
                line.push_back(c);
            return true;
        }

        if (c == '\r')
        {
            size_t pos     = f->tell();
            uint8_t next_c = 0;

            if (f->read({&next_c, 1}) == 1)
            {
                if (next_c == '\n')
                {
                    if (mode == 1)
                    {
                        line.push_back('\r');
                        line.push_back('\n');
                    }
                    return true;
                }
                f->seek(pos, origin::begin);
            }
            if (mode == 1)
                line.push_back(c);
            continue;
        }

        line.push_back(c);
    }

    return read_any;
}

static bool read_number(fs::file *f, vector<uint8_t> &out)
{
    out.clear();
    uint8_t c;

    // Skip leading whitespaces
    while (true)
    {
        if (f->read({&c, 1}) != 1)
            return false;

        if (!isspace((unsigned char)c))
            break;
    }

    out.push_back(c);

    if (out.empty())
        return false;

    const char *valid = "0123456789abcdefABCDEFxXpP.+-";
    while (f->read({&c, 1}) == 1)
    {
        if (!strchr(valid, c))
        {
            f->seek(-1, origin::current);
            break;
        }
        out.push_back(c);
    }

    return true;
}

static int generic_read(lua_State *L, fs::file *f, int start_arg)
{
    int nargs = lua_gettop(L) - start_arg;
    if (nargs == 0)
    {
        lua_pushstring(L, "*l");
        nargs = 1;
    }

    vector<uint8_t> buffer;

    for (int i = 0; i < nargs; ++i)
    {
        int idx = start_arg + 1 + i;

        if (lua_type(L, idx) == LUA_TNUMBER)
        {
            // Read bytes
            size_t bytes = (size_t)lua_tointeger(L, idx);
            if (bytes == 0)
                lua_pushstring(L, "");
            else
            {
                buffer.resize(bytes);
                size_t r = f->read(buffer);
                if (r == 0)
                    lua_pushnil(L);
                else
                    lua_pushlstring(L, (const char *)buffer.data(), r);
            }
        }
        else
        {
            // Read format
            size_t len      = 0;
            const char *fmt = luaL_checklstring(L, idx, &len);

            if (fmt[0] == '*')
                fmt++;

            if (fmt[0 == 'n']) // Number
            {
                if (read_number(f, buffer))
                {
                    lua_pushlstring(
                        L, (const char *)buffer.data(), buffer.size());
                    if (!lua_stringtonumber(L, lua_tostring(L, -1)))
                    {
                        lua_pop(L, 1);
                        lua_pushnil(L);
                    }
                }
                else
                    lua_pushnil(L);
            }
            else if (fmt[0] == 'a') // All
            {
                if (read_all(f, buffer))
                    lua_pushlstring(
                        L, (const char *)buffer.data(), buffer.size());
                else
                    lua_pushstring(L, "");
            }
            else if (fmt[0] == 'l' || fmt[0] == 'L') // Line
            {
                if (read_line(f, buffer, (fmt[0] == 'L')))
                    lua_pushlstring(
                        L, (const char *)buffer.data(), buffer.size());
                else
                    lua_pushnil(L);
            }
            else
                return luaL_argerror(L, idx, "invalid format");
        }
    }
    return nargs;
}

static int f_close(lua_State *L)
{
    fs::file **ff = (fs::file **)luaL_checkudata(L, 1, ZABATO_FILE);
    if (!ff)
        return luaL_error(L, "not a file");
    fs::file *f = *ff;
    if (f)
    {
        *ff = nullptr;
        f->close();
        delete f;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int f_flush(lua_State *L)
{
    fs::file *f = check_file(L, 1);
    f->flush();
    lua_pushboolean(L, 1);
    return 1;
}

static int f_read(lua_State *L)
{
    fs::file *f = check_file(L, 1);
    return generic_read(L, f, 1);
}

static int f_write(lua_State *L)
{
    fs::file *f = check_file(L, 1);
    int top     = lua_gettop(L);
    for (int i = 2; i <= top; i++)
    {
        size_t len    = 0;
        const char *s = luaL_checklstring(L, i, &len);
        f->write({(const uint8_t *)s, len});
    }
    lua_pushvalue(L, 1);
    return 1;
}

static int f_seek(lua_State *L)
{
    fs::file *f                       = check_file(L, 1);
    const char *const modes[]         = {"set", "cur", "end", NULL};
    static const fs::origin origins[] = {
        origin::begin, origin::current, origin::end};

    int op        = luaL_checkoption(L, 2, "cur", modes);
    size_t offset = (size_t)luaL_optinteger(L, 3, 0);

    f->seek(offset, origins[op]);
    lua_pushinteger(L, (lua_Integer)f->tell());
    return 1;
}

static int io_open(lua_State *L)
{
    fs::file_system *fs = get_filesystem(L);
    if (!fs)
        luaL_error(L, "file system not initialized");

    size_t len       = 0;
    const char *path = luaL_checklstring(L, 1, &len);
    const char *mode = luaL_optstring(L, 2, "r");

    fs::open_mode flags = parse_mode(mode);

    auto *file_obj = fs->open(path, flags);
    if (!file_obj)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "cannot open '%s'", path);
        lua_pushinteger(L, 1);
        return 3;
    }

    fs::file **f = (fs::file **)lua_newuserdata(L, sizeof(fs::file));
    *f           = file_obj;
    luaL_setmetatable(L, ZABATO_FILE);
    return 1;
}

// io.input / io.output
static int io_std(lua_State *L, const char *key, const char *mode)
{
    if (lua_gettop(L) == 0)
    {
        // Get
        lua_getfield(L, LUA_REGISTRYINDEX, key);
        return 1;
    }

    // Set
    if (lua_type(L, 1) == LUA_TSTRING)
    {
        fs::file_system *fs = get_filesystem(L);
        if (!fs)
            luaL_error(L, "file system not initialized");

        size_t len       = 0;
        const char *path = lua_tolstring(L, 1, &len);

        fs::open_mode flags = parse_mode(mode);

        fs::file *file = fs->open({path, len}, flags);
        if (!file)
            luaL_error(L, "cannot open '%s'", path);

        fs::file **f = (fs::file **)lua_newuserdata(L, sizeof(fs::file));
        *f           = file;
        luaL_setmetatable(L, ZABATO_FILE);
        return 1;
    }
    else
    {
        luaL_checkudata(L, 1, ZABATO_FILE);
        lua_pushvalue(L, 1);
    }
    lua_setfield(L, LUA_REGISTRYINDEX, key);
    return 0;
}

static int io_input(lua_State *L) { return io_std(L, ZABATO_IO_INPUT, "r"); }

static int io_output(lua_State *L) { return io_std(L, ZABATO_IO_OUTPUT, "w"); }

static int io_read(lua_State *L)
{
    fs::file *f = get_io_file(L, ZABATO_IO_INPUT);
    lua_pop(L, 1); // pop userdata
    return generic_read(L, f, 0);
}

static int io_write(lua_State *L)
{
    fs::file *f = get_io_file(L, ZABATO_IO_OUTPUT);

    int top = lua_gettop(L) - 1;
    for (int i = 1; i <= top; ++i)
    {
        size_t len    = 0;
        const char *s = luaL_checklstring(L, i, &len);
        f->write({(const uint8_t *)s, len});
    }

    return 1;
}

static int lines_iter(lua_State *L)
{
    fs::file **ff =
        (fs::file **)luaL_checkudata(L, lua_upvalueindex(1), ZABATO_FILE);
    fs::file *f = *ff;
    bool close  = lua_toboolean(L, lua_upvalueindex(2));
    vector<uint8_t> line;
    if (read_line(f, line, 0))
    {
        lua_pushlstring(L, (const char *)line.data(), line.size());
        return 1;
    }

    if (close && !(*ff))
    {
        (*ff)->close();
        delete *ff;
        *ff = nullptr;
    }
    return 0;
}

static int io_type(lua_State *L)
{
    luaL_checkany(L, 1);
    fs::file **ff = (fs::file **)luaL_testudata(L, 1, ZABATO_FILE);
    if (!ff)
        lua_pushnil(L);
    else if (!(*ff) || (*ff)->is_closed())
        lua_pushstring(L, "closed file");
    else
        lua_pushstring(L, "file");
    return 1;
}

static int io_lines(lua_State *L)
{
    if (lua_gettop(L) == 0)
    {
        get_io_file(L, ZABATO_IO_INPUT);
        lua_pushboolean(L, 0); // don't close standard input
        lua_pushcclosure(L, lines_iter, 2);
    }
    else
    {
        fs::file_system *fs = get_filesystem(L);

        size_t len       = 0;
        const char *path = luaL_checklstring(L, 1, &len);

        fs::file *file = fs->open({path, len}, fs::open_mode::read);

        fs::file **ff = (fs::file **)lua_newuserdata(L, sizeof(fs::file *));
        *ff           = file;
        luaL_setmetatable(L, ZABATO_FILE);
        lua_pushboolean(L, 1); // Auto-close
        lua_pushcclosure(L, lines_iter, 2);
    }
    return 1;
}

static int f_lines(lua_State *L)
{
    check_file(L, 1);
    lua_pushvalue(L, 1);
    lua_pushboolean(L, 0);
    lua_pushcclosure(L, lines_iter, 2);
    return 1;
}

int luaopen_zabato_io(lua_State *L)
{
    luaL_newmetatable(L, ZABATO_FILE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    protect_metatable(L);

    luaL_Reg methods[] = {
        {"close", f_close},
        {"flush", f_flush},
        {"read", f_read},
        {"write", f_write},
        {"seek", f_seek},
        {"lines", f_lines},
        {NULL, NULL},
    };
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);

    luaL_Reg libs[] = {
        {"open", io_open},
        {"input", io_input},
        {"output", io_output},
        {"read", io_read},
        {"write", io_write},
        {"lines", io_lines},
        {"type", io_type},
        {NULL, NULL},
    };
    luaL_newlib(L, libs); // Stack: [io_table]

    make_library_readonly(L); // Stack: [io_proxy]

    return 1;
}
}
} // namespace zabato