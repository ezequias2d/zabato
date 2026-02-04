#include "internal.hpp"
#include "lauxlib.h"
#include "lua.h"
#include "lua_value.hpp"

#include <new>

#include <zabato/lua/c.hpp>
#include <zabato/math.hpp>

namespace zabato
{

extern "C"
{

static real check_real(lua_State *L, int arg, real def = 0)
{
    if (lua_gettop(L) >= arg)
        return (real)luaL_checknumber(L, arg);
    return def;
}

// -----------------------------------------------------------------------------
// VEC2
// -----------------------------------------------------------------------------

static int aux_vec2_new(lua_State *L, const vec2<real> &v)
{
    void *u = lua_newuserdata(L, sizeof(vec2<real>));
    new (u) vec2<real>(v);
    luaL_getmetatable(L, META_VEC2);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vec2_new(lua_State *L)
{
    int top = lua_gettop(L);
    real x;
    real y;
    if (top <= 1)
        x = y = 0;
    else if (top == 2)
        x = y = check_real(L, 2);
    else if (top >= 3)
    {
        x = check_real(L, 2);
        y = check_real(L, 3);
    }
    return aux_vec2_new(L, vec2<real>(x, y));
}

static int l_vec2_tostring(lua_State *L)
{
    auto *v = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    lua_pushfstring(L, "vec2(%f, %f)", (double)v->x, (double)v->y);
    return 1;
}

static int l_vec2_index(lua_State *L)
{
    auto *v         = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    const char *key = luaL_checkstring(L, 2);

    const bool x0 = key[0] == 'x';
    const bool y0 = key[0] == 'y';
    const bool x1 = key[1] == 'x';
    const bool y1 = key[1] == 'y';
    const bool e1 = key[1] == '\0';
    const bool e2 = key[2] == '\0';

    const bool single = (x0 || y0) && e1;
    const bool pair   = (x0 || y0) && (x1 || y1) && e2;

    if (single)
    {
        if (x0)
        {
            lua_pushnumber(L, (double)v->x);
            return 1;
        }

        if (y0)
        {
            lua_pushnumber(L, (double)v->x);
            return 1;
        }
    }
    else if (pair)
    {
        const real x = x0 ? v->x : v->y;
        const real y = y1 ? v->y : v->x;
        return aux_vec2_new(L, vec2<real>(x, y));
    }

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);
    return 1;
}

static int l_vec2_newindex(lua_State *L)
{
    auto *v         = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    const char *key = luaL_checkstring(L, 2);

    const bool x0 = key[0] == 'x';
    const bool y0 = key[0] == 'y';
    const bool x1 = key[1] == 'x';
    const bool y1 = key[1] == 'y';
    const bool e1 = key[1] == '\0';
    const bool e2 = key[2] == '\0';

    const bool single = (x0 || y0) && e1;
    const bool pair   = ((x0 && y1) || (y0 && x1)) && e2;

    if (single)
    {
        real val = check_real(L, 3);
        if (x0)
        {
            v->x = val;
            return 0;
        }
        else if (y0)
        {
            v->y = val;
            return 0;
        }
    }
    else if (pair)
    {
        vec2<real> *val = (vec2<real> *)luaL_checkudata(L, 3, META_VEC2);
        if (x0 && y1)
        {
            v->x = val->x;
            v->y = val->y;
            return 0;
        }

        if (y0 && x1)
        {
            v->x = val->y;
            v->y = val->x;
            return 0;
        }
    }

    return 0;
}

static int l_vec2_add(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    auto *b = (vec2<real> *)luaL_checkudata(L, 2, META_VEC2);
    return aux_vec2_new(L, *a + *b);
}

static int l_vec2_sub(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    auto *b = (vec2<real> *)luaL_checkudata(L, 2, META_VEC2);
    return aux_vec2_new(L, *a - *b);
}

static int l_vec2_mul(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    if (lua_isnumber(L, 2))
    {
        real b = (real)lua_tonumber(L, 2);
        return aux_vec2_new(L, (*a) * b);
    }

    auto *b = (vec2<real> *)luaL_checkudata(L, 2, META_VEC2);
    return aux_vec2_new(L, (*a) * (*b));
}

static int l_vec2_div(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    if (lua_isnumber(L, 2))
    {
        real b = (real)lua_tonumber(L, 2);
        return aux_vec2_new(L, (*a) / b);
    }

    auto *b = (vec2<real> *)luaL_checkudata(L, 2, META_VEC2);
    return aux_vec2_new(L, (*a) / (*b));
}

static int l_vec2_unm(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    return aux_vec2_new(L, -(*a));
}

static int l_vec2_eq(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    auto *b = (vec2<real> *)luaL_checkudata(L, 2, META_VEC2);
    lua_pushboolean(L, *a == *b);
    return 1;
}

static int l_vec2_dot(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    auto *b = (vec2<real> *)luaL_checkudata(L, 2, META_VEC2);
    lua_pushnumber(L, (double)dot(*a, *b));
    return 1;
}

static int l_vec2_length(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    lua_pushnumber(L, (double)length(*a));
    return 1;
}

static int l_vec2_normalize(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    return aux_vec2_new(L, normalize(*a));
}

static int l_vec2_distance(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    auto *b = (vec2<real> *)luaL_checkudata(L, 2, META_VEC2);
    lua_pushnumber(L, (double)distance(*a, *b));
    return 1;
}

// -----------------------------------------------------------------------------
// VEC3
// -----------------------------------------------------------------------------

static int aux_vec3_new(lua_State *L, const vec3<real> &v)
{
    void *u = lua_newuserdata(L, sizeof(vec3<real>));
    new (u) vec3<real>(v);
    luaL_getmetatable(L, META_VEC3);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vec3_new(lua_State *L)
{
    int top = lua_gettop(L);
    real x;
    real y;
    real z;

    if (top <= 1)
        x = y = z = 0;
    else if (top == 2)
        x = y = z = check_real(L, 2);
    else if (top == 3)
    {
        x = check_real(L, 2);
        y = check_real(L, 3);
        z = 0;
    }
    else if (top >= 4)
    {
        x = check_real(L, 2);
        y = check_real(L, 3);
        z = check_real(L, 4);
    }
    return aux_vec3_new(L, vec3<real>(x, y, z));
}

static int l_vec3_tostring(lua_State *L)
{
    auto *v = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    lua_pushfstring(
        L, "vec3(%f, %f, %f)", (double)v->x, (double)v->y, (double)v->z);
    return 1;
}

static int l_vec3_index(lua_State *L)
{
    auto *v         = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    const char *key = luaL_checkstring(L, 2);

    const bool x0 = key[0] == 'x' || key[0] == 'r';
    const bool y0 = key[0] == 'y' || key[0] == 'g';
    const bool z0 = key[0] == 'z' || key[0] == 'b';
    const bool x1 = key[1] == 'x' || key[1] == 'r';
    const bool y1 = key[1] == 'y' || key[1] == 'g';
    const bool z1 = key[1] == 'z' || key[1] == 'b';
    const bool x2 = key[2] == 'x' || key[2] == 'r';
    const bool y2 = key[2] == 'y' || key[2] == 'g';
    const bool z2 = key[2] == 'z' || key[2] == 'b';
    const bool e1 = key[1] == '\0';
    const bool e2 = key[2] == '\0';
    const bool e3 = key[3] == '\0';

    const bool single = (x0 || y0 || z0) && e1;

    if (single)
    {
        double val = 0.0;
        if (x0)
            val = (double)v->x;
        if (y0)
            val = (double)v->y;
        if (z0)
            val = (double)v->z;
        lua_pushnumber(L, val);
        return 1;
    }

    const bool pair = (x0 || y0 || z0) && (x1 || y1 || z1) && e2;

    if (pair)
    {
        vec2<real> val;

        if (x0)
            val.x = (real)v->x;
        if (y0)
            val.x = (real)v->y;
        if (z0)
            val.x = (real)v->z;

        if (x1)
            val.y = (real)v->x;
        if (y1)
            val.y = (real)v->y;
        if (z1)
            val.y = (real)v->z;
        return aux_vec2_new(L, val);
    }

    const bool triple =
        (x0 || y0 || z0) && (x1 || y1 || z1) && (x2 || y2 || z2) && e3;

    if (triple)
    {
        vec3<real> val;

        if (x0)
            val.x = (real)v->x;
        else if (y0)
            val.x = (real)v->y;
        else if (z0)
            val.x = (real)v->z;

        if (x1)
            val.y = (real)v->x;
        else if (y1)
            val.y = (real)v->y;
        else if (z1)
            val.y = (real)v->z;

        if (x2)
            val.z = (real)v->x;
        else if (y2)
            val.z = (real)v->y;
        else if (z2)
            val.z = (real)v->z;

        return aux_vec3_new(L, val);
    }

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);
    return 1;
}

static int l_vec3_newindex(lua_State *L)
{
    auto *v         = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    const char *key = luaL_checkstring(L, 2);

    const bool x0 = key[0] == 'x' || key[0] == 'r';
    const bool y0 = key[0] == 'y' || key[0] == 'g';
    const bool z0 = key[0] == 'z' || key[0] == 'b';
    const bool x1 = key[1] == 'x' || key[1] == 'r';
    const bool y1 = key[1] == 'y' || key[1] == 'g';
    const bool z1 = key[1] == 'z' || key[1] == 'b';
    const bool x2 = key[2] == 'x' || key[2] == 'r';
    const bool y2 = key[2] == 'y' || key[2] == 'g';
    const bool z2 = key[2] == 'z' || key[2] == 'b';
    const bool e1 = key[1] == '\0';
    const bool e2 = key[2] == '\0';
    const bool e3 = key[3] == '\0';

    const bool single = (x0 || y0 || z0) && e1;
    if (single)
    {
        real val = (real)luaL_checknumber(L, 3);
        if (x0)
            v->x = val;
        else if (y0)
            v->y = val;
        else if (z0)
            v->z = val;
        return 0;
    }

    const bool xn = (x0 + x1 + x2) == 1;
    const bool yn = (y0 + y1 + y2) == 1;
    const bool zn = (z0 + z1 + z2) == 1;
    const int n   = xn + yn + zn;

    const bool pair = n == 2 && e2;

    if (pair)
    {
        vec2<real> val;

        if (lua_isnumber(L, 3))
        {
            val = vec2<real>(luaL_checknumber(L, 3));
        }
        else
        {
            auto *a = (vec2<real> *)luaL_checkudata(L, 3, META_VEC2);
            if (!a)
                luaL_error(L, "Invalid vec2");
            val = *a;
        }

        if (x0)
            v->x = val.x;
        else if (y0)
            v->y = val.x;
        else if (z0)
            v->z = val.x;

        if (x1)
            v->x = val.y;
        else if (y1)
            v->y = val.y;
        else if (z1)
            v->z = val.y;

        return 0;
    }

    const bool triple = n == 3 && e3;
    if (triple)
    {
        vec3<real> val;

        if (lua_isnumber(L, 3))
        {
            val = vec3<real>(luaL_checknumber(L, 3));
        }
        else
        {
            vec2<real> *a = (vec2<real> *)luaL_checkudata(L, 3, META_VEC2);
            if (a)
                val = vec3<real>(a->x, a->y, 0);
            else
            {
                vec3<real> *b = (vec3<real> *)luaL_checkudata(L, 3, META_VEC3);
                if (!b)
                    luaL_error(L, "Invalid vec3");
                val = *b;
            }
        }

        if (x0)
            v->x = val.x;
        else if (y0)
            v->y = val.x;
        else if (z0)
            v->z = val.x;

        if (x1)
            v->x = val.y;
        else if (y1)
            v->y = val.y;
        else if (z1)
            v->z = val.y;

        if (x2)
            v->x = val.z;
        else if (y2)
            v->y = val.z;
        else if (z2)
            v->z = val.z;

        return 0;
    }

    return 0;
}

static int l_vec3_add(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    return aux_vec3_new(L, (*a) + (*b));
}

static int l_vec3_sub(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    return aux_vec3_new(L, (*a) - (*b));
}

static int l_vec3_mul(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    if (lua_isnumber(L, 2))
    {
        real b = (real)lua_tonumber(L, 2);
        return aux_vec3_new(L, (*a) * b);
    }
    auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    return aux_vec3_new(L, (*a) * (*b));
}

static int l_vec3_div(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    if (lua_isnumber(L, 2))
    {
        real b = (real)lua_tonumber(L, 2);
        return aux_vec3_new(L, (*a) / b);
    }
    auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    return aux_vec3_new(L, (*a) / (*b));
}

static int l_vec3_unm(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    return aux_vec3_new(L, -(*a));
}

static int l_vec3_eq(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    lua_pushboolean(L, (*a) == (*b));
    return 1;
}

static int l_vec3_dot(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    lua_pushnumber(L, (double)dot(*a, *b));
    return 1;
}

static int l_vec3_cross(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    return aux_vec3_new(L, cross(*a, *b));
}

static int l_vec3_length(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    lua_pushnumber(L, (double)length(*a));
    return 1;
}

static int l_vec3_normalize(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    return aux_vec3_new(L, normalize(*a));
}

static int l_vec3_distance(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    lua_pushnumber(L, (double)distance(*a, *b));
    return 1;
}

static int l_vec3_lerp(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    real t  = (real)luaL_checknumber(L, 3);
    return aux_vec3_new(L, lerp(*a, *b, t));
}

// -----------------------------------------------------------------------------
// VEC4
// -----------------------------------------------------------------------------

static int aux_vec4_new(lua_State *L, const vec4<real> &v)
{
    void *u = lua_newuserdata(L, sizeof(vec4<real>));
    new (u) vec4<real>(v);
    luaL_getmetatable(L, META_VEC4);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vec4_new(lua_State *L)
{
    int top = lua_gettop(L);
    real x;
    real y;
    real z;
    real w;
    if (top <= 1)
        x = y = z = w = 0;
    else if (top == 2)
        x = y = z = w = check_real(L, 2);
    else if (top == 3)
    {
        x = check_real(L, 2);
        y = check_real(L, 3);
        z = w = 0;
    }
    else if (top == 4)
    {
        x = check_real(L, 2);
        y = check_real(L, 3);
        z = check_real(L, 4);
        w = 0;
    }
    else if (top >= 5)
    {
        x = check_real(L, 2);
        y = check_real(L, 3);
        z = check_real(L, 4);
        w = check_real(L, 5);
    }

    return aux_vec4_new(L, vec4<real>(x, y, z, w));
}

static int l_vec4_tostring(lua_State *L)
{
    auto *v = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    lua_pushfstring(L,
                    "vec4(%f, %f, %f, %f)",
                    (double)v->x,
                    (double)v->y,
                    (double)v->z,
                    (double)v->w);
    return 1;
}

static int l_vec4_index(lua_State *L)
{
    auto *v         = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    const char *key = luaL_checkstring(L, 2);

    const bool x0 = key[0] == 'x' || key[0] == 'r';
    const bool y0 = key[0] == 'y' || key[0] == 'g';
    const bool z0 = key[0] == 'z' || key[0] == 'b';
    const bool w0 = key[0] == 'w' || key[0] == 'a';
    const bool x1 = key[1] == 'x' || key[1] == 'r';
    const bool y1 = key[1] == 'y' || key[1] == 'g';
    const bool z1 = key[1] == 'z' || key[1] == 'b';
    const bool w1 = key[1] == 'w' || key[1] == 'a';
    const bool x2 = key[2] == 'x' || key[2] == 'r';
    const bool y2 = key[2] == 'y' || key[2] == 'g';
    const bool z2 = key[2] == 'z' || key[2] == 'b';
    const bool w2 = key[2] == 'w' || key[2] == 'a';
    const bool x3 = key[3] == 'x' || key[3] == 'r';
    const bool y3 = key[3] == 'y' || key[3] == 'g';
    const bool z3 = key[3] == 'z' || key[3] == 'b';
    const bool w3 = key[3] == 'w' || key[3] == 'a';
    const bool e1 = key[1] == '\0';
    const bool e2 = key[2] == '\0';
    const bool e3 = key[3] == '\0';
    const bool e4 = key[4] == '\0';

    const bool single = (x0 || y0 || z0 || w0) && e1;
    if (single)
    {
        double val = 0.0;
        if (x0)
            val = (double)v->x;
        else if (y0)
            val = (double)v->y;
        else if (z0)
            val = (double)v->z;
        else if (w0)
            val = (double)v->w;
        lua_pushnumber(L, val);
        return 1;
    }

    const bool pair = (x0 || y0 || z0 || w0) && (x1 || y1 || z1 || w1) && e2;
    if (pair)
    {
        vec2<real> val = {0, 0};
        if (x0)
            val.x = v->x;
        else if (y0)
            val.x = v->y;
        else if (z0)
            val.x = v->z;
        else if (w0)
            val.x = v->w;

        if (x1)
            val.y = v->x;
        else if (y1)
            val.y = v->y;
        else if (z1)
            val.y = v->z;
        else if (w1)
            val.y = v->w;

        return aux_vec2_new(L, val);
    }

    const bool triple = (x0 || y0 || z0 || w0) && (x1 || y1 || z1 || w1) &&
                        (x2 || y2 || z2 || w2) && e3;
    if (triple)
    {
        vec3<real> val = {0, 0, 0};
        if (x0)
            val.x = v->x;
        else if (y0)
            val.x = v->y;
        else if (z0)
            val.x = v->z;
        else if (w0)
            val.x = v->w;

        if (x1)
            val.y = v->x;
        else if (y1)
            val.y = v->y;
        else if (z1)
            val.y = v->z;
        else if (w1)
            val.y = v->w;

        if (x2)
            val.z = v->x;
        else if (y2)
            val.z = v->y;
        else if (z2)
            val.z = v->z;
        else if (w2)
            val.z = v->w;

        return aux_vec3_new(L, val);
    }

    const bool quad = (x0 || y0 || z0 || w0) && (x1 || y1 || z1 || w1) &&
                      (x2 || y2 || z2 || w2) && (x3 || y3 || z3 || w3) && e4;
    if (quad)
    {
        vec4<real> val = {0, 0, 0, 0};
        if (x0)
            val.x = v->x;
        else if (y0)
            val.x = v->y;
        else if (z0)
            val.x = v->z;
        else if (w0)
            val.x = v->w;

        if (x1)
            val.y = v->x;
        else if (y1)
            val.y = v->y;
        else if (z1)
            val.y = v->z;
        else if (w1)
            val.y = v->w;

        if (x2)
            val.z = v->x;
        else if (y2)
            val.z = v->y;
        else if (z2)
            val.z = v->z;
        else if (w2)
            val.z = v->w;

        if (x3)
            val.w = v->x;
        else if (y3)
            val.w = v->y;
        else if (z3)
            val.w = v->z;
        else if (w3)
            val.w = v->w;

        return aux_vec4_new(L, val);
    }

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);
    return 1;
}

static int l_vec4_newindex(lua_State *L)
{
    auto *v         = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    const char *key = luaL_checkstring(L, 2);

    const bool x0 = key[0] == 'x' || key[0] == 'r';
    const bool y0 = key[0] == 'y' || key[0] == 'g';
    const bool z0 = key[0] == 'z' || key[0] == 'b';
    const bool w0 = key[0] == 'w' || key[0] == 'a';
    const bool x1 = key[1] == 'x' || key[1] == 'r';
    const bool y1 = key[1] == 'y' || key[1] == 'g';
    const bool z1 = key[1] == 'z' || key[1] == 'b';
    const bool w1 = key[1] == 'w' || key[1] == 'a';
    const bool x2 = key[2] == 'x' || key[2] == 'r';
    const bool y2 = key[2] == 'y' || key[2] == 'g';
    const bool z2 = key[2] == 'z' || key[2] == 'b';
    const bool w2 = key[2] == 'w' || key[2] == 'a';
    const bool x3 = key[3] == 'x' || key[3] == 'r';
    const bool y3 = key[3] == 'y' || key[3] == 'g';
    const bool z3 = key[3] == 'z' || key[3] == 'b';
    const bool w3 = key[3] == 'w' || key[3] == 'a';
    const bool e1 = key[1] == '\0';
    const bool e2 = key[2] == '\0';
    const bool e3 = key[3] == '\0';
    const bool e4 = key[4] == '\0';

    const bool single = (x0 || y0 || z0 || w0) && e1;
    if (single)
    {
        real val = (real)luaL_checknumber(L, 3);
        if (x0)
            v->x = val;
        else if (y0)
            v->y = val;
        else if (z0)
            v->z = val;
        else if (w0)
            v->w = val;
        return 0;
    }

    const bool xn = (x0 + x1 + x2 + x3) == 1;
    const bool yn = (y0 + y1 + y2 + y3) == 1;
    const bool zn = (z0 + z1 + z2 + z3) == 1;
    const bool wn = (w0 + w1 + w2 + w3) == 1;
    const int n   = xn + yn + zn + wn;

    const bool pair = n == 2 && e2;

    if (pair)
    {
        vec2<real> val;

        if (lua_isnumber(L, 3))
            val = vec2<real>(luaL_checknumber(L, 3));
        else
        {
            vec2<real> *a = (vec2<real> *)luaL_checkudata(L, 3, META_VEC2);
            if (!a)
                luaL_error(L, "Invalid vec2");
            val = *a;
        }

        if (x0)
            v->x = val.x;
        else if (y0)
            v->y = val.x;
        else if (z0)
            v->z = val.x;
        else if (w0)
            v->w = val.x;

        if (x1)
            v->x = val.y;
        else if (y1)
            v->y = val.y;
        else if (z1)
            v->z = val.y;
        else if (w1)
            v->w = val.y;

        return 0;
    }

    const bool triple = n == 3 && e3;
    if (triple)
    {
        vec3<real> val;

        if (lua_isnumber(L, 3))
            val = vec3<real>(luaL_checknumber(L, 3));
        else
        {
            vec2<real> *a = (vec2<real> *)luaL_checkudata(L, 3, META_VEC2);
            if (a)
                val = vec3<real>(a->x, a->y, 0);
            else
            {
                vec3<real> *b = (vec3<real> *)luaL_checkudata(L, 3, META_VEC3);
                if (!b)
                    luaL_error(L, "Invalid vec3");
                val = *b;
            }
        }

        if (x0)
            v->x = val.x;
        else if (y0)
            v->y = val.x;
        else if (z0)
            v->z = val.x;
        else if (w0)
            v->w = val.x;

        if (x1)
            v->x = val.y;
        else if (y1)
            v->y = val.y;
        else if (z1)
            v->z = val.y;
        else if (w1)
            v->w = val.y;

        if (x2)
            v->x = val.z;
        else if (y2)
            v->y = val.z;
        else if (z2)
            v->z = val.z;
        else if (w2)
            v->w = val.z;

        return 0;
    }

    const bool quad = n == 4 && e4;
    if (quad)
    {
        vec4<real> val;

        if (lua_isnumber(L, 3))
            val = vec4<real>(luaL_checknumber(L, 3));
        else
        {
            vec2<real> *a = (vec2<real> *)luaL_checkudata(L, 3, META_VEC2);
            if (a)
                val = vec4<real>(a->x, a->y, 0, 0);
            else
            {
                vec3<real> *b = (vec3<real> *)luaL_checkudata(L, 3, META_VEC3);
                if (b)
                    val = vec4<real>(b->x, b->y, b->z, 0);
                else
                {
                    vec4<real> *c =
                        (vec4<real> *)luaL_checkudata(L, 3, META_VEC4);
                    if (!c)
                        luaL_error(L, "Invalid vec4");
                    val = *c;
                }
            }
        }

        if (x0)
            v->x = val.x;
        else if (y0)
            v->y = val.x;
        else if (z0)
            v->z = val.x;
        else if (w0)
            v->w = val.x;

        if (x1)
            v->x = val.y;
        else if (y1)
            v->y = val.y;
        else if (z1)
            v->z = val.y;
        else if (w1)
            v->w = val.y;

        if (x2)
            v->x = val.z;
        else if (y2)
            v->y = val.z;
        else if (z2)
            v->z = val.z;
        else if (w2)
            v->w = val.z;

        if (x3)
            v->x = val.w;
        else if (y3)
            v->y = val.w;
        else if (z3)
            v->z = val.w;
        else if (w3)
            v->w = val.w;

        return 0;
    }

    return 0;
}

static int l_vec4_add(lua_State *L)
{
    vec4<real> *a = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    vec4<real> *b = (vec4<real> *)luaL_checkudata(L, 2, META_VEC4);
    return aux_vec4_new(L, *a + *b);
}

static int l_vec4_sub(lua_State *L)
{
    vec4<real> *a = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    vec4<real> *b = (vec4<real> *)luaL_checkudata(L, 2, META_VEC4);
    return aux_vec4_new(L, *a - *b);
}

static int l_vec4_mul(lua_State *L)
{
    vec4<real> *a = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    vec4<real> *b = (vec4<real> *)luaL_checkudata(L, 2, META_VEC4);
    return aux_vec4_new(L, *a * *b);
}

static int l_vec4_div(lua_State *L)
{
    vec4<real> *a = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    vec4<real> *b = (vec4<real> *)luaL_checkudata(L, 2, META_VEC4);
    return aux_vec4_new(L, *a / *b);
}

static int l_vec4_unm(lua_State *L)
{
    vec4<real> *a = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    return aux_vec4_new(L, -(*a));
}

static int l_vec4_eq(lua_State *L)
{
    vec4<real> *a = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    vec4<real> *b = (vec4<real> *)luaL_checkudata(L, 2, META_VEC4);
    lua_pushboolean(L, *a == *b);
    return 1;
}

static int l_vec4_dot(lua_State *L)
{
    vec4<real> *a = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    vec4<real> *b = (vec4<real> *)luaL_checkudata(L, 2, META_VEC4);
    lua_pushnumber(L, (double)dot(*a, *b));
    return 1;
}

static int l_vec4_length(lua_State *L)
{
    vec4<real> *a = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    lua_pushnumber(L, (double)length(*a));
    return 1;
}

static int l_vec4_normalize(lua_State *L)
{
    vec4<real> *a = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    return aux_vec4_new(L, normalize(*a));
}

static int l_vec4_distance(lua_State *L)
{
    vec4<real> *a = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    vec4<real> *b = (vec4<real> *)luaL_checkudata(L, 2, META_VEC4);
    lua_pushnumber(L, (double)distance(*a, *b));
    return 1;
}

static int l_vec4_lerp(lua_State *L)
{
    vec4<real> *a = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    vec4<real> *b = (vec4<real> *)luaL_checkudata(L, 2, META_VEC4);
    real t        = luaL_checknumber(L, 3);
    return aux_vec4_new(L, lerp(*a, *b, t));
}

// -----------------------------------------------------------------------------
// QUAT
// -----------------------------------------------------------------------------

static int aux_quat_new(lua_State *L, const quat<real> &val)
{
    void *u = lua_newuserdata(L, sizeof(quat<real>));
    new (u) quat<real>(val);
    luaL_getmetatable(L, META_QUAT);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_quat_new(lua_State *L)
{
    // Identity by default: w=1
    real w = 1, x = 0, y = 0, z = 0;
    if (lua_gettop(L) >= 5)
    {
        w = check_real(L, 2);
        x = check_real(L, 3);
        y = check_real(L, 4);
        z = check_real(L, 5);
    }

    return aux_quat_new(L, quat<real>(w, x, y, z));
}

static int l_quat_tostring(lua_State *L)
{
    auto *v = (quat<real> *)luaL_checkudata(L, 1, META_QUAT);
    lua_pushfstring(L,
                    "quat(%f, %f, %f, %f)",
                    (double)v->w,
                    (double)v->x,
                    (double)v->y,
                    (double)v->z);
    return 1;
}

static int l_quat_index(lua_State *L)
{
    auto *v         = (quat<real> *)luaL_checkudata(L, 1, META_QUAT);
    const char *key = luaL_checkstring(L, 2);

    const bool x0 = key[0] == 'x';
    const bool y0 = key[0] == 'y';
    const bool z0 = key[0] == 'z';
    const bool w0 = key[0] == 'w';
    const bool x1 = key[1] == 'x';
    const bool y1 = key[1] == 'y';
    const bool z1 = key[1] == 'z';
    const bool w1 = key[1] == 'w';
    const bool x2 = key[2] == 'x';
    const bool y2 = key[2] == 'y';
    const bool z2 = key[2] == 'z';
    const bool w2 = key[2] == 'w';
    const bool x3 = key[3] == 'x';
    const bool y3 = key[3] == 'y';
    const bool z3 = key[3] == 'z';
    const bool w3 = key[3] == 'w';
    const bool e1 = key[1] == '\0';
    const bool e2 = key[2] == '\0';
    const bool e3 = key[3] == '\0';
    const bool e4 = key[4] == '\0';

    const bool single = (x0 || y0 || z0 || w0) && e1;
    if (single)
    {
        double val = 0.0;
        if (x0)
            val = (double)v->x;
        else if (y0)
            val = (double)v->y;
        else if (z0)
            val = (double)v->z;
        else if (w0)
            val = (double)v->w;
        lua_pushnumber(L, val);
        return 1;
    }

    const bool pair = (x0 || y0 || z0 || w0) && (x1 || y1 || z1 || w1) && e2;
    if (pair)
    {
        vec2<real> val = {0, 0};
        if (x0)
            val.x = v->x;
        else if (y0)
            val.x = v->y;
        else if (z0)
            val.x = v->z;
        else if (w0)
            val.x = v->w;

        if (x1)
            val.y = v->x;
        else if (y1)
            val.y = v->y;
        else if (z1)
            val.y = v->z;
        else if (w1)
            val.y = v->w;

        return aux_vec2_new(L, val);
    }

    const bool triple = (x0 || y0 || z0 || w0) && (x1 || y1 || z1 || w1) &&
                        (x2 || y2 || z2 || w2) && e3;
    if (triple)
    {
        vec3<real> val = {0, 0, 0};
        if (x0)
            val.x = v->x;
        else if (y0)
            val.x = v->y;
        else if (z0)
            val.x = v->z;
        else if (w0)
            val.x = v->w;

        if (x1)
            val.y = v->x;
        else if (y1)
            val.y = v->y;
        else if (z1)
            val.y = v->z;
        else if (w1)
            val.y = v->w;

        if (x2)
            val.z = v->x;
        else if (y2)
            val.z = v->y;
        else if (z2)
            val.z = v->z;
        else if (w2)
            val.z = v->w;

        return aux_vec3_new(L, val);
    }

    const bool quad = (x0 || y0 || z0 || w0) && (x1 || y1 || z1 || w1) &&
                      (x2 || y2 || z2 || w2) && (x3 || y3 || z3 || w3) && e4;
    if (quad)
    {
        quat<real> val = {0, 0, 0, 0};
        if (x0)
            val.x = v->x;
        else if (y0)
            val.x = v->y;
        else if (z0)
            val.x = v->z;
        else if (w0)
            val.x = v->w;

        if (x1)
            val.y = v->x;
        else if (y1)
            val.y = v->y;
        else if (z1)
            val.y = v->z;
        else if (w1)
            val.y = v->w;

        if (x2)
            val.z = v->x;
        else if (y2)
            val.z = v->y;
        else if (z2)
            val.z = v->z;
        else if (w2)
            val.z = v->w;

        if (x3)
            val.w = v->x;
        else if (y3)
            val.w = v->y;
        else if (z3)
            val.w = v->z;
        else if (w3)
            val.w = v->w;

        return aux_quat_new(L, val);
    }

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);
    return 1;
}

static int l_quat_newindex(lua_State *L)
{
    auto *v         = (quat<real> *)luaL_checkudata(L, 1, META_QUAT);
    const char *key = luaL_checkstring(L, 2);

    const bool x0 = key[0] == 'x' || key[0] == 'r';
    const bool y0 = key[0] == 'y' || key[0] == 'g';
    const bool z0 = key[0] == 'z' || key[0] == 'b';
    const bool w0 = key[0] == 'w' || key[0] == 'a';
    const bool x1 = key[1] == 'x' || key[1] == 'r';
    const bool y1 = key[1] == 'y' || key[1] == 'g';
    const bool z1 = key[1] == 'z' || key[1] == 'b';
    const bool w1 = key[1] == 'w' || key[1] == 'a';
    const bool x2 = key[2] == 'x' || key[2] == 'r';
    const bool y2 = key[2] == 'y' || key[2] == 'g';
    const bool z2 = key[2] == 'z' || key[2] == 'b';
    const bool w2 = key[2] == 'w' || key[2] == 'a';
    const bool x3 = key[3] == 'x' || key[3] == 'r';
    const bool y3 = key[3] == 'y' || key[3] == 'g';
    const bool z3 = key[3] == 'z' || key[3] == 'b';
    const bool w3 = key[3] == 'w' || key[3] == 'a';
    const bool e1 = key[1] == '\0';
    const bool e2 = key[2] == '\0';
    const bool e3 = key[3] == '\0';
    const bool e4 = key[4] == '\0';

    const bool single = (x0 || y0 || z0 || w0) && e1;
    if (single)
    {
        real val = (real)luaL_checknumber(L, 3);
        if (x0)
            v->x = val;
        else if (y0)
            v->y = val;
        else if (z0)
            v->z = val;
        else if (w0)
            v->w = val;
        return 0;
    }

    const bool xn = (x0 + x1 + x2 + x3) == 1;
    const bool yn = (y0 + y1 + y2 + y3) == 1;
    const bool zn = (z0 + z1 + z2 + z3) == 1;
    const bool wn = (w0 + w1 + w2 + w3) == 1;
    const int n   = xn + yn + zn + wn;

    const bool pair = n == 2 && e2;

    if (pair)
    {
        vec2<real> val;

        if (lua_isnumber(L, 3))
            val = vec2<real>(luaL_checknumber(L, 3));
        else
        {
            vec2<real> *a = (vec2<real> *)luaL_checkudata(L, 3, META_VEC2);
            if (!a)
                luaL_error(L, "Invalid vec2");
            val = *a;
        }

        if (x0)
            v->x = val.x;
        else if (y0)
            v->y = val.x;
        else if (z0)
            v->z = val.x;
        else if (w0)
            v->w = val.x;

        if (x1)
            v->x = val.y;
        else if (y1)
            v->y = val.y;
        else if (z1)
            v->z = val.y;
        else if (w1)
            v->w = val.y;

        return 0;
    }

    const bool triple = n == 3 && e3;
    if (triple)
    {
        vec3<real> val;

        if (lua_isnumber(L, 3))
            val = vec3<real>(luaL_checknumber(L, 3));
        else
        {
            vec2<real> *a = (vec2<real> *)luaL_checkudata(L, 3, META_VEC2);
            if (a)
                val = vec3<real>(a->x, a->y, 0);
            else
            {
                vec3<real> *b = (vec3<real> *)luaL_checkudata(L, 3, META_VEC3);
                if (!b)
                    luaL_error(L, "Invalid vec3");
                val = *b;
            }
        }

        if (x0)
            v->x = val.x;
        else if (y0)
            v->y = val.x;
        else if (z0)
            v->z = val.x;
        else if (w0)
            v->w = val.x;

        if (x1)
            v->x = val.y;
        else if (y1)
            v->y = val.y;
        else if (z1)
            v->z = val.y;
        else if (w1)
            v->w = val.y;

        if (x2)
            v->x = val.z;
        else if (y2)
            v->y = val.z;
        else if (z2)
            v->z = val.z;
        else if (w2)
            v->w = val.z;

        return 0;
    }

    const bool quad = n == 4 && e4;
    if (quad)
    {
        vec4<real> val;

        if (lua_isnumber(L, 3))
            val = vec4<real>(luaL_checknumber(L, 3));
        else
        {
            vec2<real> *a = (vec2<real> *)luaL_checkudata(L, 3, META_VEC2);
            if (a)
                val = vec4<real>(a->x, a->y, 0, 0);
            else
            {
                vec3<real> *b = (vec3<real> *)luaL_checkudata(L, 3, META_VEC3);
                if (b)
                    val = vec4<real>(b->x, b->y, b->z, 0);
                else
                {
                    vec4<real> *c =
                        (vec4<real> *)luaL_checkudata(L, 3, META_VEC4);

                    if (c)
                        val = *c;
                    else
                    {
                        quat<real> *d =
                            (quat<real> *)luaL_checkudata(L, 3, META_QUAT);
                        if (d)
                            val = vec4<real>(d->x, d->y, d->z, d->w);
                        else
                            luaL_error(L, "Invalid quat");
                    }
                }
            }
        }

        if (x0)
            v->x = val.x;
        else if (y0)
            v->y = val.x;
        else if (z0)
            v->z = val.x;
        else if (w0)
            v->w = val.x;

        if (x1)
            v->x = val.y;
        else if (y1)
            v->y = val.y;
        else if (z1)
            v->z = val.y;
        else if (w1)
            v->w = val.y;

        if (x2)
            v->x = val.z;
        else if (y2)
            v->y = val.z;
        else if (z2)
            v->z = val.z;
        else if (w2)
            v->w = val.z;

        if (x3)
            v->x = val.w;
        else if (y3)
            v->y = val.w;
        else if (z3)
            v->z = val.w;
        else if (w3)
            v->w = val.w;

        return 0;
    }

    return 0;
}

static int l_quat_mul(lua_State *L)
{
    if (auto *a = (quat<real> *)luaL_testudata(L, 1, META_QUAT))
    {
        if (auto *b = (quat<real> *)luaL_testudata(L, 2, META_QUAT))
            return aux_quat_new(L, (*a) * (*b));
        else if (auto *b = (vec3<real> *)luaL_testudata(L, 2, META_VEC3))
            return aux_vec3_new(L, (*a) * (*b));
        else if (auto *b = (vec4<real> *)luaL_testudata(L, 2, META_VEC4))
            return aux_vec4_new(L, (*a) * (*b));
        else if (lua_isnumber(L, 2))
            return aux_quat_new(L, (*a) * luaL_checknumber(L, 2));
    }
    else if (auto *b = (quat<real> *)luaL_testudata(L, 2, META_QUAT))
    {
        if (auto *a = (vec3<real> *)luaL_testudata(L, 1, META_VEC3))
            return aux_vec3_new(L, (*b) * (*a));
        else if (auto *a = (vec4<real> *)luaL_testudata(L, 1, META_VEC4))
            return aux_vec4_new(L, (*b) * (*a));
        else if (lua_isnumber(L, 1))
            return aux_quat_new(L, (*b) * luaL_checknumber(L, 1));
    }

    return luaL_error(L, "Invalid operands for quat mul");
}

static int l_quat_slerp(lua_State *L)
{
    auto *a = (quat<real> *)luaL_checkudata(L, 1, META_QUAT);
    auto *b = (quat<real> *)luaL_checkudata(L, 2, META_QUAT);
    real t  = (real)luaL_checknumber(L, 3);
    return aux_quat_new(L, slerp(*a, *b, t));
}

// -----------------------------------------------------------------------------
// MAT3
// -----------------------------------------------------------------------------

static int aux_mat3_new(lua_State *L, const mat3<real> &m)
{
    void *u = lua_newuserdata(L, sizeof(mat3<real>));
    new (u) mat3<real>(m);
    luaL_getmetatable(L, META_MAT3);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat3_new(lua_State *L)
{
    int args = lua_gettop(L);
    if (args <= 1)
        return aux_mat3_new(L, mat3<real>());

    if (args == 2)
    {
        if (auto *a = (mat3<real> *)luaL_testudata(L, 2, META_MAT3))
            return aux_mat3_new(L, *a);
        else if (auto *a = (quat<real> *)luaL_testudata(L, 2, META_QUAT))
            return aux_mat3_new(L, mat3<real>(*a));
        else if (lua_isnumber(L, 2))
            return aux_mat3_new(L, (real)luaL_checknumber(L, 2));
    }
    else if (args == 4)
    {
        mat3<real> m;
        for (int i = 0; i < 3; i++)
            m[i] = (real)luaL_checknumber(L, i + 2);
        return aux_mat3_new(L, m);
    }

    return luaL_error(L, "Invalid mat3 constructor");
}

static int l_mat3_tostring(lua_State *L)
{
    auto *m = (mat3<real> *)luaL_checkudata(L, 1, META_MAT3);
    lua_pushfstring(L,
                    "mat3(%f, %f, %f, %f, %f, %f, %f, %f, %f)",
                    (double)m->columns[0][0],
                    (double)m->columns[0][1],
                    (double)m->columns[0][2],
                    (double)m->columns[1][0],
                    (double)m->columns[1][1],
                    (double)m->columns[1][2],
                    (double)m->columns[2][0],
                    (double)m->columns[2][1],
                    (double)m->columns[2][2]);
    return 1;
}

static int l_mat3_mul(lua_State *L)
{
    if (auto *a = (mat3<real> *)luaL_testudata(L, 1, META_MAT3))
    {
        if (auto *b = (mat3<real> *)luaL_testudata(L, 2, META_MAT3))
            return aux_mat3_new(L, (*a) * (*b));
        else if (auto *b = (vec3<real> *)luaL_testudata(L, 2, META_VEC3))
            return aux_vec3_new(L, (*a) * (*b));
        else if (lua_isnumber(L, 2))
            return aux_mat3_new(L, (*a) * (real)luaL_checknumber(L, 2));
    }
    else if (auto *b = (mat3<real> *)luaL_testudata(L, 2, META_MAT3))
    {
        if (auto *a = (vec3<real> *)luaL_testudata(L, 1, META_VEC3))
            return aux_vec3_new(L, (*b) * (*a));
        else if (lua_isnumber(L, 1))
            return aux_mat3_new(L, (*b) * (real)luaL_checknumber(L, 1));
    }

    return luaL_error(L, "Invalid operands for mat3 mul");
}

static int l_mat3_index(lua_State *L)
{
    auto *m   = (mat3<real> *)luaL_checkudata(L, 1, META_MAT3);
    int index = luaL_checkinteger(L, 2);
    if (index < 1 || index > 9)
        return luaL_error(L, "Invalid index for mat3");
    return aux_vec3_new(L, m->columns[(index - 1) / 3][(index - 1) % 3]);
}

static int l_mat3_newindex(lua_State *L)
{
    auto *m   = (mat3<real> *)luaL_checkudata(L, 1, META_MAT3);
    int index = luaL_checkinteger(L, 2);
    if (index < 1 || index > 9)
        return luaL_error(L, "Invalid index for mat3");
    m->columns[(index - 1) / 3][(index - 1) % 3] = (real)luaL_checknumber(L, 3);
    return 0;
}

static int l_mat3_identity(lua_State *L)
{
    return aux_mat3_new(L, mat3<real>::identity());
}

static int l_mat3_inverse(lua_State *L)
{
    auto *m = (mat3<real> *)luaL_checkudata(L, 1, META_MAT3);
    mat3<real> res;
    if (inverse(*m, res))
        return aux_mat3_new(L, res);
    return 0;
}

static int l_mat3_transpose(lua_State *L)
{
    auto *m = (mat3<real> *)luaL_checkudata(L, 1, META_MAT3);
    return aux_mat3_new(L, transpose(*m));
}

static int l_mat3_decompose(lua_State *L)
{
    auto *m = (mat3<real> *)luaL_checkudata(L, 1, META_MAT3);
    vec3<real> s;
    quat<real> r;
    mat3_decompose(*m, s, r);
    aux_vec3_new(L, s);
    aux_quat_new(L, r);
    return 2;
}

static int l_mat3_from_quat(lua_State *L)
{
    auto *q = (quat<real> *)luaL_checkudata(L, 1, META_QUAT);
    return aux_mat3_new(L, mat3_from_quat(*q));
}

// -----------------------------------------------------------------------------
// MAT4
// -----------------------------------------------------------------------------

static int aux_mat4_new(lua_State *L, const mat4<real> &m)
{
    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    new (u) mat4<real>(m);
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat4_new(lua_State *L)
{
    int args = lua_gettop(L);
    if (args <= 1)
        return aux_mat4_new(L, mat4<real>());
    else if (args == 2)
    {
        if (auto *a = (mat4<real> *)luaL_testudata(L, 2, META_MAT4))
            return aux_mat4_new(L, *a);
        else if (auto *a = (quat<real> *)luaL_testudata(L, 2, META_QUAT))
            return aux_mat4_new(L, mat4_from_quat(*a));
        else if (lua_isnumber(L, 2))
            return aux_mat4_new(L, (real)luaL_checknumber(L, 2));
    }
    else if (args == 17)
    {
        mat4<real> m;
        for (int i = 0; i < 16; i++)
            m[i] = (real)luaL_checknumber(L, i + 2);
        return aux_mat4_new(L, m);
    }

    return luaL_error(L, "Invalid mat4 constructor");
}

static int l_mat4_identity(lua_State *L)
{
    return aux_mat4_new(L, mat4<real>::identity());
}

static int l_mat4_translation(lua_State *L)
{
    auto *v = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    return aux_mat4_new(L, mat4_translation(*v));
}

static int l_mat4_scaling(lua_State *L)
{
    auto *v = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    return aux_mat4_new(L, mat4_scaling(*v));
}

static int l_mat4_rotation_x(lua_State *L)
{
    real a = (real)luaL_checknumber(L, 1);
    return aux_mat4_new(L, mat4_rotation_x(a));
}

static int l_mat4_rotation_y(lua_State *L)
{
    real a = (real)luaL_checknumber(L, 1);
    return aux_mat4_new(L, mat4_rotation_y(a));
}

static int l_mat4_rotation_z(lua_State *L)
{
    real a = (real)luaL_checknumber(L, 1);
    return aux_mat4_new(L, mat4_rotation_z(a));
}

static int l_mat4_tostring(lua_State *L)
{
    auto *m = (mat4<real> *)luaL_checkudata(L, 1, META_MAT4);
    lua_pushfstring(
        L,
        "mat4(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)",
        (double)m->m[0][0],
        (double)m->m[0][1],
        (double)m->m[0][2],
        (double)m->m[0][3],
        (double)m->m[1][0],
        (double)m->m[1][1],
        (double)m->m[1][2],
        (double)m->m[1][3],
        (double)m->m[2][0],
        (double)m->m[2][1],
        (double)m->m[2][2],
        (double)m->m[2][3],
        (double)m->m[3][0],
        (double)m->m[3][1],
        (double)m->m[3][2],
        (double)m->m[3][3]);
    return 1;
}

static int l_mat4_mul(lua_State *L)
{
    if (auto *a = (mat4<real> *)luaL_testudata(L, 1, META_MAT4))
    {
        if (auto *b = (mat4<real> *)luaL_testudata(L, 2, META_MAT4))
            return aux_mat4_new(L, (*a) * (*b));
        else if (auto *b = (vec4<real> *)luaL_testudata(L, 2, META_VEC4))
            return aux_vec4_new(L, (*a) * (*b));
        else if (lua_isnumber(L, 2))
            return aux_mat4_new(L, (*a) * (real)luaL_checknumber(L, 2));
    }
    else if (auto *b = (mat4<real> *)luaL_testudata(L, 2, META_MAT4))
    {
        if (auto *a = (vec4<real> *)luaL_testudata(L, 1, META_VEC4))
            return aux_vec4_new(L, (*b) * (*a));
        else if (lua_isnumber(L, 1))
            return aux_mat4_new(L, (real)luaL_checknumber(L, 1) * (*b));
    }

    return luaL_error(L, "Invalid operands for mat4 mul");
}

static int l_mat4_index(lua_State *L)
{
    auto *m   = (mat4<real> *)luaL_checkudata(L, 1, META_MAT4);
    int index = luaL_checkinteger(L, 2);
    if (index < 1 || index > 16)
        return luaL_error(L, "Invalid index for mat4");
    return aux_vec4_new(L, m->m[(index - 1) / 4][(index - 1) % 4]);
}

static int l_mat4_newindex(lua_State *L)
{
    auto *m   = (mat4<real> *)luaL_checkudata(L, 1, META_MAT4);
    int index = luaL_checkinteger(L, 2);
    if (index < 1 || index > 16)
        return luaL_error(L, "Invalid index for mat4");
    m->m[(index - 1) / 4][(index - 1) % 4] = (real)luaL_checknumber(L, 3);
    return 0;
}

static int l_mat4_inverse(lua_State *L)
{
    auto *m = (mat4<real> *)luaL_checkudata(L, 1, META_MAT4);

    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    mat4<real> res;
    if (inverse(*m, res))
    {
        new (u) mat4<real>(res);
        luaL_getmetatable(L, META_MAT4);
        lua_setmetatable(L, -2);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int l_mat4_transpose(lua_State *L)
{
    auto *m = (mat4<real> *)luaL_checkudata(L, 1, META_MAT4);
    return aux_mat4_new(L, transpose(*m));
}

static int l_mat4_look_at(lua_State *L)
{
    auto *eye    = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    auto *center = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    auto *up     = (vec3<real> *)luaL_checkudata(L, 3, META_VEC3);

    return aux_mat4_new(L, mat4_look_at(*eye, *center, *up));
}

static int l_mat4_ortho(lua_State *L)
{
    real left   = (real)luaL_checknumber(L, 1);
    real right  = (real)luaL_checknumber(L, 2);
    real bottom = (real)luaL_checknumber(L, 3);
    real top    = (real)luaL_checknumber(L, 4);
    real near   = (real)luaL_checknumber(L, 5);
    real far    = (real)luaL_checknumber(L, 6);

    return aux_mat4_new(L, mat4_ortho(left, right, bottom, top, near, far));
}

static int l_mat4_perspective(lua_State *L)
{
    real fov    = (real)luaL_checknumber(L, 1);
    real aspect = (real)luaL_checknumber(L, 2);
    real near   = (real)luaL_checknumber(L, 3);
    real far    = (real)luaL_checknumber(L, 4);

    return aux_mat4_new(L, mat4_perspective_fov(fov, aspect, near, far));
}

static int l_mat4_decompose(lua_State *L)
{
    auto *m = (mat4<real> *)luaL_checkudata(L, 1, META_MAT4);
    vec3<real> pos;
    vec3<real> scale;
    quat<real> rot;

    mat4_decompose(*m, pos, scale, rot);

    int result = 0;

    result += aux_vec3_new(L, pos);
    result += aux_vec3_new(L, scale);
    result += aux_quat_new(L, rot);

    return result;
}

static int l_mat4_from_quat(lua_State *L)
{
    auto *q = (quat<real> *)luaL_checkudata(L, 1, META_QUAT);
    return aux_mat4_new(L, mat4_from_quat(*q));
}

// Helper to register a constructor table with static methods
static void register_type_table(lua_State *L,
                                const char *name,
                                lua_CFunction constructor,
                                const vector<luaL_Reg> &static_funcs)
{
    lua_newtable(L);

    for (const auto &reg : static_funcs)
    {
        lua_pushcfunction(L, reg.func);
        lua_setfield(L, -2, reg.name);
    }

    lua_newtable(L);
    lua_pushcfunction(L, constructor);
    lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2);
    lua_setglobal(L, name);
}

int luaopen_zabato_math(lua_State *L)
{
    // --- VEC2 ---
    luaL_Reg vec2_funcs[] = {
        {"__tostring", l_vec2_tostring},
        {"__add", l_vec2_add},
        {"__sub", l_vec2_sub},
        {"__mul", l_vec2_mul},
        {"__div", l_vec2_div},
        {"__unm", l_vec2_unm},
        {"__eq", l_vec2_eq},
        {"__index", l_vec2_index},
        {"__newindex", l_vec2_newindex},
        {"dot", l_vec2_dot},
        {"length", l_vec2_length},
        {"normalize", l_vec2_normalize},
        {"distance", l_vec2_distance},
        {NULL, NULL},
    };

    if (luaL_newmetatable(L, META_VEC2))
    {
        luaL_setfuncs(L, vec2_funcs, 0);
        protect_metatable(L);
    }
    lua_pop(L, 1); // Pop metatable

    register_type_table(L,
                        "vec2",
                        l_vec2_new,
                        {
                            {"dot", l_vec2_dot},
                            {"length", l_vec2_length},
                            {"normalize", l_vec2_normalize},
                            {"distance", l_vec2_distance},
                        });

    // --- VEC3 ---
    luaL_Reg vec3_funcs[] = {
        {"__tostring", l_vec3_tostring},
        {"__add", l_vec3_add},
        {"__sub", l_vec3_sub},
        {"__mul", l_vec3_mul},
        {"__div", l_vec3_div},
        {"__unm", l_vec3_unm},
        {"__eq", l_vec3_eq},
        {"__index", l_vec3_index},
        {"__newindex", l_vec3_newindex},
        {"dot", l_vec3_dot},
        {"cross", l_vec3_cross},
        {"length", l_vec3_length},
        {"normalize", l_vec3_normalize},
        {"distance", l_vec3_distance},
        {"lerp", l_vec3_lerp},
        {NULL, NULL},
    };

    if (luaL_newmetatable(L, META_VEC3))
    {
        luaL_setfuncs(L, vec3_funcs, 0);
        protect_metatable(L);
    }
    lua_pop(L, 1); // Pop metatable

    register_type_table(L,
                        "vec3",
                        l_vec3_new,
                        {
                            {"dot", l_vec3_dot},
                            {"cross", l_vec3_cross},
                            {"length", l_vec3_length},
                            {"normalize", l_vec3_normalize},
                            {"distance", l_vec3_distance},
                            {"lerp", l_vec3_lerp},
                        });

    // VEC4 Instance Attributes
    luaL_Reg vec4_funcs[] = {
        {"__tostring", l_vec4_tostring},
        {"__add", l_vec4_add},
        {"__sub", l_vec4_sub},
        {"__mul", l_vec4_mul},
        {"__div", l_vec4_div},
        {"__unm", l_vec4_unm},
        {"__eq", l_vec4_eq},
        {"__index", l_vec4_index},
        {"__newindex", l_vec4_newindex},
        {"dot", l_vec4_dot},
        {"length", l_vec4_length},
        {"normalize", l_vec4_normalize},
        {"distance", l_vec4_distance},
        {"lerp", l_vec4_lerp},
        {NULL, NULL},
    };

    if (luaL_newmetatable(L, META_VEC4))
    {
        luaL_setfuncs(L, vec4_funcs, 0);
        protect_metatable(L);
    }
    lua_pop(L, 1); // Pop metatable

    register_type_table(L,
                        "vec4",
                        l_vec4_new,
                        {
                            {"dot", l_vec4_dot},
                            {"length", l_vec4_length},
                            {"normalize", l_vec4_normalize},
                            {"distance", l_vec4_distance},
                            {"lerp", l_vec4_lerp},
                        });

    // --- QUAT ---
    luaL_Reg quat_funcs[] = {
        {"__tostring", l_quat_tostring},
        {"__mul", l_quat_mul},
        {"__index", l_quat_index},
        {"__newindex", l_quat_newindex},
        {"slerp", l_quat_slerp},
        {NULL, NULL},
    };

    if (luaL_newmetatable(L, META_QUAT))
    {
        luaL_setfuncs(L, quat_funcs, 0);
        protect_metatable(L);
    }
    lua_pop(L, 1); // Pop metatable

    register_type_table(L,
                        "quat",
                        l_quat_new,
                        {
                            {"slerp", l_quat_slerp},
                        });

    // --- MAT3 ---
    luaL_Reg mat3_funcs[] = {
        {"__tostring", l_mat3_tostring},
        {"__mul", l_mat3_mul},
        {"__index", l_mat3_index},
        {"__newindex", l_mat3_newindex},
        {"inverse", l_mat3_inverse},
        {"transpose", l_mat3_transpose},
        {"decompose", l_mat3_decompose},
        {NULL, NULL},
    };

    if (luaL_newmetatable(L, META_MAT3))
    {
        luaL_setfuncs(L, mat3_funcs, 0);
        protect_metatable(L);
    }
    lua_pop(L, 1); // Pop metatable

    // MAT3 Callable Table
    register_type_table(L,
                        "mat3",
                        l_mat3_new,
                        {
                            {"identity", l_mat3_identity},
                            {"from_quat", l_mat3_from_quat},
                            {"inverse", l_mat3_inverse},
                            {"transpose", l_mat3_transpose},
                            {"decompose", l_mat3_decompose},
                        });

    // --- MAT4 ---
    luaL_Reg mat4_funcs[] = {
        {"__tostring", l_mat4_tostring},
        {"__mul", l_mat4_mul},
        {"__index", l_mat4_index},
        {"__newindex", l_mat4_newindex},
        {"inverse", l_mat4_inverse},
        {"transpose", l_mat4_transpose},
        {"decompose", l_mat4_decompose},
        {NULL, NULL},
    };

    if (luaL_newmetatable(L, META_MAT4))
    {
        luaL_setfuncs(L, mat4_funcs, 0);
        protect_metatable(L);
    }
    lua_pop(L, 1); // Pop metatable

    // MAT4 Callable Table
    register_type_table(L,
                        "mat4",
                        l_mat4_new,
                        {
                            {"identity", l_mat4_identity},
                            {"translation", l_mat4_translation},
                            {"scaling", l_mat4_scaling},
                            {"rotation_x", l_mat4_rotation_x},
                            {"rotation_y", l_mat4_rotation_y},
                            {"rotation_z", l_mat4_rotation_z},
                            {"look_at", l_mat4_look_at},
                            {"ortho", l_mat4_ortho},
                            {"perspective", l_mat4_perspective},
                            {"from_quat", l_mat4_from_quat},
                        });

    lua_pushboolean(L, 1);
    return 1;
}
}
} // namespace zabato
