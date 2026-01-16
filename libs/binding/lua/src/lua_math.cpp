#include <new>
#include <vector>
#include <zabato/lua.hpp>
#include <zabato/math.hpp>

namespace zabato
{

// Helper for checking numbers with default
static real check_real(lua_State *L, int arg, real def = 0)
{
    if (lua_gettop(L) >= arg)
        return (real)luaL_checknumber(L, arg);
    return def;
}

// -----------------------------------------------------------------------------
// VEC2
// -----------------------------------------------------------------------------

template <typename T> static int l_vec2_new(lua_State *L)
{
    real x = check_real(L, 1);
    real y = check_real(L, 2);

    void *u = lua_newuserdata(L, sizeof(vec2<T>));
    new (u) vec2<T>(x, y);
    luaL_getmetatable(L, META_VEC2);
    lua_setmetatable(L, -2);
    return 1;
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

    if (key[0] == 'x' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->x);
        return 1;
    }
    if (key[0] == 'y' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->y);
        return 1;
    }

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);
    return 1;
}

static int l_vec2_newindex(lua_State *L)
{
    auto *v         = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    const char *key = luaL_checkstring(L, 2);
    real val        = (real)luaL_checknumber(L, 3);

    if (key[0] == 'x' && key[1] == '\0')
    {
        v->x = val;
        return 0;
    }
    if (key[0] == 'y' && key[1] == '\0')
    {
        v->y = val;
        return 0;
    }

    return 0;
}

static int l_vec2_add(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    auto *b = (vec2<real> *)luaL_checkudata(L, 2, META_VEC2);
    void *u = lua_newuserdata(L, sizeof(vec2<real>));
    new (u) vec2<real>(*a + *b);
    luaL_getmetatable(L, META_VEC2);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vec2_sub(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    auto *b = (vec2<real> *)luaL_checkudata(L, 2, META_VEC2);
    void *u = lua_newuserdata(L, sizeof(vec2<real>));
    new (u) vec2<real>(*a - *b);
    luaL_getmetatable(L, META_VEC2);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vec2_mul(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    if (lua_isnumber(L, 2))
    {
        real b  = (real)lua_tonumber(L, 2);
        void *u = lua_newuserdata(L, sizeof(vec2<real>));
        new (u) vec2<real>(*a * b);
    }
    else
    {
        auto *b = (vec2<real> *)luaL_checkudata(L, 2, META_VEC2);
        void *u = lua_newuserdata(L, sizeof(vec2<real>));
        new (u) vec2<real>(*a * *b);
    }
    luaL_getmetatable(L, META_VEC2);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vec2_div(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    if (lua_isnumber(L, 2))
    {
        real b  = (real)lua_tonumber(L, 2);
        void *u = lua_newuserdata(L, sizeof(vec2<real>));
        new (u) vec2<real>(*a / b);
    }
    else
    {
        auto *b = (vec2<real> *)luaL_checkudata(L, 2, META_VEC2);
        void *u = lua_newuserdata(L, sizeof(vec2<real>));
        new (u) vec2<real>(*a / *b);
    }
    luaL_getmetatable(L, META_VEC2);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vec2_unm(lua_State *L)
{
    auto *a = (vec2<real> *)luaL_checkudata(L, 1, META_VEC2);
    void *u = lua_newuserdata(L, sizeof(vec2<real>));
    new (u) vec2<real>(-(*a));
    luaL_getmetatable(L, META_VEC2);
    lua_setmetatable(L, -2);
    return 1;
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
    void *u = lua_newuserdata(L, sizeof(vec2<real>));
    new (u) vec2<real>(normalize(*a));
    luaL_getmetatable(L, META_VEC2);
    lua_setmetatable(L, -2);
    return 1;
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

template <typename T> static int l_vec3_new(lua_State *L)
{
    real x = check_real(L, 1);
    real y = check_real(L, 2);
    real z = check_real(L, 3);

    void *u = lua_newuserdata(L, sizeof(vec3<T>));
    new (u) vec3<T>(x, y, z);
    luaL_getmetatable(L, META_VEC3);
    lua_setmetatable(L, -2);
    return 1;
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

    if (key[0] == 'x' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->x);
        return 1;
    }
    if (key[0] == 'y' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->y);
        return 1;
    }
    if (key[0] == 'z' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->z);
        return 1;
    }
    if (key[0] == 'r' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->x);
        return 1;
    }
    if (key[0] == 'g' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->y);
        return 1;
    }
    if (key[0] == 'b' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->z);
        return 1;
    }

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);
    return 1;
}

static int l_vec3_newindex(lua_State *L)
{
    auto *v         = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    const char *key = luaL_checkstring(L, 2);
    real val        = (real)luaL_checknumber(L, 3);

    if (key[0] == 'x' && key[1] == '\0')
    {
        v->x = val;
        return 0;
    }
    if (key[0] == 'y' && key[1] == '\0')
    {
        v->y = val;
        return 0;
    }
    if (key[0] == 'z' && key[1] == '\0')
    {
        v->z = val;
        return 0;
    }
    if (key[0] == 'r' && key[1] == '\0')
    {
        v->x = val;
        return 0;
    }
    if (key[0] == 'g' && key[1] == '\0')
    {
        v->y = val;
        return 0;
    }
    if (key[0] == 'b' && key[1] == '\0')
    {
        v->z = val;
        return 0;
    }

    return 0;
}

static int l_vec3_add(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    void *u = lua_newuserdata(L, sizeof(vec3<real>));
    new (u) vec3<real>(*a + *b);
    luaL_getmetatable(L, META_VEC3);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vec3_sub(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    void *u = lua_newuserdata(L, sizeof(vec3<real>));
    new (u) vec3<real>(*a - *b);
    luaL_getmetatable(L, META_VEC3);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vec3_mul(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    if (lua_isnumber(L, 2))
    {
        real b  = (real)lua_tonumber(L, 2);
        void *u = lua_newuserdata(L, sizeof(vec3<real>));
        new (u) vec3<real>(*a * b);
    }
    else
    {
        auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
        void *u = lua_newuserdata(L, sizeof(vec3<real>));
        new (u) vec3<real>(*a * *b);
    }
    luaL_getmetatable(L, META_VEC3);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vec3_div(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    if (lua_isnumber(L, 2))
    {
        real b  = (real)lua_tonumber(L, 2);
        void *u = lua_newuserdata(L, sizeof(vec3<real>));
        new (u) vec3<real>(*a / b);
    }
    else
    {
        auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
        void *u = lua_newuserdata(L, sizeof(vec3<real>));
        new (u) vec3<real>(*a / *b);
    }
    luaL_getmetatable(L, META_VEC3);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vec3_unm(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    void *u = lua_newuserdata(L, sizeof(vec3<real>));
    new (u) vec3<real>(-(*a));
    luaL_getmetatable(L, META_VEC3);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vec3_eq(lua_State *L)
{
    auto *a = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    auto *b = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    lua_pushboolean(L, *a == *b);
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
    void *u = lua_newuserdata(L, sizeof(vec3<real>));
    new (u) vec3<real>(cross(*a, *b));
    luaL_getmetatable(L, META_VEC3);
    lua_setmetatable(L, -2);
    return 1;
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
    void *u = lua_newuserdata(L, sizeof(vec3<real>));
    new (u) vec3<real>(normalize(*a));
    luaL_getmetatable(L, META_VEC3);
    lua_setmetatable(L, -2);
    return 1;
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
    void *u = lua_newuserdata(L, sizeof(vec3<real>));
    new (u) vec3<real>(lerp(*a, *b, t));
    luaL_getmetatable(L, META_VEC3);
    lua_setmetatable(L, -2);
    return 1;
}

// -----------------------------------------------------------------------------
// VEC4
// -----------------------------------------------------------------------------

template <typename T> static int l_vec4_new(lua_State *L)
{
    real x = check_real(L, 1);
    real y = check_real(L, 2);
    real z = check_real(L, 3);
    real w = check_real(L, 4);

    void *u = lua_newuserdata(L, sizeof(vec4<T>));
    new (u) vec4<T>(x, y, z, w);
    luaL_getmetatable(L, META_VEC4);
    lua_setmetatable(L, -2);
    return 1;
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

    if (key[0] == 'x' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->x);
        return 1;
    }
    if (key[0] == 'y' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->y);
        return 1;
    }
    if (key[0] == 'z' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->z);
        return 1;
    }
    if (key[0] == 'w' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->w);
        return 1;
    }
    if (key[0] == 'r' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->x);
        return 1;
    }
    if (key[0] == 'g' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->y);
        return 1;
    }
    if (key[0] == 'b' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->z);
        return 1;
    }
    if (key[0] == 'a' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->w);
        return 1;
    }

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);
    return 1;
}

static int l_vec4_newindex(lua_State *L)
{
    auto *v         = (vec4<real> *)luaL_checkudata(L, 1, META_VEC4);
    const char *key = luaL_checkstring(L, 2);
    real val        = (real)luaL_checknumber(L, 3);

    if (key[0] == 'x' && key[1] == '\0')
    {
        v->x = val;
        return 0;
    }
    if (key[0] == 'y' && key[1] == '\0')
    {
        v->y = val;
        return 0;
    }
    if (key[0] == 'z' && key[1] == '\0')
    {
        v->z = val;
        return 0;
    }
    if (key[0] == 'w' && key[1] == '\0')
    {
        v->w = val;
        return 0;
    }
    if (key[0] == 'r' && key[1] == '\0')
    {
        v->x = val;
        return 0;
    }
    if (key[0] == 'g' && key[1] == '\0')
    {
        v->y = val;
        return 0;
    }
    if (key[0] == 'b' && key[1] == '\0')
    {
        v->z = val;
        return 0;
    }
    if (key[0] == 'a' && key[1] == '\0')
    {
        v->w = val;
        return 0;
    }

    return 0;
}

// -----------------------------------------------------------------------------
// QUAT
// -----------------------------------------------------------------------------

template <typename T> static int l_quat_new(lua_State *L)
{
    // Identity by default: w=1
    real w = 1, x = 0, y = 0, z = 0;
    if (lua_gettop(L) >= 4)
    {
        w = check_real(L, 1);
        x = check_real(L, 2);
        y = check_real(L, 3);
        z = check_real(L, 4);
    }

    void *u = lua_newuserdata(L, sizeof(quat<T>));
    new (u) quat<T>(w, x, y, z);
    luaL_getmetatable(L, META_QUAT);
    lua_setmetatable(L, -2);
    return 1;
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

    if (key[0] == 'w' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->w);
        return 1;
    }
    if (key[0] == 'x' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->x);
        return 1;
    }
    if (key[0] == 'y' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->y);
        return 1;
    }
    if (key[0] == 'z' && key[1] == '\0')
    {
        lua_pushnumber(L, (double)v->z);
        return 1;
    }

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);
    return 1;
}

static int l_quat_newindex(lua_State *L)
{
    auto *v         = (quat<real> *)luaL_checkudata(L, 1, META_QUAT);
    const char *key = luaL_checkstring(L, 2);
    real val        = (real)luaL_checknumber(L, 3);

    if (key[0] == 'w' && key[1] == '\0')
    {
        v->w = val;
        return 0;
    }
    if (key[0] == 'x' && key[1] == '\0')
    {
        v->x = val;
        return 0;
    }
    if (key[0] == 'y' && key[1] == '\0')
    {
        v->y = val;
        return 0;
    }
    if (key[0] == 'z' && key[1] == '\0')
    {
        v->z = val;
        return 0;
    }
    return 0;
}

static int l_quat_mul(lua_State *L)
{
    auto *a = (quat<real> *)luaL_checkudata(L, 1, META_QUAT);

    // Check if second arg is quaternion or vector
    void *ud = lua_touserdata(L, 2);
    if (ud)
    {
        if (lua_getmetatable(L, 2))
        {
            lua_getfield(L, LUA_REGISTRYINDEX, META_QUAT);
            bool is_quat = lua_rawequal(L, -1, -2);
            lua_pop(L, 2); // pop field and metatable

            if (is_quat)
            {
                auto *b = (quat<real> *)ud;
                void *u = lua_newuserdata(L, sizeof(quat<real>));
                new (u) quat<real>(*a * *b);
                luaL_getmetatable(L, META_QUAT);
                lua_setmetatable(L, -2);
                return 1;
            }

            lua_getfield(L, LUA_REGISTRYINDEX, META_VEC3);
            bool is_vec3 = lua_rawequal(L, -1, -2);
            lua_pop(L, 2);

            if (is_vec3)
            {
                auto *b = (vec3<real> *)ud;
                void *u = lua_newuserdata(L, sizeof(vec3<real>));
                new (u) vec3<real>(*a * *b);
                luaL_getmetatable(L, META_VEC3);
                lua_setmetatable(L, -2);
                return 1;
            }
        }
    }

    return luaL_error(L, "Invalid operands for quat mul");
}

static int l_quat_slerp(lua_State *L)
{
    auto *a = (quat<real> *)luaL_checkudata(L, 1, META_QUAT);
    auto *b = (quat<real> *)luaL_checkudata(L, 2, META_QUAT);
    real t  = (real)luaL_checknumber(L, 3);
    void *u = lua_newuserdata(L, sizeof(quat<real>));
    new (u) quat<real>(slerp(*a, *b, t));
    luaL_getmetatable(L, META_QUAT);
    lua_setmetatable(L, -2);
    return 1;
}

// -----------------------------------------------------------------------------
// MAT3
// -----------------------------------------------------------------------------

template <typename T> static int l_mat3_new(lua_State *L)
{
    void *u = lua_newuserdata(L, sizeof(mat3<T>));
    new (u) mat3<T>();
    luaL_getmetatable(L, META_MAT3);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat3_tostring(lua_State *L)
{
    lua_pushstring(L, "mat3(...)");
    return 1;
}

static int l_mat3_mul(lua_State *L)
{
    auto *a = (mat3<real> *)luaL_checkudata(L, 1, META_MAT3);

    // Check 2nd arg
    void *ud = lua_touserdata(L, 2);
    if (ud)
    {
        if (lua_getmetatable(L, 2))
        {
            lua_getfield(L, LUA_REGISTRYINDEX, META_MAT3);
            bool is_mat3 = lua_rawequal(L, -1, -2);
            lua_pop(L, 2);

            if (is_mat3)
            {
                auto *b = (mat3<real> *)ud;
                void *u = lua_newuserdata(L, sizeof(mat3<real>));
                new (u) mat3<real>(*a * *b);
                luaL_getmetatable(L, META_MAT3);
                lua_setmetatable(L, -2);
                return 1;
            }

            lua_getfield(L, LUA_REGISTRYINDEX, META_VEC3);
            bool is_vec3 = lua_rawequal(L, -1, -2);
            lua_pop(L, 2);

            if (is_vec3)
            {
                auto *b = (vec3<real> *)ud;
                void *u = lua_newuserdata(L, sizeof(vec3<real>));
                new (u) vec3<real>(*a * *b);
                luaL_getmetatable(L, META_VEC3);
                lua_setmetatable(L, -2);
                return 1;
            }
        }
    }
    return luaL_error(L, "Invalid operands for mat3 mul");
}

static int l_mat3_identity(lua_State *L)
{
    void *u = lua_newuserdata(L, sizeof(mat3<real>));
    new (u) mat3<real>(mat3<real>::identity());
    luaL_getmetatable(L, META_MAT3);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat3_inverse(lua_State *L)
{
    auto *m = (mat3<real> *)luaL_checkudata(L, 1, META_MAT3);
    void *u = lua_newuserdata(L, sizeof(mat3<real>));
    mat3<real> res;
    if (inverse(*m, res))
    {
        new (u) mat3<real>(res);
        luaL_getmetatable(L, META_MAT3);
        lua_setmetatable(L, -2);
        return 1;
    }
    return 0; // Return nil if singular?
}

static int l_mat3_transpose(lua_State *L)
{
    auto *m = (mat3<real> *)luaL_checkudata(L, 1, META_MAT3);
    void *u = lua_newuserdata(L, sizeof(mat3<real>));
    new (u) mat3<real>(transpose(*m));
    luaL_getmetatable(L, META_MAT3);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat3_from_quat(lua_State *L)
{
    auto *q = (quat<real> *)luaL_checkudata(L, 1, META_QUAT);
    void *u = lua_newuserdata(L, sizeof(mat3<real>));
    new (u) mat3<real>(mat3_from_quat(*q));
    luaL_getmetatable(L, META_MAT3);
    lua_setmetatable(L, -2);
    return 1;
}

// -----------------------------------------------------------------------------
// MAT4
// -----------------------------------------------------------------------------

template <typename T> static int l_mat4_new(lua_State *L)
{
    void *u = lua_newuserdata(L, sizeof(mat4<T>));
    new (u) mat4<T>();
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat4_identity(lua_State *L)
{
    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    new (u) mat4<real>(mat4<real>::identity());
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat4_translation(lua_State *L)
{
    auto *v = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    new (u) mat4<real>(mat4_translation(*v));
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat4_scaling(lua_State *L)
{
    auto *v = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    new (u) mat4<real>(mat4_scaling(*v));
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat4_rotation_x(lua_State *L)
{
    real a  = (real)luaL_checknumber(L, 1);
    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    new (u) mat4<real>(mat4_rotation_x(a));
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}
static int l_mat4_rotation_y(lua_State *L)
{
    real a  = (real)luaL_checknumber(L, 1);
    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    new (u) mat4<real>(mat4_rotation_y(a));
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}
static int l_mat4_rotation_z(lua_State *L)
{
    real a  = (real)luaL_checknumber(L, 1);
    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    new (u) mat4<real>(mat4_rotation_z(a));
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat4_tostring(lua_State *L)
{
    lua_pushstring(L, "mat4(...)");
    return 1;
}

static int l_mat4_mul(lua_State *L)
{
    auto *a = (mat4<real> *)luaL_checkudata(L, 1, META_MAT4);

    // Check 2nd arg
    void *ud = lua_touserdata(L, 2);
    if (ud)
    {
        if (lua_getmetatable(L, 2))
        {
            lua_getfield(L, LUA_REGISTRYINDEX, META_MAT4);
            bool is_mat4 = lua_rawequal(L, -1, -2);
            lua_pop(L, 2);

            if (is_mat4)
            {
                auto *b = (mat4<real> *)ud;
                void *u = lua_newuserdata(L, sizeof(mat4<real>));
                new (u) mat4<real>(*a * *b);
                luaL_getmetatable(L, META_MAT4);
                lua_setmetatable(L, -2);
                return 1;
            }

            lua_getfield(L, LUA_REGISTRYINDEX, META_VEC4);
            bool is_vec4 = lua_rawequal(L, -1, -2);
            lua_pop(L, 2);

            if (is_vec4)
            {
                auto *b = (vec4<real> *)ud;
                void *u = lua_newuserdata(L, sizeof(vec4<real>));
                new (u) vec4<real>(*a * *b);
                luaL_getmetatable(L, META_VEC4);
                lua_setmetatable(L, -2);
                return 1;
            }
        }
    }
    return luaL_error(L, "Invalid operands for mat4 mul");
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
    return 0;
}

static int l_mat4_transpose(lua_State *L)
{
    auto *m = (mat4<real> *)luaL_checkudata(L, 1, META_MAT4);
    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    new (u) mat4<real>(transpose(*m));
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat4_look_at(lua_State *L)
{
    auto *eye    = (vec3<real> *)luaL_checkudata(L, 1, META_VEC3);
    auto *center = (vec3<real> *)luaL_checkudata(L, 2, META_VEC3);
    auto *up     = (vec3<real> *)luaL_checkudata(L, 3, META_VEC3);

    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    new (u) mat4<real>(mat4_look_at(*eye, *center, *up));
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat4_ortho(lua_State *L)
{
    real left   = (real)luaL_checknumber(L, 1);
    real right  = (real)luaL_checknumber(L, 2);
    real bottom = (real)luaL_checknumber(L, 3);
    real top    = (real)luaL_checknumber(L, 4);
    real near   = (real)luaL_checknumber(L, 5);
    real far    = (real)luaL_checknumber(L, 6);

    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    new (u) mat4<real>(mat4_ortho(left, right, bottom, top, near, far));
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat4_perspective(lua_State *L)
{
    real fov    = (real)luaL_checknumber(L, 1);
    real aspect = (real)luaL_checknumber(L, 2);
    real near   = (real)luaL_checknumber(L, 3);
    real far    = (real)luaL_checknumber(L, 4);

    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    new (u) mat4<real>(mat4_perspective_fov(fov, aspect, near, far));
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mat4_decompose(lua_State *L)
{
    auto *m = (mat4<real> *)luaL_checkudata(L, 1, META_MAT4);
    vec3<real> pos;
    vec3<real> scale;
    quat<real> rot;

    mat4_decompose(*m, pos, scale, rot);

    // Push 3 values
    void *u1 = lua_newuserdata(L, sizeof(vec3<real>));
    new (u1) vec3<real>(pos);
    luaL_getmetatable(L, META_VEC3);
    lua_setmetatable(L, -2);

    void *u2 = lua_newuserdata(L, sizeof(vec3<real>));
    new (u2) vec3<real>(scale);
    luaL_getmetatable(L, META_VEC3);
    lua_setmetatable(L, -2);

    void *u3 = lua_newuserdata(L, sizeof(quat<real>));
    new (u3) quat<real>(rot);
    luaL_getmetatable(L, META_QUAT);
    lua_setmetatable(L, -2);

    return 3;
}

static int l_mat4_from_quat(lua_State *L)
{
    auto *q = (quat<real> *)luaL_checkudata(L, 1, META_QUAT);
    void *u = lua_newuserdata(L, sizeof(mat4<real>));
    new (u) mat4<real>(mat4_from_quat(*q));
    luaL_getmetatable(L, META_MAT4);
    lua_setmetatable(L, -2);
    return 1;
}

// -----------------------------------------------------------------------------
// REGISTRATION
// -----------------------------------------------------------------------------

// Helper to register a constructor table with static methods
static void register_type_table(lua_State *L,
                                const char *name,
                                lua_CFunction constructor,
                                const std::vector<luaL_Reg> &static_funcs)
{
    // Create the table that will act as the class
    lua_newtable(L); // [table]

    // Register static methods
    for (const auto &reg : static_funcs)
    {
        lua_pushcfunction(L, reg.func);
        lua_setfield(L, -2, reg.name);
    }

    // Create metatable for the class table to make it callable
    lua_newtable(L); // [table, metatable]
    lua_pushcfunction(L, constructor);
    lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2); // [table]

    // Set global
    lua_setglobal(L, name);
}

void register_math_bindings(lua_State *L)
{
    // VEC2 Instance Attributes
    if (luaL_newmetatable(L, META_VEC2))
    {
        lua_pushcfunction(L, l_vec2_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, l_vec2_add);
        lua_setfield(L, -2, "__add");
        lua_pushcfunction(L, l_vec2_sub);
        lua_setfield(L, -2, "__sub");
        lua_pushcfunction(L, l_vec2_mul);
        lua_setfield(L, -2, "__mul");
        lua_pushcfunction(L, l_vec2_div);
        lua_setfield(L, -2, "__div");
        lua_pushcfunction(L, l_vec2_unm);
        lua_setfield(L, -2, "__unm");
        lua_pushcfunction(L, l_vec2_eq);
        lua_setfield(L, -2, "__eq");
        lua_pushcfunction(L, l_vec2_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, l_vec2_newindex);
        lua_setfield(L, -2, "__newindex");

        // Instance methods
        lua_pushcfunction(L, l_vec2_dot);
        lua_setfield(L, -2, "dot");
        lua_pushcfunction(L, l_vec2_length);
        lua_setfield(L, -2, "length");
        lua_pushcfunction(L, l_vec2_normalize);
        lua_setfield(L, -2, "normalize");
        lua_pushcfunction(L, l_vec2_distance);
        lua_setfield(L, -2, "distance");
    }
    lua_pop(L, 1);

    // VEC2 Callable Table
    register_type_table(
        L,
        "vec2",
        [](lua_State *L)
        {
            lua_remove(L, 1); // Remove the table (self) passed to __call
            return l_vec2_new<real>(L);
        },
        {
            {"dot", l_vec2_dot},
            {"length", l_vec2_length},
            {"normalize", l_vec2_normalize},
            {"distance", l_vec2_distance},
        });

    // VEC3 Instance Attributes
    if (luaL_newmetatable(L, META_VEC3))
    {
        lua_pushcfunction(L, l_vec3_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, l_vec3_add);
        lua_setfield(L, -2, "__add");
        lua_pushcfunction(L, l_vec3_sub);
        lua_setfield(L, -2, "__sub");
        lua_pushcfunction(L, l_vec3_mul);
        lua_setfield(L, -2, "__mul");
        lua_pushcfunction(L, l_vec3_div);
        lua_setfield(L, -2, "__div");
        lua_pushcfunction(L, l_vec3_unm);
        lua_setfield(L, -2, "__unm");
        lua_pushcfunction(L, l_vec3_eq);
        lua_setfield(L, -2, "__eq");
        lua_pushcfunction(L, l_vec3_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, l_vec3_newindex);
        lua_setfield(L, -2, "__newindex");

        lua_pushcfunction(L, l_vec3_dot);
        lua_setfield(L, -2, "dot");
        lua_pushcfunction(L, l_vec3_cross);
        lua_setfield(L, -2, "cross");
        lua_pushcfunction(L, l_vec3_length);
        lua_setfield(L, -2, "length");
        lua_pushcfunction(L, l_vec3_normalize);
        lua_setfield(L, -2, "normalize");
        lua_pushcfunction(L, l_vec3_distance);
        lua_setfield(L, -2, "distance");
        lua_pushcfunction(L, l_vec3_lerp);
        lua_setfield(L, -2, "lerp");
    }
    lua_pop(L, 1);

    // VEC3 Callable Table
    register_type_table(L,
                        "vec3",
                        [](lua_State *L)
                        {
                            lua_remove(L, 1);
                            return l_vec3_new<real>(L);
                        },
                        {
                            {"dot", l_vec3_dot},
                            {"cross", l_vec3_cross},
                            {"length", l_vec3_length},
                            {"normalize", l_vec3_normalize},
                            {"distance", l_vec3_distance},
                            {"lerp", l_vec3_lerp},
                        });

    // VEC4 Instance Attributes
    if (luaL_newmetatable(L, META_VEC4))
    {
        lua_pushcfunction(L, l_vec4_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, l_vec4_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, l_vec4_newindex);
        lua_setfield(L, -2, "__newindex");
    }
    lua_pop(L, 1);

    // VEC4 Callable Table
    register_type_table(L,
                        "vec4",
                        [](lua_State *L)
                        {
                            lua_remove(L, 1);
                            return l_vec4_new<real>(L);
                        },
                        {});

    // QUAT Instance Attributes
    if (luaL_newmetatable(L, META_QUAT))
    {
        lua_pushcfunction(L, l_quat_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, l_quat_mul);
        lua_setfield(L, -2, "__mul");
        lua_pushcfunction(L, l_quat_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, l_quat_newindex);
        lua_setfield(L, -2, "__newindex");

        lua_pushcfunction(L, l_quat_slerp);
        lua_setfield(L, -2, "slerp");
    }
    lua_pop(L, 1);

    // QUAT Callable Table
    register_type_table(L,
                        "quat",
                        [](lua_State *L)
                        {
                            lua_remove(L, 1);
                            return l_quat_new<real>(L);
                        },
                        {
                            {"slerp", l_quat_slerp},
                        });

    // MAT3 Instance Attributes
    if (luaL_newmetatable(L, META_MAT3))
    {
        lua_pushcfunction(L, l_mat3_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, l_mat3_mul);
        lua_setfield(L, -2, "__mul");
        lua_pushcfunction(L, l_mat3_inverse);
        lua_setfield(L, -2, "inverse");
        lua_pushcfunction(L, l_mat3_transpose);
        lua_setfield(L, -2, "transpose");
    }
    lua_pop(L, 1);

    // MAT3 Callable Table
    register_type_table(L,
                        "mat3",
                        [](lua_State *L)
                        {
                            lua_remove(L, 1);
                            return l_mat3_new<real>(L);
                        },
                        {
                            {"identity", l_mat3_identity},
                            {"from_quat", l_mat3_from_quat},
                        });

    // MAT4 Instance Attributes
    if (luaL_newmetatable(L, META_MAT4))
    {
        lua_pushcfunction(L, l_mat4_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, l_mat4_mul);
        lua_setfield(L, -2, "__mul");
        lua_pushcfunction(L, l_mat4_inverse);
        lua_setfield(L, -2, "inverse");
        lua_pushcfunction(L, l_mat4_transpose);
        lua_setfield(L, -2, "transpose");
        lua_pushcfunction(L, l_mat4_decompose);
        lua_setfield(L, -2, "decompose");
    }
    lua_pop(L, 1);

    // MAT4 Callable Table
    register_type_table(L,
                        "mat4",
                        [](lua_State *L)
                        {
                            lua_remove(L, 1);
                            return l_mat4_new<real>(L);
                        },
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
}

} // namespace zabato
