#include <string.h>
#include <zabato/error.hpp>
#include <zabato/game_message.hpp>
#include <zabato/lua.hpp>
#include <zabato/serializer.hpp>
#include <zabato/string.hpp>
#include <zabato/symbol.hpp>
#include <zabato/value.hpp>

namespace zabato
{

static void push_value_to_lua(lua_State *L, const value &val);

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

const rtti lua_script_instance::TYPE("zabato.lua.script_instance",
                                     &script_instance::TYPE);

lua_script_instance::lua_script_instance(
    const shared_ptr<zabato::script> &script_resource,
    lua_State *L,
    int env_ref)
    : script_instance(script_resource), m_L(L), m_env_ref(env_ref)
{
}

lua_script_instance::~lua_script_instance()
{
    if (m_L && m_env_ref != LUA_NOREF)
    {
        luaL_unref(m_L, LUA_REGISTRYINDEX, m_env_ref);
    }
}

void lua_script_instance::update(real dt)
{
    if (m_env_ref == LUA_NOREF)
        return;

    // Get environment
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_env_ref);
    // Get update function from environment
    lua_getfield(m_L, -1, "update");
    if (lua_isfunction(m_L, -1))
    {
        lua_pushnumber(m_L, (float)dt);
        if (lua_pcall(m_L, 1, 0, 0) != LUA_OK)
        {
            report(report_type::error,
                   "Lua update error: %s",
                   lua_tostring(m_L, -1));
            lua_pop(m_L, 1);
        }
    }
    else
    {
        lua_pop(m_L, 1); // pop nil/non-func
    }
    lua_pop(m_L, 1); // pop env
}

void lua_script_instance::on_message(const game_message &msg)
{
    if (m_env_ref == LUA_NOREF)
        return;

    // Get environment
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_env_ref);
    // Get on_message function
    lua_getfield(m_L, -1, "on_message");

    if (lua_isfunction(m_L, -1))
    {
        // Construct message table
        lua_newtable(m_L);

        // msg_id (symbol) -> string
        if (msg.msg_id)
        {
            lua_pushstring(m_L, get_symbol_name(msg.msg_id));
            lua_setfield(m_L, -2, "id");
        }

        // sender_id (uuid) -> string
        string sender = msg.sender_id.to_string();
        lua_pushlstring(m_L, sender.c_str(), sender.length());
        lua_setfield(m_L, -2, "sender");

        // receiver_id (uuid) -> string
        string receiver = msg.receiver_id.to_string();
        lua_pushlstring(m_L, receiver.c_str(), receiver.length());
        lua_setfield(m_L, -2, "receiver");

        // data (value) -> Lua Value
        push_value_to_lua(m_L, msg.data);
        lua_setfield(m_L, -2, "data");

        if (lua_pcall(m_L, 1, 0, 0) != LUA_OK)
        {
            report(report_type::error,
                   "Lua on_message error: %s",
                   lua_tostring(m_L, -1));
            lua_pop(m_L, 1);
        }
    }
    else
    {
        lua_pop(m_L, 1); // pop nil/non-func
    }
    lua_pop(m_L, 1); // pop env
}

void lua_script_instance::set_property(const char *name, real val)
{
    if (m_env_ref == LUA_NOREF)
        return;

    lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_env_ref);
    lua_pushnumber(m_L, (float)val);
    lua_setfield(m_L, -2, name);
    lua_pop(m_L, 1);
}

void lua_script_instance::on_draw_gizmos(gpu &g, bool selected)
{
    if (m_env_ref == LUA_NOREF)
        return;

    // Get environment
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_env_ref);
    // Get on_draw_gizmos function
    lua_getfield(m_L, -1, "on_draw_gizmos");

    if (lua_isfunction(m_L, -1))
    {
        lua_pushlightuserdata(m_L, &g);
        lua_pushboolean(m_L, selected);
        if (lua_pcall(m_L, 2, 0, 0) != LUA_OK)
        {
            report(report_type::error,
                   "Lua on_draw_gizmos error: %s",
                   lua_tostring(m_L, -1));
            lua_pop(m_L, 1);
        }
    }
    else
    {
        lua_pop(m_L, 1); // pop nil/non-func
    }
    lua_pop(m_L, 1); // pop env
}

static void serialize_table(lua_State *L, serializer &stream, int index);

bool lua_script_instance::register_object(serializer &stream) const
{
    if (m_script)
    {
        resource_ref ref("script_res");
        stream.write(ref);
        return true;
    }
    return false;
}

void lua_script_instance::save(serializer &stream) const
{
    resource_ref ref;
    stream.write(ref);
    bool has_data = (m_env_ref != LUA_NOREF);
    stream.write(has_data);
    if (has_data)
    {
        lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_env_ref);
        serialize_table(m_L, stream, -1);
        lua_pop(m_L, 1);
    }
}

enum class LuaType : uint8_t
{
    Nil     = 0,
    Boolean = 1,
    Number  = 2,
    String  = 3,
    Table   = 4
};

static void serialize_value(lua_State *L, serializer &stream, int index);

static void serialize_table(lua_State *L, serializer &stream, int index)
{
    lua_pushvalue(L, index); // Push table on top
    int table_idx = lua_gettop(L);

    uint32_t count = 0;
    lua_pushnil(L);
    while (lua_next(L, table_idx) != 0)
    {
        count++;
        lua_pop(L, 1);
    }

    stream.write(count);

    lua_pushnil(L);
    while (lua_next(L, table_idx) != 0)
    {
        // Stack: table, key, value
        serialize_value(L, stream, -2); // Key
        serialize_value(L, stream, -1); // Value
        lua_pop(L, 1);
    }
    lua_pop(L, 1); // Pop table copy
}

static void serialize_value(lua_State *L, serializer &stream, int index)
{
    int type = lua_type(L, index);
    switch (type)
    {
    case LUA_TNIL:
        stream.write(LuaType::Nil);
        break;
    case LUA_TBOOLEAN:
        stream.write(LuaType::Boolean);
        stream.write((uint8_t)lua_toboolean(L, index));
        break;
    case LUA_TNUMBER:
        stream.write(LuaType::Number);
        stream.write((double)lua_tonumber(L, index));
        break;
    case LUA_TSTRING:
    {
        stream.write(LuaType::String);
        size_t len;
        const char *str = lua_tolstring(L, index, &len);
        string_view s(str, len);
        stream.write(s);
        break;
    }
    case LUA_TTABLE:
        stream.write(LuaType::Table);
        serialize_table(L, stream, index);
        break;
    default:
        // Unsupported types treated as Nil
        stream.write(LuaType::Nil);
        break;
    }
}

static void deserialize_value(lua_State *L, serializer &stream);

static void deserialize_table(lua_State *L, serializer &stream)
{
    lua_newtable(L);
    uint32_t count = 0;
    stream.read(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        deserialize_value(L, stream); // Key
        deserialize_value(L, stream); // Value
        lua_settable(L, -3);
    }
}

static void deserialize_value(lua_State *L, serializer &stream)
{
    LuaType type;
    stream.read(type);

    switch (type)
    {
    case LuaType::Nil:
        lua_pushnil(L);
        break;
    case LuaType::Boolean:
    {
        uint8_t val;
        stream.read(val);
        lua_pushboolean(L, val);
        break;
    }
    case LuaType::Number:
    {
        double val;
        stream.read(val);
        lua_pushnumber(L, val);
        break;
    }
    case LuaType::String:
    {
        string val;
        stream.read(val);
        lua_pushlstring(L, val.c_str(), val.length());
        break;
    }
    case LuaType::Table:
        deserialize_table(L, stream);
        break;
    default:
        lua_pushnil(L);
        break;
    }
}

void lua_script_instance::load(serializer &stream, serializer_link *link)
{
    resource_ref ref;
    stream.read(ref);

    if (stream.get_manager())
    {
        ref.set_manager(stream.get_manager());
        m_script = shared_ptr<zabato::script>(ref.get<zabato::script>());
    }

    if (m_script && m_L)
    {
        string_view src = m_script->source();

        if (luaL_loadbuffer(m_L, src.data(), src.size(), ref.c_path()) ==
            LUA_OK)
        {
            // Set environment
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_env_ref);
            lua_setupvalue(m_L, -2, 1);

            // Run chunk
            if (lua_pcall(m_L, 0, 0, 0) != LUA_OK)
            {
                std::cout << "Script Runtime Error: " << lua_tostring(m_L, -1)
                          << std::endl;
                lua_pop(m_L, 1);
            }
        }
        else
        {
            std::cout << "Script Compile Error: " << lua_tostring(m_L, -1)
                      << std::endl;
            lua_pop(m_L, 1);
        }
    }

    if (!m_L)
        return;

    bool has_data = false;
    stream.read(has_data);

    if (has_data)
    {
        deserialize_table(m_L, stream);

        lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_env_ref);
        lua_getfield(m_L, -1, "on_load");

        if (lua_isfunction(m_L, -1))
        {
            lua_pushvalue(m_L, -3); // Push copy of data table
            if (lua_pcall(m_L, 1, 0, 0) != LUA_OK)
            {
                report(report_type::error,
                       "Error calling on_load: %s",
                       lua_tostring(m_L, -1));
                lua_pop(m_L, 1);
            }
        }
        else
        {
            lua_pop(m_L, 1); // Pop non-function
        }
        lua_pop(m_L, 1); // Pop env
        lua_pop(m_L, 1); // Pop data table
    }
}

lua_script_system::lua_script_system(fs::file_system &fs)
    : script_system(fs), m_L(nullptr)
{
}

lua_script_system::~lua_script_system() { shutdown(); }

bool lua_script_system::initialize()
{
    m_L = luaL_newstate();
    if (!m_L)
        return false;
    luaL_openlibs(m_L);
    return true;
}

void lua_script_system::shutdown()
{
    if (m_L)
    {
        lua_close(m_L);
        m_L = nullptr;
    }
}

void lua_script_system::tick()
{
    if (m_L)
        lua_gc(m_L, LUA_GCSTEP, 0);
}

script_instance *lua_script_system::load_script(const char *filepath,
                                                uuid owner_id)
{
    if (!m_L)
        return nullptr;

    auto file = m_fs.open(filepath, fs::open_mode::read);
    if (!file)
    {
        report(report_type::error, "Failed to load script file: %s", filepath);
        return nullptr;
    }

    // Get file size
    file->seek(0, fs::origin::end);
    size_t size = file->tell();
    file->seek(0, fs::origin::begin);

    // Read file
    vector<char> buffer(size);
    file->read({reinterpret_cast<uint8_t *>(buffer.data()), buffer.size()});
    file->close();

    // Create a new environment table
    lua_newtable(m_L);
    // Set metatable __index to global _G so script can access globals
    lua_newtable(m_L);
    lua_pushglobaltable(m_L);
    lua_setfield(m_L, -2, "__index");
    lua_setmetatable(m_L, -2);

    int env_ref = luaL_ref(m_L, LUA_REGISTRYINDEX); // Stack: empty

    // Load the chunk
    if (luaL_loadbuffer(m_L, buffer.data(), buffer.size(), filepath) != LUA_OK)
    {
        report(report_type::error, "Lua load error: %s", lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
        luaL_unref(m_L, LUA_REGISTRYINDEX, env_ref);
        return nullptr;
    }

    // Set the chunk's environment
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, env_ref);
    lua_setupvalue(m_L, -2, 1);

    // Execute the chunk
    if (lua_pcall(m_L, 0, 0, 0) != LUA_OK)
    {
        report(report_type::error,
               "Lua execution error: %s",
               lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
        luaL_unref(m_L, LUA_REGISTRYINDEX, env_ref);
        return nullptr;
    }

    // Inject owner_id
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, env_ref);
    string uuid_str = owner_id.to_string();
    lua_pushlstring(m_L, uuid_str.c_str(), uuid_str.length());
    lua_setfield(m_L, -2, "owner_id");
    lua_pop(m_L, 1);

    string source_str(buffer.data(), buffer.size());
    auto script_res = make_shared<script>(source_str);

    auto *inst = new lua_script_instance(script_res, m_L, env_ref);

    // Inject instance
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, env_ref);
    lua_pushlightuserdata(m_L, inst);
    lua_setfield(m_L, -2, "__instance");
    lua_pop(m_L, 1);

    return inst;
}

static int lua_function_wrapper(lua_State *L);

// Helper for Value Wrapper GC
static int value_gc(lua_State *L)
{
    value *v = (value *)lua_touserdata(L, 1);
    v->~value();
    return 0;
}

// Helper to push callable value
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

// Function Wrapper for Callbacks
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

void lua_script_system::register_global_function(const string_view &name,
                                                 value cb)
{
    push_callable_value(m_L, this, cb);
    string s(name);
    lua_setglobal(m_L, s.c_str());
}

static int lua_class_index(lua_State *L)
{
    // Stack: Userdata, Key
    lua_getmetatable(L, 1); // Stack: U, K, MT

    // 1. Check Methods
    lua_pushvalue(L, 2); // K
    lua_rawget(L, -2);   // MT[K]
    if (!lua_isnil(L, -1))
        return 1; // Found method
    lua_pop(L, 1);

    // 2. Check Getters
    lua_getfield(L, -1, "__getters"); // Stack: U, K, MT, Getters
    lua_pushvalue(L, 2);              // K
    lua_rawget(L, -2);                // Getters[K]

    if (lua_islightuserdata(L, -1))
    {
        // Call Getter
        // Logic: The stored value is the wrapper closure? No, we store closures
        // in register_class. If we store closures in __getters, we just call
        // it. Let's assume __getters has { "prop": closure }. Stack: ...,
        // GetterClosure
        lua_pushvalue(L, 1); // Push Self
        lua_call(L, 1, 1);   // Call Getter(Self) -> Ret
        return 1;
    }

    return 0; // Nil
}

static int lua_class_newindex(lua_State *L)
{
    // Stack: Userdata, Key, Value
    lua_getmetatable(L, 1); // Stack: U, K, V, MT

    // Check Setters
    lua_getfield(L, -1, "__setters"); // Stack: U, K, V, MT, Setters
    lua_pushvalue(L, 2);              // K
    lua_rawget(L, -2);                // Setters[K]

    if (!lua_isnil(L, -1))
    {
        // Call Setter
        // Stack: ..., SetterClosure
        lua_pushvalue(L, 1); // Self
        lua_pushvalue(L, 3); // Val
        lua_call(L, 2, 0);   // Call Setter(Self, Val)
        return 0;
    }

    return luaL_error(
        L, "Property '%s' is read-only or does not exist.", lua_tostring(L, 2));
}

void lua_script_system::register_class(const script_class_def &def)
{
    if (luaL_newmetatable(m_L, def.class_name))
    {
        // __methods (Convention: Methods stored directly in MT for speed if
        // generic index fails, but since we override __index, we must manually
        // check methods or put them in table) Optimization: Put methods in MT.
        // __index wrapper checks MT first. Correct.

        // __getters table
        lua_newtable(m_L);
        for (const auto &kv : def.properties)
        {
            if (!kv.value.getter.is_nil())
            {
                push_callable_value(m_L, this, kv.value.getter);
                lua_setfield(m_L, -2, kv.value.name);
            }
        }
        lua_setfield(m_L, -2, "__getters");

        // __setters table
        lua_newtable(m_L);
        for (const auto &kv : def.properties)
        {
            if (!kv.value.setter.is_nil())
            {
                push_callable_value(m_L, this, kv.value.setter);
                lua_setfield(m_L, -2, kv.value.name);
            }
        }
        lua_setfield(m_L, -2, "__setters");

        // Methods
        for (const auto &kv : def.methods)
        {
            push_callable_value(m_L, this, kv.value.cb);
            lua_setfield(m_L, -2, kv.value.name);
        }

        // Metamethods (Destructor)
        // Metamethods (Destructor)
        if (!def.destructor.is_nil())
        {
            push_callable_value(m_L, this, def.destructor);
            lua_setfield(m_L, -2, "__gc");
        }

        // Set __index and __newindex
        lua_pushcfunction(m_L, lua_class_index);
        lua_setfield(m_L, -2, "__index");

        lua_pushcfunction(m_L, lua_class_newindex);
        lua_setfield(m_L, -2, "__newindex");
    }
    lua_pop(m_L, 1);

    if (!def.constructor.is_nil())
        register_global_function(def.class_name, def.constructor);
}

void lua_script_system::set_global_var(const char *name,
                                       const script_value &val)
{
    push_value(val);
    lua_setglobal(m_L, name);
}

class lua_iterator;
class lua_value : public ivalue
{
public:
    static const rtti TYPE;
    const rtti &get_type_info() const override { return TYPE; }

    lua_State *L;
    int ref_id;

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
        lua_pop(L, 1);

        switch (t)
        {
        case LUA_TNIL:
            return value_type::NIL;
        case LUA_TBOOLEAN:
            return value_type::BOOLEAN;
        case LUA_TNUMBER:
            return value_type::NUMBER;
        case LUA_TSTRING:
            return value_type::STRING;
        case LUA_TTABLE:
            return value_type::MAP;
        case LUA_TFUNCTION:
            return value_type::FUNCTION;
        case LUA_TUSERDATA:
            return value_type::NATIVE_OBJECT;
        default:
            return value_type::NIL;
        }
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

        value res(zabato::make_shared<lua_value>(L, -1, true));
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
            auto it = zabato::make_shared<lua_iterator>(L);

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
        bool v = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return v;
    }

    double as_number() const override
    {
        push();
        double v = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return v;
    }

    int64_t as_int() const override
    {
        push();
        int64_t v = lua_tointeger(L, -1);
        lua_pop(L, 1);
        return v;
    }

    string_view as_string() const override
    {
        push();
        size_t len;
        const char *s = lua_tolstring(L, -1, &len);
        lua_pop(L, 1);
        return string_view(s, len);
    }

    void *as_pointer() const override
    {
        push();
        void *p = lua_touserdata(L, -1);
        lua_pop(L, 1);
        return p;
    }

    intptr_t as_function() const override { return ref_id; }

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
        value res(make_shared<lua_value>(L, -1, true));
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
        value res(zabato::make_shared<lua_value>(L, -1, true));
        lua_pop(L, 2);
        return res;
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
    }
    break;
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
        if (val.as_function() > 0)
            lua_rawgeti(L, LUA_REGISTRYINDEX, val.as_function());
        else
            lua_pushnil(L);
        break;
    default:
        lua_pushnil(L);
        break;
    }
}

script_value lua_script_system::to_value(int index)
{
    return script_value(
        zabato::make_shared<zabato::lua_value>(m_L, index, true));
}

void lua_script_system::push_value(const script_value &val)
{
    push_value_to_lua(m_L, val);
}

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
            value res(make_shared<lua_value>(m_main_L, -1, true));
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
            value res(make_shared<lua_value>(m_main_L, -1, true));
            lua_pop(m_main_L, 1);
            return res;
        }
        return zabato::value();
    }
};

const rtti lua_value::TYPE("zabato.lua.value", &ivalue::TYPE);

} // namespace zabato
