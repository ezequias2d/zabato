#pragma once

#include <zabato/lua/c.hpp>
#include <zabato/script.hpp>

namespace zabato
{
/**
 * @brief Lua implementation of script_instance.
 */
class lua_script_instance : public script_instance
{
public:
    static const rtti TYPE;
    static void reflect(reflection &r);
    const rtti &type() const override { return TYPE; }

    lua_script_instance(const string &path,
                        const shared_ptr<zabato::script> &script_resource,
                        lua_State *L,
                        int env_ref);
    lua_script_instance(lua_State *L);
    virtual ~lua_script_instance();

    void initialize(const controller::context &ctx) override;
    void start() override;
    void update(real dt) override;

    void on_message(const game_message &msg) override;
    void set_property(const char *name, real val) override;
    void on_draw_gizmos(gpu &g, bool selected) override;

    void set_env_var(const string_view &name, const value &v) override;
    value get_env_var(const string_view &name) const override;

    // Object serialization (Resource & Type)
    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    bool register_object(serializer &stream) const override;

    void save_xml(xml_serializer &serializer,
                  tinyxml2::XMLElement &el) const override;
    void load_xml(xml_serializer &serializer,
                  tinyxml2::XMLElement &el) override;

    int env_ref() const { return m_env_ref; }

    lua_script_instance();

private:
    string m_script_path;
    lua_State *m_L;
    int m_env_ref;
};
} // namespace zabato