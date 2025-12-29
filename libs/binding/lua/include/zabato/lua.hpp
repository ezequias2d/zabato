#pragma once

#include <zabato/resource.hpp>
#include <zabato/script.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/vector.hpp>

extern "C"
{
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace zabato
{

class lua_script_system;

/**
 * @brief Lua implementation of script_instance.
 */
class lua_script_instance : public script_instance
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }

    lua_script_instance(const shared_ptr<zabato::script> &script_resource,
                        lua_State *L,
                        int env_ref);
    ~lua_script_instance() override;

    void update(real dt) override;
    void on_message(const game_message &msg) override;
    void set_property(const char *name, real val) override;
    void on_draw_gizmos(gpu &g, bool selected) override;

    // Object serialization (Resource & Type)
    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    bool register_object(serializer &stream) const override;

private:
    lua_State *m_L;
    int m_env_ref;
};

/**
 * @brief Lua implementation of script_system.
 */
class lua_script_system : public script_system
{
public:
    lua_script_system(fs::file_system &fs);
    ~lua_script_system() override;

    bool initialize() override;
    void shutdown() override;
    void tick() override;

    script_instance *load_script(const char *filepath, uuid owner_id) override;

    void register_global_function(const string_view &name, value cb) override;
    void register_class(const script_class_def &def) override;
    void set_global_var(const char *name, const script_value &val) override;

    void push_value(const script_value &val);
    script_value to_value(int index);

private:
    lua_State *m_L;
};

} // namespace zabato
