#pragma once

#include <zabato/lua/script_system.hpp>
#include <zabato/script.hpp>

namespace zabato
{
class lua_script_system;

class lua_script_args : public script_args
{
public:
    lua_State *L;
    int start_index;
    int num_args;
    int returns_count = 0;
    lua_script_system *sys;

    lua_script_args(lua_State *l, int start, int count, lua_script_system *s)
        : L(l), start_index(start), num_args(count), sys(s)
    {
    }

    int count() const override { return num_args; }

    script_value get_value(int index) override
    {
        return sys->to_value(start_index + index);
    }

    void error(const char *msg) override { luaL_error(L, "%s", msg); }

    void push_return(const script_value &v) override
    {
        sys->push_value(v);
        returns_count++;
    }

    void type_error(const char *msg) override
    {
        luaL_error(L, "TypeError: %s", msg);
    }
};
} // namespace zabato