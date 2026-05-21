#include "lua_value.hpp"

#include <zabato/error.hpp>
#include <zabato/game_message.hpp>
#include <zabato/lua/script_instance.hpp>
#include <zabato/lua/script_system.hpp>
#include <zabato/serializer.hpp>
#include <zabato/spatial.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti lua_script_instance::TYPE("zabato.lua.script",
                                     &script_instance::TYPE,
                                     lua_script_instance::reflect);

void lua_script_instance::reflect(reflection &r)
{
    script_instance::reflect(r);
}

lua_script_instance::lua_script_instance(lua_State *L)
    : script_instance(nullptr), m_script_path(), m_L(L), m_env_ref(LUA_NOREF)
{
}

lua_script_instance::lua_script_instance(
    const string &path,
    const shared_ptr<zabato::script> &script_resource,
    lua_State *L,
    int env_ref)
    : script_instance(script_resource), m_script_path(path), m_L(L),
      m_env_ref(env_ref)
{
}

lua_script_instance::~lua_script_instance()
{
    if (m_L && m_env_ref != LUA_NOREF)
    {
        luaL_unref(m_L, LUA_REGISTRYINDEX, m_env_ref);
    }
}

void lua_script_instance::initialize(const controller::context &ctx)
{

    if (m_env_ref != LUA_NOREF && m_L && m_object)
    {
        lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_env_ref);

        // Inject 'object'
        push_value_to_lua(m_L, value(m_object));
        lua_setfield(m_L, -2, "object");

        // Inject 'world' if available
        if (m_object->is_derived(spatial::TYPE))
        {
            auto *sp = static_cast<spatial *>(m_object);
            if (auto *w = sp->get_world())
            {
                push_value_to_lua(m_L, value(w));
                lua_setfield(m_L, -2, "world");
            }
        }

        lua_pop(m_L, 1);
    }
}

void lua_script_instance::start() {}

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
        lua_pushstring(m_L, get_symbol_name(msg.msg_id));
        lua_setfield(m_L, -2, "id");

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

void lua_script_instance::set_env_var(const string_view &name, const value &v)
{
    if (m_env_ref == LUA_NOREF || !m_L)
        return;

    lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_env_ref);
    push_value_to_lua(m_L, v);
    string name_str(name.data(), name.size());
    lua_setfield(m_L, -2, name_str.c_str());
    lua_pop(m_L, 1);
}

value lua_script_instance::get_env_var(const string_view &name) const
{
    if (m_env_ref == LUA_NOREF || !m_L)
        return value();

    lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_env_ref);
    string name_str(name.data(), name.size());
    lua_getfield(m_L, -1, name_str.c_str());

    if (lua_isnil(m_L, -1))
    {
        lua_pop(m_L, 2);
        return value();
    }

    lua_script_system *sys = nullptr;
    lua_getfield(m_L, LUA_REGISTRYINDEX, SYS_REG_KEY);
    sys = static_cast<lua_script_system *>(lua_touserdata(m_L, -1));
    lua_pop(m_L, 1);

    value result;
    if (sys)
        result = sys->to_value(-1);

    lua_pop(m_L, 2);
    return result;
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

void lua_script_instance::save_xml(xml_serializer &serializer,
                                   tinyxml2::XMLElement &el) const
{
    script_instance::save_xml(serializer, el);
    if (!m_script_path.empty())
        el.SetAttribute("src", m_script_path.c_str());
}

void lua_script_instance::load_xml(xml_serializer &serializer,
                                   tinyxml2::XMLElement &el)
{
    object::load_xml(serializer, el);
    const char *src = el.Attribute("src");

    if (src)
    {
        m_script_path = src;
        if (serializer.get_manager())
        {
            auto res =
                serializer.get_manager()->load<zabato::script>(string(src));
            if (!res.has_error())
                m_script = res.value;
        }

        if (m_script && m_L)
        {
            string path        = src;
            string_view source = m_script->source();

            // Ensure environment exists
            if (m_env_ref == LUA_NOREF)
            {
                lua_newtable(m_L);
                lua_newtable(m_L);
                lua_pushglobaltable(m_L);
                lua_setfield(m_L, -2, "__index");
                lua_setmetatable(m_L, -2);
                m_env_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
            }

            if (luaL_loadbuffer(
                    m_L, source.data(), source.size(), path.c_str()) == LUA_OK)
            {
                // Set environment for the chunk
                lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_env_ref);
                lua_setupvalue(m_L, -2, 1);

                if (lua_pcall(m_L, 0, 0, 0) != LUA_OK)
                {
                    report(report_type::error,
                           "Script XML Load Error: %s",
                           lua_tostring(m_L, -1));
                    lua_pop(m_L, 1);
                }
                else
                {
                    // Inject owner/instance
                    lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_env_ref);
                    string uuid_str = id().to_string();
                    lua_pushlstring(m_L, uuid_str.c_str(), uuid_str.length());
                    lua_setfield(m_L, -2, "owner_id");

                    // Inject 'object' (the entity this script is attached to)
                    if (m_object)
                    {
                        push_value_to_lua(m_L, value(m_object));
                        lua_setfield(m_L, -2, "object");
                    }

                    lua_pushlightuserdata(m_L, this);
                    lua_setfield(m_L, -2, "__instance");
                    lua_pop(m_L, 1);
                }
            }
            else
            {
                report(report_type::error,
                       "Script XML Compile Error: %s",
                       lua_tostring(m_L, -1));
                lua_pop(m_L, 1);
            }
        }
    }
}

enum class LuaType : uint8_t
{
    Nil     = 0,
    Boolean = 1,
    Number  = 2,
    String  = 3,
    Table   = 4,
    Vec2    = 5,
    Vec3    = 6,
    Vec4    = 7,
    Quat    = 8,
    Color   = 9,
    Mat3    = 10,
    Mat4    = 11
};

static void deserialize_table(lua_State *L, serializer &stream);

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
    case LuaType::Vec2:
    {
        vec2<real> val;
        stream.read(val);
        void *u = lua_newuserdata(L, sizeof(vec2<real>));
        new (u) vec2<real>(val);
        luaL_getmetatable(L, META_VEC2);
        lua_setmetatable(L, -2);
        break;
    }
    case LuaType::Vec3:
    {
        vec3<real> val;
        stream.read(val);
        void *u = lua_newuserdata(L, sizeof(vec3<real>));
        new (u) vec3<real>(val);
        luaL_getmetatable(L, META_VEC3);
        lua_setmetatable(L, -2);
        break;
    }
    case LuaType::Vec4:
    {
        vec4<real> val;
        stream.read(val);
        void *u = lua_newuserdata(L, sizeof(vec4<real>));
        new (u) vec4<real>(val);
        luaL_getmetatable(L, META_VEC4);
        lua_setmetatable(L, -2);
        break;
    }
    case LuaType::Quat:
    {
        quat<real> val;
        stream.read(val);
        void *u = lua_newuserdata(L, sizeof(quat<real>));
        new (u) quat<real>(val);
        luaL_getmetatable(L, META_QUAT);
        lua_setmetatable(L, -2);
        break;
    }
    case LuaType::Color:
    {
        color val;
        stream.read(val);
        void *u = lua_newuserdata(L, sizeof(color));
        new (u) color(val);
        luaL_getmetatable(L, META_COLOR);
        lua_setmetatable(L, -2);
        break;
    }
    case LuaType::Mat3:
    {
        mat3<real> val;
        stream.read(val);
        void *u = lua_newuserdata(L, sizeof(mat3<real>));
        new (u) mat3<real>(val);
        luaL_getmetatable(L, META_MAT3);
        lua_setmetatable(L, -2);
        break;
    }
    case LuaType::Mat4:
    {
        mat4<real> val;
        stream.read(val);
        void *u = lua_newuserdata(L, sizeof(mat4<real>));
        new (u) mat4<real>(val);
        luaL_getmetatable(L, META_MAT4);
        lua_setmetatable(L, -2);
        break;
    }
    default:
        lua_pushnil(L);
        break;
    }
}

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

void lua_script_instance::load(serializer &stream, serializer_link *link)
{
    resource_ref ref;
    stream.read(ref);

    if (stream.get_manager())
    {
        ref.set_manager(stream.get_manager());
        m_script = ref.get<zabato::script>();
        if (ref.path().length() > 0)
            m_script_path = string(ref.path());
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
                report(report_type::error,
                       "Script Runtime Error: %s",
                       lua_tostring(m_L, -1));
                lua_pop(m_L, 1);
            }
        }
        else
        {
            report(report_type::error,
                   "Script Compile Error: %s",
                   lua_tostring(m_L, -1));
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
    case LUA_TUSERDATA:
    {
        if (auto *v2 = (vec2<real> *)luaL_testudata(L, index, META_VEC2))
        {
            stream.write(LuaType::Vec2);
            stream.write(*v2);
        }
        else if (auto *v3 = (vec3<real> *)luaL_testudata(L, index, META_VEC3))
        {
            stream.write(LuaType::Vec3);
            stream.write(*v3);
        }
        else if (auto *v4 = (vec4<real> *)luaL_testudata(L, index, META_VEC4))
        {
            stream.write(LuaType::Vec4);
            stream.write(*v4);
        }
        else if (auto *q = (quat<real> *)luaL_testudata(L, index, META_QUAT))
        {
            stream.write(LuaType::Quat);
            stream.write(*q);
        }
        else if (auto *c = (color *)luaL_testudata(L, index, META_COLOR))
        {
            stream.write(LuaType::Color);
            stream.write(*c);
        }
        else if (auto *m3 = (mat3<real> *)luaL_testudata(L, index, META_MAT3))
        {
            stream.write(LuaType::Mat3);
            stream.write(*m3);
        }
        else if (auto *m4 = (mat4<real> *)luaL_testudata(L, index, META_MAT4))
        {
            stream.write(LuaType::Mat4);
            stream.write(*m4);
        }
        else
        {
            stream.write(LuaType::Nil);
        }
        break;
    }
    default:
        // Unsupported types treated as Nil
        stream.write(LuaType::Nil);
        break;
    }
}

void lua_script_instance::save(serializer &stream) const
{
    resource_ref ref;
    if (!m_script_path.empty())
        ref.set_path(m_script_path.c_str());

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
} // namespace zabato