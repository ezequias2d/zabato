#pragma once

#include <editor/editor_resources.hpp>
#include <zabato/camera.hpp>
#include <zabato/game_message.hpp>
#include <zabato/gpu.hpp>
#include <zabato/imgui.hpp>
#include <zabato/material.hpp>
#include <zabato/model.hpp>
#include <zabato/renderer.hpp>
#include <zabato/shared_ptr.hpp>

namespace zabato::editor
{

class editor_app;

class material_editor_window
{
public:
    void init(editor_app *app);
    void draw(bool &open);
    void process_message(const game_message &msg);

    void set_material(const string &path, shared_ptr<material> mat);

private:
    void draw_preview_panel();
    void draw_settings_tab();
    void draw_render_state();
    void draw_shader_selection();
    void draw_properties();

    editor_app *m_app = nullptr;
    shared_ptr<material> m_current_material;
    string m_current_path;

    // Preview
    class forward_renderer *m_preview_renderer = nullptr;
    framebuffer *m_preview_fbo                 = nullptr;
    model *m_preview_model                     = nullptr;
    class camera *m_preview_cam                = nullptr;

    symbol_ref cmd_inspect_asset = "cmd_inspect_asset";

    // Preview State
    class light *m_preview_light  = nullptr;
    bool m_auto_rotate            = true;
    quat<real> m_preview_rotation = {0, 0, 0, 1};
    vec3<real> m_preview_center   = {0, 0, 0};
    real m_preview_dist           = 3.0f;
    string m_last_mesh_path;
};

} // namespace zabato::editor
