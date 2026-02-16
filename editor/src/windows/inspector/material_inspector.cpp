#include "preview_scene.hpp"
#include <editor/asset_database.hpp>
#include <editor/core/editor_registry.hpp>
#include <editor/editor.hpp>
#include <tinyxml2.h>
#include <zabato/color.hpp>
#include <zabato/error.hpp>
#include <zabato/gpu.hpp>
#include <zabato/imgui.hpp>
#include <zabato/light.hpp>
#include <zabato/lua/script_system.hpp>
#include <zabato/material.hpp>
#include <zabato/material_params.hpp>
#include <zabato/math.hpp>
#include <zabato/mesh.hpp>
#include <zabato/renderer.hpp>
#include <zabato/resource.hpp>
#include <zabato/shader.hpp>
#include <zabato/shader_asset.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/string.hpp>
#include <zabato/vector.hpp>
#include <zabato/xml_serializer.hpp>

using namespace zabato;

namespace zabato::editor
{

template <typename T> struct combo_pair
{
    T value;
    const char *name;

    combo_pair(T v, const char *n) : value(v), name(n) {}
};

template <typename T>
static bool enum_combo(const char *label,
                       T *current_val,
                       const vector<combo_pair<T>> &items)
{
    const char *preview = "";
    for (const auto &item : items)
        if (item.value == *current_val)
            preview = item.name;

    if (ImGui::BeginCombo(label, preview))
    {
        for (const auto &item : items)
        {
            bool is_selected = (*current_val == item.value);
            if (ImGui::Selectable(item.name, is_selected))
                *current_val = item.value;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
        return true;
    }
    return false;
}

class material_inspector
{
public:
    void init(editor_app &app)
    {
        m_app = &app;
        m_preview.init(app);
    }
    void draw(resource_ref &mat, real dtime);

    void set_path(const string &path) { m_current_path = path; }
    const string &get_path() const { return m_current_path; }

private:
    void draw_preview_panel(material &mat, real dtime);
    void draw_render_state(material &mat);
    void draw_shader_selection(material &mat);
    void draw_properties(material &mat);
    void save_material_xml(material &mat);
    void save_material_xml(material &mat, const string &path);

    editor_app *m_app  = nullptr;
    bool m_had_changed = false;
    string m_current_path;

    preview_scene m_preview;
    string m_last_mesh_path;

    // Per-parameter texture paths for selectors
    hash_map<string, string> m_texture_paths;

    // Current material being edited (for preview)
    shared_ptr<material> m_current_material = nullptr;

    // For save confirmation
    shared_ptr<material> m_pending_material = nullptr;
    string m_pending_path;
};

void material_inspector::save_material_xml(material &mat, const string &path)
{
    if (path.empty())
    {
        m_app->get_console().log_error("No path set for material save");
        return;
    }

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *root = doc.NewElement("material");
    doc.InsertEndChild(root);

    xml_serializer serializer;
    mat.save_xml(serializer, *root);

    tinyxml2::XMLPrinter printer;
    doc.Accept(&printer);
    string_view xml = printer.CStr();
    auto fs         = m_app->get_resource_manager()->get_file_system();

    if (fs->write_all_text(path, xml))
        m_app->get_console().log_success("Material saved: " + path);
    else
        m_app->get_console().log_error("Failed to save material: " + path);
}

void material_inspector::save_material_xml(material &mat)
{
    save_material_xml(mat, m_current_path);
}

void material_inspector::draw_preview_panel(material &mat, real dtime)
{
    if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
    {
        gpu *g = m_app->get_gpu();
        if (!g)
            return;

        // Use a default sphere if not set
        if (string(m_preview.get_model().get_mesh_path()).empty())
        {
            m_preview.set_mesh("builtin:sphere", m_app->get_resource_manager());
        }

        // Mesh Selection
        {
            string current_mesh_path = m_preview.get_model().get_mesh_path();

            ImGui::PushID("MatInspector_MeshSelect");
            if (draw_asset_selector("Preview Mesh",
                                    current_mesh_path,
                                    asset_type::mesh,
                                    m_app->get_asset_database()))
                m_preview.set_mesh(current_mesh_path.c_str(),
                                   m_app->get_resource_manager());

            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Text("%s", current_mesh_path.c_str());
        }

        // Draw Preview
        ImVec2 size(256, 256);

        // Auto-Fit Bounds Check
        auto mesh = m_preview.get_model().get_mesh();
        if (mesh)
        {
            string current_mesh_path_check =
                string(m_preview.get_model().get_mesh_path());

            if (current_mesh_path_check != m_last_mesh_path)
            {
                vec3<real> min_pt, max_pt;
                mesh->get_bounds(min_pt, max_pt);
                vec3<real> center = (min_pt + max_pt) * 0.5f;
                real dist         = length(max_pt - min_pt) * 1.5f;
                if (dist < 0.1f)
                    dist = 0.1f;

                m_preview.set_focus(center, dist);
                m_last_mesh_path = current_mesh_path_check;
            }
        }

        m_preview.draw_controls();

        m_preview.set_material(m_current_material);
        m_preview.set_clear_color({0.1f, 0.1f, 0.1f, 1.0f});

        m_preview.render_default(size.x, size.y, dtime);
    }
}

void material_inspector::draw_render_state(material &mat)
{
    if (ImGui::CollapsingHeader("Render State", ImGuiTreeNodeFlags_DefaultOpen))
    {
        render_state &state = mat.state();

        ImGui::BeginGroup();
        if (ImGui::Checkbox("Depth Write", &state.depth_write))
            m_had_changed = true;
        if (ImGui::Checkbox("Depth Test", &state.depth_test))
            m_had_changed = true;
        if (ImGui::Checkbox("Cull Face", &state.cull_face))
            m_had_changed = true;
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::BeginGroup();

        if (enum_combo("Cull Mode",
                       &state.cull_mode,
                       vector<combo_pair<cull_face_mode>>{
                           {cull_face_mode::back, "Back"},
                           {cull_face_mode::front, "Front"},
                           {cull_face_mode::front_and_back, "Front & Back"}}))
            m_had_changed = true;

        ImGui::EndGroup();

        ImGui::Separator();
        if (ImGui::Checkbox("Blend", &state.blend))
            m_had_changed = true;
        if (state.blend)
        {
            vector<combo_pair<blend_factor>> factors = {
                {blend_factor::zero, "Zero"},
                {blend_factor::one, "One"},
                {blend_factor::src_color, "Src Color"},
                {blend_factor::one_minus_src_color, "1-Src Color"},
                {blend_factor::src_alpha, "Src Alpha"},
                {blend_factor::one_minus_src_alpha, "1-Src Alpha"},
                {blend_factor::dst_alpha, "Dst Alpha"},
                {blend_factor::dst_color, "Dst Color"}};
            if (enum_combo("Src Factor", &state.blend_src, factors))
                m_had_changed = true;
            if (enum_combo("Dst Factor", &state.blend_dst, factors))
                m_had_changed = true;
        }
    }
}

static void set_param_from_value(material_parameter &param, const value &value)
{
    if (!value.is_valid() && param.type == param_type::color_val)
    {
        param.v4 = {1, 1, 1, 1};
        return;
    }

    switch (param.type)
    {
    case param_type::float_val:
        param.r = (real)value.as_number();
        break;
    case param_type::int_val:
        param.i = value.as_int();
        break;
    case param_type::vec2_val:
        param.v2 = value.as_vec2();
        break;
    case param_type::vec3_val:
        param.v3 = value.as_vec3();
        break;
    case param_type::vec4_val:
        param.v4 = value.as_vec4();
        break;
    case param_type::color_val:
        param.v4 = value.as_color().as_vec4();
        break;
    case param_type::bool_val:
        param.i = value.as_bool() ? 1 : 0;
        break;
    }
}

void material_inspector::draw_shader_selection(material &mat)
{
    if (ImGui::CollapsingHeader("Shader", ImGuiTreeNodeFlags_DefaultOpen))
    {
        string shader_path = "";

        if (draw_asset_selector("Shader Asset",
                                shader_path,
                                asset_type::shader,
                                m_app->get_asset_database()))
        {
            auto *ss = (lua_script_system *)m_app->get_script_system();
            if (ss)
            {
                auto rm  = m_app->get_resource_manager();
                auto res = rm->load<shader_asset>(shader_path);
                if (!res.has_error())
                {
                    auto sh = res.value;
                    if (sh->compile(*ss, m_app->get_gpu(), "glsl120"))
                    {
                        mat.set_shader_path(shader_path);

                        report(report_type::info,
                               "Shader compiled. Uniforms: %d",
                               sh->get_compilation_result().uniforms.size());

                        for (const auto &u :
                             sh->get_compilation_result().uniforms)
                        {
                            const string &name = u.name;
                            const string &type = u.type;
                            const string &hint = u.hint;
                            const value &def   = u.default_value;

                            if (name.find("__") == 0)
                                continue;

                            param_type pt = param_type::vec4_val;
                            bool type_set = false;

                            if (!hint.empty())
                            {
                                if (hint == "color")
                                {
                                    pt       = param_type::color_val;
                                    type_set = true;
                                }
                                else if (hint == "normal")
                                {
                                    pt       = param_type::vec3_val;
                                    type_set = true;
                                }
                            }

                            if (!type_set)
                            {
                                if (type == "float")
                                    pt = param_type::float_val;
                                else if (type == "int")
                                    pt = param_type::int_val;
                                else if (type == "bool")
                                    pt = param_type::bool_val;
                                else if (type == "vec2")
                                    pt = param_type::vec2_val;
                                else if (type == "vec3")
                                    pt = param_type::vec3_val;
                                else if (type == "vec4")
                                {
                                    if (name.find("Color") != string::npos &&
                                        hint.empty())
                                        pt = param_type::color_val;
                                    else
                                        pt = param_type::vec4_val;
                                }
                                else if (type == "sampler2D" ||
                                         type == "Texture2D")
                                {
                                    pt = param_type::int_val;
                                    mat.set_texture(name, {});
                                    mat.ensure_param(name, pt);
                                    continue;
                                }
                            }

                            bool exists = false;
                            for (const auto &ep : mat.parameters())
                            {
                                if (ep.name == name)
                                {
                                    exists = true;
                                    break;
                                }
                            }

                            auto &param = mat.ensure_param(name, pt);
                            if (!exists)
                                set_param_from_value(param, def);
                        }
                    }
                }
            }
        }
    }
}

void material_inspector::draw_properties(material &mat)
{
    if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto &params = mat.parameters();

        ImGui::Text("Parameter count: %zu", params.size());

        for (auto &p : params)
        {
            ImGui::PushID(p.name.c_str());
            switch (p.type)
            {
            case param_type::bool_val:
            {
                bool v = (bool)p.i;
                if (ImGui::Checkbox(p.name.c_str(), &v))
                {
                    p.i           = v ? 1 : 0;
                    m_had_changed = true;
                }
                break;
            }
            case param_type::float_val:
            {
                float v = (float)p.r;
                if (ImGui::DragFloat(p.name.c_str(), &v, 0.01f))
                {
                    p.r           = v;
                    m_had_changed = true;
                }
                break;
            }
            case param_type::int_val:
            {
                bool is_tex    = false;
                auto &textures = mat.textures();
                for (size_t i = 0; i < textures.size(); ++i)
                {
                    if (textures[i].uniform_name == p.name)
                    {
                        is_tex = true;
                        break;
                    }
                }

                if (is_tex)
                {
                    // Use per-parameter texture path
                    string sel_path;
                    m_texture_paths.try_get_value(p.name, sel_path);

                    ImGui::PushID("TexSelect");
                    if (draw_asset_selector(p.name.c_str(),
                                            sel_path,
                                            asset_type::texture,
                                            m_app->get_asset_database()))
                    {
                        m_texture_paths.add_or_set(p.name, sel_path);
                        resource_ref tex(sel_path,
                                         m_app->get_resource_manager());
                        mat.set_texture(p.name, tex);
                        m_had_changed = true;
                    }
                    ImGui::PopID();
                    ImGui::SameLine();
                    ImGui::Text("%s", sel_path.c_str());
                }
                else
                {
                    if (ImGui::DragInt(p.name.c_str(), &p.i))
                        m_had_changed = true;
                }
                break;
            }
            case param_type::vec2_val:
            {
                float v[2] = {(float)p.v2.x, (float)p.v2.y};
                if (ImGui::DragFloat2(p.name.c_str(), v, 0.01f))
                {
                    p.v2          = vec2<real>(v[0], v[1]);
                    m_had_changed = true;
                }
                break;
            }
            case param_type::vec3_val:
            {
                float v[3] = {(float)p.v3.x, (float)p.v3.y, (float)p.v3.z};
                if (ImGui::DragFloat3(p.name.c_str(), v, 0.01f))
                {
                    p.v3          = vec3<real>(v[0], v[1], v[2]);
                    m_had_changed = true;
                }
                break;
            }
            case param_type::vec4_val:
            {
                float v[4] = {
                    (float)p.v4.x, (float)p.v4.y, (float)p.v4.z, (float)p.v4.w};
                if (ImGui::DragFloat4(p.name.c_str(), v, 0.01f))
                {
                    p.v4          = vec4<real>(v[0], v[1], v[2], v[3]);
                    m_had_changed = true;
                }
                break;
            }
            case param_type::color_val:
            {
                float v[4] = {
                    (float)p.v4.x, (float)p.v4.y, (float)p.v4.z, (float)p.v4.w};
                if (ImGui::ColorEdit4(p.name.c_str(), v))
                {
                    p.v4          = vec4<real>(v[0], v[1], v[2], v[3]);
                    m_had_changed = true;
                }
                break;
            }
            }
            ImGui::PopID();
        }
    }
}

void material_inspector::draw(resource_ref &mat_ref, real dtime)
{
    auto mat = mat_ref.get<material>();

    if (m_current_material.get() != mat.get())
    {
        if (m_had_changed)
        {
            ImGui::OpenPopup("Save Changes?");
            m_pending_material = m_current_material;
            m_pending_path     = m_current_path;
        }
        m_current_material = mat;
        m_current_material->set_resource_manager(m_app->get_resource_manager());
        m_had_changed  = false;
        m_current_path = string(mat_ref.path());
    }
    else
    {
        if (m_current_path != string(mat_ref.path()))
            m_current_path = string(mat_ref.path());
    }

    // Save Confirmation Popup
    if (ImGui::BeginPopupModal(
            "Save Changes?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Do you want to save changes to %s?",
                    m_pending_path.c_str());
        ImGui::Separator();

        if (ImGui::Button("Yes", ImVec2(120, 0)))
        {
            if (m_pending_material)
                save_material_xml(*m_pending_material, m_pending_path);
            ImGui::CloseCurrentPopup();
            m_pending_material = nullptr;
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(120, 0)))
        {
            m_app->get_resource_manager()->unload(m_pending_path);
            m_app->get_resource_manager()->load<material>(m_pending_path);

            ImGui::CloseCurrentPopup();
            m_pending_material = nullptr;
        }
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }

    ImGui::Text("Editing: %s", m_current_path.c_str());

    if (ImGui::Button("Save"))
    {
        save_material_xml(*mat);
        m_had_changed = false;
    }
    ImGui::Separator();

    draw_preview_panel(*mat, dtime);
    draw_render_state(*mat);
    draw_shader_selection(*mat);
    draw_properties(*mat);
}

void register_material_inspector()
{
    material_inspector *mat_inspector = new material_inspector();
    editor_registry::register_preview(
        material::TYPE,
        [mat_inspector](void *obj, editor_app &app, real dtime)
        {
            mat_inspector->init(app);
            mat_inspector->draw(*static_cast<resource_ref *>(obj), dtime);
        });
}

} // namespace zabato::editor
