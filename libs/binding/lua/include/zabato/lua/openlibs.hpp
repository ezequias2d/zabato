#pragma once

#include <zabato/lua/c.hpp>

namespace zabato
{
extern "C"
{
int luaopen_zabato_base(lua_State *L);
int luaopen_zabato_io(lua_State *L);
int luaopen_zabato_console(lua_State *L);
int luaopen_zabato_package(lua_State *L);
int luaopen_zabato_os(lua_State *L);
int luaopen_zabato_math(lua_State *L);
}
} // namespace zabato