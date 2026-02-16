#pragma once

#include "internal.hpp"
#include "lua_script_args.hpp"

#include <zabato/console.hpp>
#include <zabato/reflection.hpp>
#include <zabato/shared_ptr.hpp>

namespace zabato
{
static const char META_OBJECT[] = "zabato.object";
static const char META_VEC2[]   = "zabato.vec2";
static const char META_VEC3[]   = "zabato.vec3";
static const char META_VEC4[]   = "zabato.vec4";
static const char META_QUAT[]   = "zabato.quat";
static const char META_COLOR[]  = "zabato.color";
static const char META_MAT3[]   = "zabato.mat3";
static const char META_MAT4[]   = "zabato.mat4";

static const char SYS_REG_KEY[] = "zabato.sys";

static lua_script_system *get_sys(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, SYS_REG_KEY);
    auto *sys = (lua_script_system *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return sys;
}

static fs::file_system *get_filesystem(lua_State *L)
{
    auto *sys = get_sys(L);
    if (!sys)
        return nullptr;
    return &sys->get_file_system();
}

static console *lua_get_console(lua_State *L)
{
    auto *sys = get_sys(L);
    if (!sys)
        return nullptr;
    return &sys->get_console();
}

class lua_value;

static void push_object_metatable(lua_State *L);
static void
push_callable_value(lua_State *L, lua_script_system *sys, const value &v);
static void push_value_to_lua(lua_State *L, const value &val);

static int object_index(lua_State *L)
{
    /* Stack: [userdata (object*), key (string)]*/
    auto *ptr = (pointer<object> *)lua_touserdata(L, 1);
    if (!ptr || !*ptr)
        return 0;

    object *obj     = *ptr;
    size_t len      = 0;
    const char *key = luaL_checklstring(L, 2, &len);
    string key_sv(key, len);

    /* Get system from upvalue */
    lua_script_system *sys = get_sys(L);

    /* Walk the inheritance chain via Reflection */
    const rtti *type = &obj->type();
    while (type)
    {
        const reflection *ref = type->get_reflection();
        if (!ref)
            ref = &type->ensure_reflection();

        if (ref)
        {
            /* Check properties (Getters) */
            property_def def;
            if (ref->properties.try_get_value(key_sv, def))
            {
                if (def.getter.is_valid())
                {
                    auto d = def.getter.as_function();
                    d.set_object(obj); /* Bind 'self' */

                    lua_script_args args(L, 0, 0, sys);
                    d(sys, nullptr, &args);
                    return args.returns_count;
                }
            }

            /* Check Methods */
            value method_val;
            if (ref->methods.try_get_value(key_sv, method_val))
            {
                if (method_val.is_valid())
                {
                    auto d = method_val.as_function();
                    d.set_object(obj); /* Bind 'self' */

                    /* Push the bound function to Lua */
                    push_callable_value(L, sys, value(d));
                    return 1;
                }
            }
        }
        type = type->base();
    }
    return 0;
}

static int object_newindex(lua_State *L)
{
    /* Stack: [userdata (object*), key (string), value (any) ]*/
    auto *ptr = (pointer<object> *)lua_touserdata(L, 1);
    if (!ptr || !*ptr)
        return 0;

    object *obj            = *ptr;
    const char *key        = luaL_checkstring(L, 2);
    lua_script_system *sys = get_sys(L);

    const rtti *type = &obj->type();
    while (type)
    {
        const reflection *ref = type->get_reflection();
        if (!ref)
            ref = &type->ensure_reflection();

        if (ref)
        {
            /* Check Properties (Setters) */
            property_def def;
            if (ref->properties.try_get_value(string(key), def))
            {
                if (def.setter.is_valid())
                {
                    auto d = def.setter.as_function();
                    d.set_object(obj);

                    /* Invoke C++ Setter with Value at Stack index 3*/
                    lua_script_args args(L, 3, 1, sys);
                    d(sys, nullptr, &args);
                    return 0;
                }
            }
        }
        type = type->base();
    }

    return luaL_error(L, "Property '%s' not found or read-only", key);
}

static int object_gc(lua_State *L)
{
    auto *ptr = (pointer<object> *)lua_touserdata(L, 1);
    ptr->~pointer(); /* Destruct the pointer */
    return 0;
}

static int object_tostring(lua_State *L)
{
    auto *ptr = (pointer<object> *)lua_touserdata(L, 1);
    if (!ptr || !*ptr)
        lua_pushstring(L, "nil object");
    else
        lua_pushfstring(L, "object(%s)", (*ptr)->name());
    return 1;
}

static int object_eq(lua_State *L)
{
    auto *ptr1 = (pointer<object> *)luaL_testudata(L, 1, META_OBJECT);
    auto *ptr2 = (pointer<object> *)luaL_testudata(L, 2, META_OBJECT);
    if (ptr1 && ptr2)
        lua_pushboolean(L, *ptr1 == *ptr2);
    else
        lua_pushboolean(L, 0);
    return 1;
}

static void push_object_metatable(lua_State *L)
{
    if (luaL_newmetatable(L, META_OBJECT))
    {
        lua_script_system *sys = get_sys(L);

        lua_pushcfunction(L, object_gc);
        lua_setfield(L, -2, "__gc");

        lua_pushcfunction(L, object_tostring);
        lua_setfield(L, -2, "__tostring");

        lua_pushcfunction(L, object_eq);
        lua_setfield(L, -2, "__eq");

        lua_pushcfunction(L, object_index);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, object_newindex);
        lua_setfield(L, -2, "__newindex");

        protect_metatable(L);
    }
}

class lua_iterator;
class lua_value : public ivalue
{
public:
    static const rtti TYPE;
    const rtti &get_type_info() const override { return TYPE; }

    lua_State *L;
    int ref_id;

    mutable string m_string_cache;

    lua_value(lua_State *l, int ref) : L(l), ref_id(ref) {}
    lua_value(lua_State *l, int index, bool) : L(l)
    {
        lua_pushvalue(L, index);
        ref_id = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    ~lua_value() override
    {
        if (L && ref_id > 0)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, ref_id);
        }
    }

    void push() const
    {
        if (ref_id > 0)
            lua_rawgeti(L, LUA_REGISTRYINDEX, ref_id);
        else
            lua_pushnil(L);
    }

    value_type type() const override
    {
        if (ref_id <= 0)
            return value_type::NIL;
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref_id);
        int t = lua_type(L, -1);

        value_type res = value_type::NIL;
        switch (t)
        {
        case LUA_TNIL:
            res = value_type::NIL;
            break;
        case LUA_TBOOLEAN:
            res = value_type::BOOLEAN;
            break;
        case LUA_TNUMBER:
            res = value_type::NUMBER;
            break;
        case LUA_TSTRING:
            res = value_type::STRING;
            break;
        case LUA_TTABLE:
            res = value_type::MAP;
            break;
        case LUA_TFUNCTION:
            res = value_type::FUNCTION;
            break;
        case LUA_TUSERDATA:
        {
            if (lua_getmetatable(L, -1))
            {
                lua_getfield(L, LUA_REGISTRYINDEX, META_OBJECT);
                if (lua_rawequal(L, -1, -2))
                {
                    res = value_type::OBJECT;
                }
                lua_pop(L, 1);

                if (res == value_type::NIL)
                {
                    auto check = [&](const char *meta, value_type target)
                    {
                        if (res != value_type::NIL)
                            return;
                        lua_getfield(L, LUA_REGISTRYINDEX, meta);
                        if (lua_rawequal(L, -1, -2))
                            res = target;
                        lua_pop(L, 1);
                    };
                    check(META_VEC2, value_type::VEC2);
                    check(META_VEC3, value_type::VEC3);
                    check(META_VEC4, value_type::VEC4);
                    check(META_QUAT, value_type::QUAT);
                    check(META_COLOR, value_type::COLOR);
                    check(META_MAT3, value_type::MAT3);
                    check(META_MAT4, value_type::MAT4);
                }
                lua_pop(L, 1); // Pop MT
            }
            if (res == value_type::NIL)
                res = value_type::POINTER;
            break;
        }
        case LUA_TLIGHTUSERDATA:
            res = value_type::POINTER;
            break;
        }

        lua_pop(L, 1);
        return res;
    }

    void set(const value &key, const value &val) override
    {
        push(); // Push table
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            return;
        }

        push_value_to_lua(L, key);
        push_value_to_lua(L, val);

        lua_settable(L, -3);
        lua_pop(L, 1);
    }

    value get(const value &key) const override
    {
        push();
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            return value();
        }

        push_value_to_lua(L, key);
        lua_gettable(L, -2);

        value res(static_cast<shared_ptr<ivalue>>(
            make_shared<lua_value>(L, -1, true)));
        lua_pop(L, 2);
        return res;
    }

    shared_ptr<iterator> get_iterator() const override
    {
        push(); // Push 'this'
        // Stack: Obj

        lua_getglobal(L, "pairs"); // Stack: Obj, pairs
        lua_pushvalue(L, -2);      // Stack: Obj, pairs, Obj
        // call pairs(Obj) -> Func, State, Key, Closing (Optional)
        // We request 4 return values to capture the optional closing value.
        // If it's missing, Lua pushes nil.
        if (lua_pcall(L, 1, 4, 0) == LUA_OK)
        {
            // Stack: Obj, Func, State, Key, Closing
            auto it = make_shared<lua_iterator>(L);

            // lua_iterator constructor moved 4 items to thread.
            // Stack is now: Obj

            lua_pop(L, 1); // Pop Obj
            return it;
        }
        else
        {
            // Stack: Obj, Error
            lua_pop(L, 2); // Pop Error and Obj
        }
        return nullptr;
    }

    bool as_bool() const override
    {
        push();
        bool b = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return b;
    }
    double as_number() const override
    {
        push();
        double n = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return n;
    }
    int64_t as_int() const override
    {
        push();
        int64_t i = (int64_t)lua_tointeger(L, -1);
        lua_pop(L, 1);
        return i;
    }

    string_view as_string() const override
    {
        push();
        size_t len    = 0;
        const char *s = luaL_tolstring(L, -1, &len);

        if (s)
            m_string_cache.assign(s, len);
        else
            m_string_cache.clear();

        lua_pop(L, 2);
        return m_string_cache;
    }

    pointer<base_object> as_object() const override
    {
        pointer<base_object> obj = nullptr;
        push();
        if (lua_isuserdata(L, -1))
        {
            // It's an object!
            pointer<base_object> *ptr =
                static_cast<pointer<base_object> *>(lua_touserdata(L, -1));
            if (ptr && *ptr)
                obj = *ptr;
        }
        lua_pop(L, 1);
        return obj;
    }

    void *as_pointer() const override
    {
        push();
        void *p = nullptr;
        if (lua_isuserdata(L, -1) || lua_islightuserdata(L, -1))
            p = lua_touserdata(L, -1);
        lua_pop(L, 1);
        return p;
    }

    script_delegate as_function() const override { return script_delegate(); }

    void call(script_system *sys,
              script_instance *ctx,
              script_args *args) const override
    {
        lua_script_system *lua_sys = static_cast<lua_script_system *>(sys);
        int base                   = lua_gettop(L);

        push();
        if (!lua_isfunction(L, -1))
        {
            lua_pop(L, 1);
            return;
        }

        int arg_count = args ? args->count() : 0;
        for (int i = 0; i < arg_count; ++i)
            push_value_to_lua(L, args->get_value(i));

        if (lua_pcall(L, arg_count, LUA_MULTRET, 0) != LUA_OK)
        {
            const char *err = lua_tostring(L, -1);
            if (args)
                args->error(err);
            lua_pop(L, 1); // Error message
            return;
        }

        int returns_count = lua_gettop(L) - base;
        if (args && returns_count > 0)
            for (int i = 0; i < returns_count; ++i)
                args->push_return(lua_sys->to_value(base + 1 + i));

        lua_settop(L, base);
    }

    template <typename T> T as_userdata() const
    {
        push();
        T *ptr = (T *)lua_touserdata(L, -1);
        T res  = ptr ? *ptr : T();
        lua_pop(L, 1);
        return res;
    }

    bool is_vec2() const override { return type() == value_type::VEC2; }
    bool is_vec3() const override { return type() == value_type::VEC3; }
    bool is_vec4() const override { return type() == value_type::VEC4; }
    bool is_quat() const override { return type() == value_type::QUAT; }
    bool is_color() const override { return type() == value_type::COLOR; }
    bool is_mat3() const override { return type() == value_type::MAT3; }
    bool is_mat4() const override { return type() == value_type::MAT4; }

    // TODO: check type
    vec2<real> as_vec2() const override { return as_userdata<vec2<real>>(); }
    vec3<real> as_vec3() const override { return as_userdata<vec3<real>>(); }
    vec4<real> as_vec4() const override { return as_userdata<vec4<real>>(); }
    quat<real> as_quat() const override { return as_userdata<quat<real>>(); }
    color as_color() const override { return as_userdata<color>(); }
    mat3<real> as_mat3() const override { return as_userdata<mat3<real>>(); }
    mat4<real> as_mat4() const override { return as_userdata<mat4<real>>(); }

    void set_field(string_view key, const value &v) override
    {
        push(); // Push table
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            return;
        }

        lua_pushlstring(L, key.data(), key.size());

        push_value_to_lua(L, v);

        lua_settable(L, -3);
        lua_pop(L, 1); // Pop table
    }

    value get_field(string_view key) const override
    {
        push(); // Table
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            return value();
        }

        lua_pushlstring(L, key.data(), key.size());
        lua_gettable(L, -2);

        // Result is on stack. Capture it into a NEW lua_value.
        value res(static_cast<shared_ptr<ivalue>>(
            make_shared<lua_value>(L, -1, true)));
        lua_pop(L, 2); // Pop result and table
        return res;
    }

    void push(const value &v) override
    {
        push();
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            return;
        }
        int len = lua_rawlen(L, -1);

        push_value_to_lua(L, v);

        lua_rawseti(L, -2, len + 1);
        lua_pop(L, 1);
    }

    value get_at(size_t index) const override
    {
        push();
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            return value();
        }

        lua_rawgeti(L, -1, index + 1); // Lua 1-based
        value res(static_cast<shared_ptr<ivalue>>(
            make_shared<lua_value>(L, -1, true)));
        lua_pop(L, 2);
        return res;
    }

    void set_at(size_t index, const value &v) override
    {
        push();
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            return;
        }

        lua_rawseti(L, -2, index + 1);
        lua_pop(L, 1);
    }

    void remove_at(size_t index) override
    {
        push();
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            return;
        }

        lua_Integer pos  = static_cast<lua_Integer>(index) + 1;
        lua_Integer size = static_cast<lua_Integer>(lua_rawlen(L, -1));

        for (; pos < size; ++pos)
        {
            lua_rawgeti(L, -1, pos + 1);
            lua_rawseti(L, -2, pos);
        }

        lua_pushnil(L);
        lua_rawseti(L, -2, pos);

        lua_pop(L, 1);
    }

    size_t length() const override
    {
        push();
        size_t len = 0;
        if (lua_istable(L, -1))
            len = lua_rawlen(L, -1);
        else if (lua_isstring(L, -1))
            len = lua_rawlen(L, -1);
        lua_pop(L, 1);
        return len;
    }

    bool operator==(const value &other) const override
    {
        push();
        push_value_to_lua(L, other);
        bool res = lua_compare(L, -2, -1, LUA_OPEQ);
        lua_pop(L, 2);
        return res;
    }

    bool operator!=(const value &other) const override
    {
        return !(*this == other);
    }

    bool operator<(const value &other) const override
    {
        push();
        push_value_to_lua(L, other);
        bool res = lua_compare(L, -2, -1, LUA_OPLT);
        lua_pop(L, 2);
        return res;
    }

    bool operator>(const value &other) const override
    {
        // a > b  <==>  b < a
        push_value_to_lua(L, other); // b
        push();                      // a
        bool res = lua_compare(L, -2, -1, LUA_OPLT);
        lua_pop(L, 2);
        return res;
    }

    bool operator<=(const value &other) const override
    {
        push();
        push_value_to_lua(L, other);
        bool res = lua_compare(L, -2, -1, LUA_OPLE);
        lua_pop(L, 2);
        return res;
    }

    bool operator>=(const value &other) const override
    {
        // a >= b  <==>  b <= a
        push_value_to_lua(L, other); // b
        push();                      // a
        bool res = lua_compare(L, -2, -1, LUA_OPLE);
        lua_pop(L, 2);
        return res;
    }
};

struct lua_iterator : public iterator
{
    lua_State *m_main_L;
    lua_State *m_thread_L;
    int m_thread_ref;
    bool m_valid;

    // Assumes the iterator quadruplet Func, State, Key, Closing is on top of L
    lua_iterator(lua_State *l)
        : m_main_L(l), m_thread_L(nullptr), m_thread_ref(LUA_NOREF),
          m_valid(false)
    {
        // Create a new thread (coroutine) to isolate the iteration stack
        m_thread_L   = lua_newthread(m_main_L);
        m_thread_ref = luaL_ref(m_main_L, LUA_REGISTRYINDEX); // Anchor thread

        // Move inputs to thread stack: [Func, State, Key, Closing]
        // Inputs are at top of main stack
        lua_xmove(m_main_L, m_thread_L, 4);
    }

    ~lua_iterator()
    {
        if (m_thread_ref != LUA_NOREF)
            luaL_unref(m_main_L, LUA_REGISTRYINDEX, m_thread_ref);
    }

    bool next() override
    {
        // Require at least 4 items (Func, State, Key, Closing)
        if (lua_gettop(m_thread_L) < 4)
            return false;

        // Stack layout:
        // 1: Func
        // 2: State
        // 3: Key (Control Variable)
        // 4: Closing (To-be-closed variable, or nil)
        // 5: Value (Previous, optional)

        // Clean up previous value if present
        lua_settop(m_thread_L, 4);

        // Prepare call: Func(State, Key)
        lua_pushvalue(m_thread_L, 1); // Func
        lua_pushvalue(m_thread_L, 2); // State
        lua_pushvalue(m_thread_L, 3); // Key

        if (lua_pcall(m_thread_L, 2, LUA_MULTRET, 0) == LUA_OK)
        {
            // Returns: NewKey, NewValue, ... pushed at 5, 6...
            // Stack: F, S, K, C, NK, NV, ...

            // If NewKey (Slot 5) is nil OR absent (none), iteration finished.
            if (lua_isnoneornil(m_thread_L, 5))
            {
                lua_settop(m_thread_L, 4);
                m_valid = false;
                return false;
            }

            // Stack: [Func, State, Key, Closing, NewKey, NewValue(opt)]

            // Update Control Key (Slot 3) with NewKey (Slot 5)
            lua_copy(m_thread_L, 5, 3);

            // Remove the NewKey from Slot 5.
            // This shifts NewValue (if present) from Slot 6 to Slot 5.
            lua_remove(m_thread_L, 5);

            // Now Stack: [Func, State, Key(New), Closing, NewValue(opt)]
            // We want Slot 5 to hold the Value for 'get_value'.
            // If NewValue wasn't returned, Slot 5 is empty.
            if (lua_gettop(m_thread_L) < 5)
                lua_pushnil(m_thread_L);

            // Trim extra returns, ensuring Stack: [F, S, K, C, V]
            lua_settop(m_thread_L, 5);

            m_valid = true;
            return true;
        }

        m_valid = false;
        return false;
    }

    value get_key() const override
    {
        if (m_valid)
        {
            lua_pushvalue(m_thread_L, 3); // Key
            lua_xmove(m_thread_L, m_main_L, 1);
            value res(static_cast<shared_ptr<ivalue>>(
                make_shared<lua_value>(m_main_L, -1, true)));
            lua_pop(m_main_L, 1);
            return res;
        }
        return value();
    }

    value get_value() const override
    {
        if (m_valid)
        {
            // Value is at Slot 5 due to Closing var at 4
            lua_pushvalue(m_thread_L, 5); // Value
            lua_xmove(m_thread_L, m_main_L, 1);
            value res(static_cast<shared_ptr<ivalue>>(
                make_shared<lua_value>(m_main_L, -1, true)));
            lua_pop(m_main_L, 1);
            return res;
        }
        return value();
    }
};

static int value_gc(lua_State *L)
{
    value *v = (value *)lua_touserdata(L, 1);
    v->~value();
    return 0;
}

static int lua_function_wrapper(lua_State *L)
{
    lua_script_system *sys =
        (lua_script_system *)lua_touserdata(L, lua_upvalueindex(1));
    value *val = (value *)lua_touserdata(L, lua_upvalueindex(2));

    lua_script_args args(L, 1, lua_gettop(L), sys);

    // Attempt to find the calling script instance by inspecting the call stack
    script_instance *instance = nullptr;
    lua_Debug ar;

    // Level 1 is the caller (the Lua function calling this C function)
    if (lua_getstack(L, 1, &ar))
    {
        if (lua_getinfo(L, "f", &ar)) // Pushes function onto stack
        {
            const char *name = nullptr;
            for (int i = 1; (name = lua_getupvalue(L, -1, i)) != nullptr; ++i)
            {
                if (strcmp(name, "_ENV") == 0)
                {
                    // Stack: ... func, val (which is the env table)
                    if (lua_istable(L, -1))
                    {
                        lua_getfield(L, -1, "__instance");
                        if (lua_islightuserdata(L, -1))
                        {
                            instance = (script_instance *)lua_touserdata(L, -1);
                        }
                        lua_pop(L, 1); // pop __instance
                    }
                    lua_pop(L, 1); // pop _ENV value
                    break;
                }
                lua_pop(L, 1); // pop other upvalue
            }
            lua_pop(L, 1); // Pop calling function
        }
    }

    if (val)
        val->call(sys, instance, &args);

    return args.returns_count;
}

static void
push_callable_value(lua_State *L, lua_script_system *sys, const value &v)
{
    void *u = lua_newuserdata(L, sizeof(value));
    new (u) value(v);

    if (luaL_newmetatable(L, "zabato.value_wrapper"))
    {
        lua_pushcfunction(L, value_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);

    lua_pushlightuserdata(L, sys); // Upvalue 1: System
    lua_pushvalue(L, -2);          // Upvalue 2: Value Wrapper (Userdata)
    lua_pushcclosure(L, lua_function_wrapper, 2);

    lua_remove(L, -2); // Remove userdata, kept in closure
}

static void push_value_to_lua(lua_State *L, const value &val)
{
    if (val.is_nil())
    {
        lua_pushnil(L);
        return;
    }

    // If it is a lua_value belonging to THIS state, just push ref
    if (val.impl->get_type_info().is_exactly(lua_value::TYPE))
    {
        auto *lv = static_cast<lua_value *>(val.impl.get());
        if (lv->L == L)
        {
            lv->push();
            return;
        }
    }

    switch (val.type())
    {
    case value_type::BOOLEAN:
        lua_pushboolean(L, val.as_bool());
        break;
    case value_type::NUMBER:
        lua_pushnumber(L, val.as_number());
        break;
    case value_type::INTEGER:
        lua_pushinteger(L, val.as_int());
        break;
    case value_type::STRING:
    {
        string s = val.as_string();
        lua_pushlstring(L, s.c_str(), s.length());
        break;
    }
    case value_type::VEC2:
    {
        void *u = lua_newuserdata(L, sizeof(vec2<real>));
        new (u) vec2<real>(val.as_vec2());
        luaL_getmetatable(L, META_VEC2);
        lua_setmetatable(L, -2);
        break;
    }
    case value_type::VEC3:
    {
        void *u = lua_newuserdata(L, sizeof(vec3<real>));
        new (u) vec3<real>(val.as_vec3());
        luaL_getmetatable(L, META_VEC3);
        lua_setmetatable(L, -2);
        break;
    }
    case value_type::VEC4:
    {
        void *u = lua_newuserdata(L, sizeof(vec4<real>));
        new (u) vec4<real>(val.as_vec4());
        luaL_getmetatable(L, META_VEC4);
        lua_setmetatable(L, -2);
        break;
    }
    case value_type::QUAT:
    {
        void *u = lua_newuserdata(L, sizeof(quat<real>));
        new (u) quat<real>(val.as_quat());
        luaL_getmetatable(L, META_QUAT);
        lua_setmetatable(L, -2);
        break;
    }
    case value_type::COLOR:
    {
        void *u = lua_newuserdata(L, sizeof(color));
        new (u) color(val.as_color());
        luaL_getmetatable(L, META_COLOR);
        lua_setmetatable(L, -2);
        break;
    }
    case value_type::MAT3:
    {
        void *u = lua_newuserdata(L, sizeof(mat3<real>));
        new (u) mat3<real>(val.as_mat3());
        luaL_getmetatable(L, META_MAT3);
        lua_setmetatable(L, -2);
        break;
    }
    case value_type::MAT4:
    {
        void *u = lua_newuserdata(L, sizeof(mat4<real>));
        new (u) mat4<real>(val.as_mat4());
        luaL_getmetatable(L, META_MAT4);
        lua_setmetatable(L, -2);
        break;
    }
    case value_type::MAP:
    case value_type::LIST:
    {
        lua_newtable(L);

        // If Native Value, we can iterate
        if (val.impl->get_type_info().is_exactly(native_value::TYPE))
        {
            auto *nv = static_cast<native_value *>(val.impl.get());
            if (nv->m_type == value_type::MAP)
            {
                for (auto it = nv->t_val->begin(); it != nv->t_val->end(); ++it)
                {
                    string_view s = it->key.as_string();
                    lua_pushlstring(L, s.data(), s.length());
                    push_value_to_lua(L, it->value);
                    lua_settable(L, -3);
                }
            }
            else if (nv->m_type == value_type::LIST)
            {
                for (size_t i = 0; i < nv->a_val->size(); ++i)
                {
                    push_value_to_lua(L, (*nv->a_val)[i]);
                    lua_rawseti(L, -2, i + 1);
                }
            }
        }
        else if (val.is_list())
        {
            for (size_t i = 0; i < val.length(); ++i)
            {
                push_value_to_lua(L, val.get_at(i));
                lua_rawseti(L, -2, i + 1);
            }
        }
        else if (val.is_map())
        {
            auto it = val.get_iterator();
            if (it)
            {
                while (it->next())
                {
                    push_value_to_lua(L, it->get_key());
                    push_value_to_lua(L, it->get_value());
                    lua_settable(L, -3);
                }
            }
        }
        break;
    }
    case value_type::FUNCTION:
        if (val.as_function().is_valid()) // Check valid delegate
        {
            if (val.impl->get_type_info().is_exactly(native_value::TYPE))
            {
                push_callable_value(L, get_sys(L), val);
            }
            else
            {
                if (val.as_function().is_valid())
                    lua_rawgeti(L,
                                LUA_REGISTRYINDEX,
                                (intptr_t)val.as_function().get_object());
                else
                    lua_pushnil(L);
            }
        }
        else
            lua_pushnil(L);
        break;
    case value_type::OBJECT:
    {
        pointer<base_object> obj = val.as_object();
        if (obj)
        {
            void *ud = lua_newuserdata(L, sizeof(pointer<base_object>));
            new (ud) pointer<base_object>(obj);
            push_object_metatable(L);
            lua_setmetatable(L, -2);
        }
        else
        {
            lua_pushnil(L);
        }
        break;
    }
    default:
        lua_pushnil(L);
        break;
    }
}

} // namespace zabato