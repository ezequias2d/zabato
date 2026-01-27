#include "lua_value.hpp"

#include <zabato/error.hpp>
#include <zabato/fs.hpp>
#include <zabato/lua/openlibs.hpp>
#include <zabato/lua/script_instance.hpp>
#include <zabato/lua/script_system.hpp>

namespace zabato
{
lua_script_system::lua_script_system(fs::file_system &fs, console &console)
    : script_system(fs, console), m_L(nullptr)
{
}

lua_script_system::~lua_script_system() { shutdown(); }

bool lua_script_system::initialize()
{
    m_L = luaL_newstate();
    if (!m_L)
        return false;

    luaopen_zabato_base(m_L);

    luaL_requiref(m_L, "string", luaopen_string, 1);
    lua_pop(m_L, 1);
    luaL_requiref(m_L, "table", luaopen_table, 1);
    lua_pop(m_L, 1);
    luaL_requiref(m_L, "math", luaopen_math, 1);
    lua_pop(m_L, 1);
    luaL_requiref(m_L, "coroutine", luaopen_coroutine, 1);
    lua_pop(m_L, 1);
    luaL_requiref(m_L, "utf8", luaopen_utf8, 1);
    lua_pop(m_L, 1);

    luaL_requiref(m_L, "io", luaopen_zabato_io, 1);
    lua_pop(m_L, 1);
    luaL_requiref(m_L, "os", luaopen_zabato_os, 1);
    lua_pop(m_L, 1);
    luaL_requiref(m_L, "package", luaopen_zabato_package, 1);
    lua_pop(m_L, 1);
    luaL_requiref(m_L, "console", luaopen_zabato_console, 1);
    lua_pop(m_L, 1);

    // Registry setup
    lua_pushlightuserdata(m_L, this);
    lua_setfield(m_L, LUA_REGISTRYINDEX, SYS_REG_KEY);
    luaopen_zabato_math(m_L);

    // Register factories
    if (object::s_factory)
        object::s_factory->add(lua_script_instance::TYPE.name(),
                               object::factory_delegate::from_method<
                                   lua_script_system,
                                   &lua_script_system::create_instance>(this));

    if (object::s_factory_xml)
        object::s_factory_xml->add(
            lua_script_instance::TYPE.name(),
            object::factory_delegate_xml::from_method<
                lua_script_system,
                &lua_script_system::create_instance_xml>(this));

    return true;
}

object *lua_script_system::create_instance(serializer &s)
{
    return new lua_script_instance(m_L);
}

object *lua_script_system::create_instance_xml(xml_serializer &s,
                                               tinyxml2::XMLElement &el)
{
    auto *inst = new lua_script_instance(m_L);
    inst->load_xml(s, el);
    return inst;
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
    lua_newtable(m_L); /* Stack: [env] */

    // Set metatable __index to global _G so script can access globals
    lua_newtable(m_L);                /* Stack: [env, metatable] */
    lua_pushglobaltable(m_L);         /* Stack: [env, metatable, global] */
    lua_setfield(m_L, -2, "__index"); /* metatable.__index = global */
    lua_setmetatable(m_L, -2);        /* setmetatable(env, metatable) */

    /* Stack: [env] */
    int env_ref = luaL_ref(m_L, LUA_REGISTRYINDEX); /* Store env in registry */

    // Load the chunk
    if (luaL_loadbufferx(m_L, buffer.data(), buffer.size(), filepath, "t") !=
        LUA_OK)
    {
        report(report_type::error, "Lua load error: %s", lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
        luaL_unref(m_L, LUA_REGISTRYINDEX, env_ref);
        return nullptr;
    }
    /* Stack: [chunk] */

    /* Apply environment */
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, env_ref); /* Stack: [chunk, env] */
    lua_setupvalue(m_L, -2, 1); /* Set _ENV (upvalue 1 of chunk) */
    /* Stack: [chunk] */

    /* Execute the chunk */
    if (lua_pcall(m_L, 0, 0, 0) != LUA_OK)
    {
        report(report_type::error,
               "Lua execution error: %s",
               lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
        luaL_unref(m_L, LUA_REGISTRYINDEX, env_ref);
        return nullptr;
    }

    /* Inject metadata into environment */
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, env_ref); /* Stack: [env] */

    string uuid_str = owner_id.to_string();
    lua_pushlstring(m_L, uuid_str.c_str(), uuid_str.length());
    lua_setfield(m_L, -2, "owner_id"); /* Stack: [env] */

    /* Create script */
    string source_str(buffer.data(), buffer.size());
    auto script_res = make_shared<script>(source_str);
    auto *inst = new lua_script_instance(filepath, script_res, m_L, env_ref);

    /* Inject instance */
    lua_pushlightuserdata(m_L, inst);
    lua_setfield(m_L, -2, "__instance");
    lua_pop(m_L, 1);

    return inst;
}

static void push_value_to_lua(lua_State *L, const value &val);

void lua_script_system::register_global_function(const string_view &name,
                                                 value cb)
{
    push_callable_value(m_L, this, cb);
    string s(name);
    lua_setglobal(m_L, s.c_str());
}

static int lua_class_index(lua_State *L)
{
    /* Stack: Userdata, Key */
    lua_getmetatable(L, 1); /* Stack: U, K, MT */

    /* 1. Check Methods */
    lua_pushvalue(L, 2); /* K */
    lua_rawget(L, -2);   /* MT[K] */
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
        /* __getters table */
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

        /* __setters table */
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

        /* Methods */
        for (const auto &kv : def.methods)
        {
            push_callable_value(m_L, this, kv.value.cb);
            lua_setfield(m_L, -2, kv.value.name);
        }

        /* Destructor (__gc) */
        if (!def.destructor.is_nil())
        {
            push_callable_value(m_L, this, def.destructor);
            lua_setfield(m_L, -2, "__gc");
        }

        /* Metamethods */
        lua_pushcfunction(m_L, lua_class_index);
        lua_setfield(m_L, -2, "__index");

        lua_pushcfunction(m_L, lua_class_newindex);
        lua_setfield(m_L, -2, "__newindex");

        protect_metatable(m_L);
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

script_value lua_script_system::to_value(int index)
{
    return script_value(static_cast<shared_ptr<ivalue>>(
        make_shared<lua_value>(m_L, index, true)));
}

void lua_script_system::push_value(const value &val)
{
    push_value_to_lua(m_L, val);
}

bool lua_script_system::compile_zshader(const string &path,
                                        zshader_compilation_result &out_result,
                                        const string_view &backend)
{
    if (!m_L)
        return false;

    int top = lua_gettop(m_L);

    /* Require 'zshader' */
    lua_getglobal(m_L, "require");
    lua_pushstring(m_L, "zshader");
    if (lua_pcall(m_L, 1, 1, 0) != LUA_OK)
    {
        report_error(error_code::failed_to_require_zshader,
                     lua_tostring(m_L, -1));
        lua_settop(m_L, top);
        return false;
    }
    /* Stack: [zshader_table] */

    if (!lua_istable(m_L, -1))
    {
        report_error(error_code::failed_to_require_zshader,
                     "'zshader' module did not return a table");
        lua_settop(m_L, top);
        return false;
    }

    /* Create Isolated Environment for Zshader */
    lua_newtable(m_L); /* Stack: [zshader_table, env] */

    lua_newtable(m_L);                /* Stack: [zshader_table, env, mt] */
    lua_pushglobaltable(m_L);         /* [zshader_table, env, mt, _G] */
    lua_setfield(m_L, -2, "__index"); /* mt.__index = _G */
    lua_setmetatable(m_L, -2);        /* env.__metatable = mt */

    /* Stack: [zshader_table, env] */

    /* Unpacks zshader contents into 'env' */
    lua_pushnil(m_L);
    while (lua_next(m_L, -3) != 0)
    {
        /* Stack: [zshader_table, env, key, value] */

        lua_pushvalue(m_L, -2); /* Copy Key */
        lua_pushvalue(m_L, -2); /* Copy Value */
        lua_settable(m_L, -5);  /* Set into 'env' */

        lua_pop(m_L, 1); /* Pop original value */
    }
    /* Stack: [zshader_table, env] */

    auto file = m_fs.open(path, fs::open_mode::read);
    if (!file)
    {
        report(
            report_type::error, "Failed to open shader file: %s", path.c_str());
        lua_settop(m_L, top);
        return false;
    }

    file->seek(0, fs::origin::end);
    size_t size = file->tell();
    file->seek(0, fs::origin::begin);

    vector<uint8_t> buffer(size);
    file->read({buffer.data(), buffer.size()});
    file->close();
    delete file;

    string chunkname = "@";
    chunkname += path;
    if (luaL_loadbufferx(m_L,
                         (const char *)buffer.data(),
                         buffer.size(),
                         chunkname.c_str(),
                         "t") != LUA_OK)
    {
        report(report_type::error,
               "Shader %s failed to load: %s",
               path.c_str(),
               lua_tostring(m_L, -1));
        lua_settop(m_L, top);
        return false;
    }
    /* Stack: [zshader_lib, env, user_chunk] */

    lua_pushvalue(m_L, -2);     /* Stack: [zshader_lib, env, user_chunk, env] */
    lua_setupvalue(m_L, -2, 1); /* Set env as upvalue 1 */

    if (lua_pcall(m_L, 0, 1, 0) != LUA_OK)
    {
        report(report_type::error,
               "Failed to compile shader %s: %s",
               path.c_str(),
               lua_tostring(m_L, -1));
        lua_settop(m_L, top);
        return false;
    }
    /* Stack: [zshader_lib, env, shader_def_table] */

    if (!lua_istable(m_L, -1))
    {
        report(report_type::error,
               "Shader %s failed to compile: expected a table, got %s",
               path.c_str(),
               lua_tostring(m_L, -1));
        lua_settop(m_L, top);
        return false;
    }

    out_result.name = path;

    /* Compile shader */
    lua_getfield(m_L, -3, "compile"); /* Get from zshader_lib[compile] */
    lua_pushvalue(m_L, -2);           /* Push ShaderDef */

    if (lua_pcall(m_L, 1, 1, 0) != LUA_OK)
    {
        report(report_type::error,
               "Failed to compile shader %s: %s",
               path.c_str(),
               lua_tostring(m_L, -1));
        lua_settop(m_L, top);
        return false;
    }
    /* Stack: [zshader_lib, env, shader_def_table, compiled_shader] */

    /* Extract Uniforms */
    lua_getfield(m_L, -1, "uniforms");
    if (lua_istable(m_L, -1))
    {
        lua_pushnil(m_L);
        while (lua_next(m_L, -2) != 0)
        {
            if (lua_isstring(m_L, -2) && lua_isstring(m_L, -1))
            {
                out_result.uniforms.push_back(
                    {lua_tostring(m_L, -2), lua_tostring(m_L, -1)});
            }
            lua_pop(m_L, 1);
        }
    }
    lua_pop(m_L, 1);

    /* Transpile */
    lua_getfield(m_L, -3, "transpile"); // ZShader.transpile
    lua_pushvalue(m_L, -2);             // CompiledShader
    lua_pushlstring(m_L, backend.data(), backend.size());
    if (lua_pcall(m_L, 2, 1, 0) != LUA_OK)
    {
        report(report_type::error,
               "Failed to transpile shader %s: %s",
               path.c_str(),
               lua_tostring(m_L, -1));
        lua_settop(m_L, top);
        return false;
    }

    string full_source = lua_tostring(m_L, -1);

    // Split based on marker
    string marker    = "// --- FRAGMENT ---";
    size_t split_pos = full_source.find(marker);
    if (split_pos != string::npos)
    {
        out_result.glsl_vertex = full_source.substr(0, split_pos);
        out_result.glsl_fragment =
            "#version 120\n" + full_source.substr(split_pos + marker.length());
    }
    else
    {
        out_result.glsl_vertex   = full_source;
        out_result.glsl_fragment = ""; // fallback
    }

    lua_settop(m_L, top);
    return true;
}

} // namespace zabato