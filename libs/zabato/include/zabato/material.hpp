#pragma once

#include <zabato/material_params.hpp>
#include <zabato/script.hpp>
#include <zabato/shader_asset.hpp>
#include <zabato/vector.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

class material : public resource
{
public:
    static constexpr chunk_id CHUNK_ID = chunk_id("MATL");
    static const rtti TYPE;
    virtual const rtti &type() const override { return TYPE; }

    material();
    virtual ~material();

    shared_ptr<shader_asset> get_shader() const
    {
        return m_shader.get<shader_asset>();
    }

    void set_shader_path(string_view path) { m_shader.set_path(path); }
    string_view get_shader_path() const { return m_shader.path(); }

    void set_script_system(script_system *system) { m_script_system = system; }

    script_system *get_script_system() const { return m_script_system; }

    void set_resource_manager(resource_manager *mgr)
    {
        m_shader.set_manager(mgr);
        for (auto &texture : m_textures)
            texture.tex.set_manager(mgr);
    }

    /**
     * @brief Set a material parameter.
     *
     * @param name Parameter name.
     * @param val Parameter value.
     */
    void set_float(const string_view &name, real val);
    void set_int(const string_view &name, int val);
    void set_bool(const string_view &name, bool val);
    void set_vec2(const string_view &name, const vec2<real> &val);
    void set_vec3(const string_view &name, const vec3<real> &val);
    void set_vec4(const string_view &name, const vec4<real> &val);
    void set_color(const string_view &name, const color &val);
    void set_texture(const string_view &name, const resource_ref &texture);

    /**
     * @brief Helper to find/create param
     *
     * @param name Parameter name.
     * @param type Parameter type.
     * @return material_parameter&
     */
    material_parameter &ensure_param(const string_view &name, param_type type);

    /**
     * @brief Get material parameters.
     *
     * @return vector<material_parameter>&
     */
    vector<material_parameter> &parameters() { return m_parameters; }

    /**
     * @brief Get material parameters.
     *
     * @return const vector<material_parameter>&
     */
    const vector<material_parameter> &parameters() const
    {
        return m_parameters;
    }

    /**
     * @brief Get material texture slots.
     *
     * @return vector<texture_slot>&
     */
    vector<texture_slot> &textures() { return m_textures; }

    /**
     * @brief Get material texture slots.
     *
     * @return const vector<texture_slot>&
     */
    const vector<texture_slot> &textures() const { return m_textures; }

    /**
     * @brief Apply material state to GPU.
     *
     * @param g GPU context.
     */
    void apply(gpu &g);

    /**
     * @brief Get material dependencies.
     *
     * @return vector<shared_ptr<resource>>
     */
    vector<shared_ptr<resource>> &dependencies() { return m_dependencies; }

    void release_dependencies() { m_dependencies.clear(); }

    /**
     * @brief Get material render state.
     *
     * @return render_state&
     */
    render_state &state() { return m_state; }

    /**
     * @brief Get material render state.
     *
     * @return const render_state&
     */
    const render_state &state() const { return m_state; }

    void save_xml(xml_serializer &serializer, tinyxml2::XMLElement &element);
    void load_xml(xml_serializer &serializer, tinyxml2::XMLElement &element);

private:
    resource_ref m_shader;
    render_state m_state;
    class script_system *m_script_system;
    vector<material_parameter> m_parameters;
    vector<texture_slot> m_textures;
    vector<shared_ptr<resource>> m_dependencies;
};

template <typename T>
inline result<void> serialize(ice_writer &writer, const vector<T> &vec)
{
    ice_uint32_t count = (uint32_t)vec.size();
    auto res           = writer.write(&count, sizeof(count));
    if (res.has_error())
        return res.error;

    for (const auto &item : vec)
    {
        auto r = serialize(writer, item);
        if (r.has_error())
            return r.error;
    }
    return {};
}

template <typename T>
inline result<void> deserialize(ice_reader &reader, vector<T> &vec)
{
    ice_uint32_t count;
    if (reader.read(&count, sizeof(count)) != sizeof(count))
        return report_error(error_code::unable_to_read);

    vec.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
        auto r = deserialize(reader, vec[i]);
        if (r.has_error())
            return r.error;
    }
    return {};
}

// ----------------------------------------------------------------------------
// Struct Serialization
// ----------------------------------------------------------------------------

#pragma pack(push, 1)
struct ICE_RENDER_STATE
{
    ice_int8_t depth_test;
    ice_uint8_t depth_compare;
    ice_int8_t depth_write;

    ice_int8_t blend;
    ice_uint8_t blend_src;
    ice_uint8_t blend_dst;

    ice_int8_t alpha_test;
    ice_uint8_t alpha_compare;
    ice_real alpha_ref;

    ice_int8_t cull_face;
    ice_uint8_t cull_mode;

    ice_uint8_t poly_mode;
    ice_int8_t poly_offset;
    ice_real poly_offset_factor;
    ice_real poly_offset_units;

    ICE_RENDER_STATE() {}
    ICE_RENDER_STATE(const render_state &s)
    {
        depth_test    = s.depth_test;
        depth_compare = (uint8_t)s.depth_compare;
        depth_write   = s.depth_write;

        blend     = s.blend;
        blend_src = (uint8_t)s.blend_src;
        blend_dst = (uint8_t)s.blend_dst;

        alpha_test    = s.alpha_test;
        alpha_compare = (uint8_t)s.alpha_compare;
        alpha_ref     = s.alpha_ref;

        cull_face = s.cull_face;
        cull_mode = (uint8_t)s.cull_mode;

        poly_mode          = (uint8_t)s.poly_mode;
        poly_offset        = s.poly_offset;
        poly_offset_factor = s.poly_offset_factor;
        poly_offset_units  = s.poly_offset_units;
    }

    void to_state(render_state &s) const
    {
        s.depth_test    = depth_test;
        s.depth_compare = (depth_func)(uint8_t)depth_compare;
        s.depth_write   = depth_write;

        s.blend     = blend;
        s.blend_src = (blend_factor)(uint8_t)blend_src;
        s.blend_dst = (blend_factor)(uint8_t)blend_dst;

        s.alpha_test    = alpha_test;
        s.alpha_compare = (alpha_func)(uint8_t)alpha_compare;
        s.alpha_ref     = alpha_ref;

        s.cull_face = cull_face;
        s.cull_mode = (cull_face_mode)(uint8_t)cull_mode;

        s.poly_mode          = (polygon_mode)(uint8_t)poly_mode;
        s.poly_offset        = poly_offset;
        s.poly_offset_factor = poly_offset_factor;
        s.poly_offset_units  = poly_offset_units;
    }
};

#pragma pack(pop)

template <>
inline result<void> serialize(ice_writer &writer, const render_state &s)
{
    ICE_RENDER_STATE ice_state(s);
    auto res = writer.write(&ice_state, sizeof(ice_state));
    if (res.has_error())
        return res.error;
    return {};
}

template <> inline result<void> deserialize(ice_reader &reader, render_state &s)
{
    ICE_RENDER_STATE ice_state;
    if (reader.read(&ice_state, sizeof(ice_state)) != sizeof(ice_state))
        return report_error(error_code::unable_to_read);
    ice_state.to_state(s);
    return {};
}

template <>
inline result<void> serialize(ice_writer &writer, const material &mat)
{
    // 1. Shader Path
    string shader_path = string(mat.get_shader_path());
    auto r             = serialize(writer, shader_path);
    if (r.has_error())
        return r.error;

    // 2. Render State
    r = serialize(writer, mat.state());
    if (r.has_error())
        return r.error;

    // 3. Parameters
    r = serialize(writer, mat.parameters());
    if (r.has_error())
        return r.error;

    // 4. Textures
    r = serialize(writer, mat.textures());
    if (r.has_error())
        return r.error;

    return {};
}

template <> inline result<void> deserialize(ice_reader &reader, material &mat)
{
    // 1. Shader Path
    string shader_path;
    auto r = deserialize(reader, shader_path);
    if (r.has_error())
        return r.error;
    mat.set_shader_path(shader_path);

    // 2. Render State
    r = deserialize(reader, mat.state());
    if (r.has_error())
        return r.error;

    // 3. Parameters
    r = deserialize(reader, mat.parameters());
    if (r.has_error())
        return r.error;

    // 4. Textures
    r = deserialize(reader, mat.textures());
    if (r.has_error())
        return r.error;

    return {};
}

} // namespace zabato
