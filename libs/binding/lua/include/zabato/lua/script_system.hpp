#pragma once

#include <stdio.h>

#include <zabato/lua/c.hpp>
#include <zabato/script.hpp>

namespace zabato
{

/**
 * @struct zshader_compilation_result
 * @brief Results from compiling a ZShader script (GLSL code + Reflection).
 */
struct zshader_compilation_result
{
    string glsl_vertex;
    string glsl_fragment;
    string name;

    vector<std::pair<string, string>> uniforms;
    vector<std::pair<string, string>> attributes;
};

/**
 * @brief Lua implementation of script_system.
 */
class lua_script_system : public script_system
{
public:
    lua_script_system(fs::file_system &fs, console &console);
    ~lua_script_system() override;

    friend class lua_script_instance;

    bool initialize() override;
    void shutdown() override;
    void tick() override;

    // ZShader Compiler Integration
    bool compile_zshader(const string &path,
                         zshader_compilation_result &out_result,
                         const string_view &backend = "glsl120");

    fs::file_system &get_file_system() { return m_fs; }
    console &get_console() { return m_console; }

    script_instance *load_script(const char *filepath, uuid owner_id) override;

    object *create_instance(serializer &s) override;
    object *create_instance_xml(xml_serializer &s,
                                tinyxml2::XMLElement &el) override;

    void register_global_function(const string_view &name, value cb) override;
    void register_class(const script_class_def &def) override;
    void set_global_var(const char *name, const script_value &val) override;

    void push_value(const script_value &val);
    script_value to_value(int index);

private:
    lua_State *m_L;
};

} // namespace zabato