#pragma once

#include <editor/editor_camera.hpp>
#include <editor/windows/viewport.hpp>

#include <zabato/game_message.hpp>
#include <zabato/gpu.hpp>
#include <zabato/picking.hpp>
#include <zabato/renderer.hpp>
#include <zabato/window.hpp>
#include <zabato/world.hpp>

namespace zabato::editor
{
class editor_app;

class scene_view_window
{
public:
    scene_view_window();
    ~scene_view_window() = default;

    void init(window *win);
    void shutdown();
    void update(real delta_time);
    void render(world &w, renderer &r, gpu &g, editor_app &app);
    void on_message(const game_message &msg);
    void focus(const vec3<real> &center, real radius);
    void look_along(const vec3<real> &dir);
    void look_at(const vec3<real> &target);

    bool is_focused() const { return m_viewport.is_focused(); }
    bool is_hovered() const { return m_viewport.is_hovered(); }
    bool is_capturing_input() const { return m_is_dragging; }

    camera &get_camera() { return *m_camera; }

private:
    void on_scene_render(world &w, camera &cam, gpu &g, editor_app &app);
    void on_overlay_render(world &w, camera &cam, editor_app &app);

private:
    viewport_window m_viewport;

#pragma region Camera
    pointer<camera> m_camera;
    vec3<real> m_camera_position = {0, 0, 5};
    vec2<real> m_camera_rotation = {0, 0}; // Pitch, Yaw

    real m_speed       = 5.0f;
    real m_sensitivity = 1.0f;
    bool m_is_dragging = false;
    vec2<real> m_last_mouse_pos;

    window *m_window               = nullptr;
    vec2<real> m_accumulated_delta = {0, 0};

    void on_cursor_move(window *w, real x, real y, real dx, real dy);
#pragma endregion Camera

    vector<object *> m_selection;

    enum class tool_mode
    {
        hand,
        move,
        rotate,
        scale
    };
    tool_mode m_active_tool = tool_mode::hand;

    // Interaction State
    ray3<real> m_latest_ray = {{0, 0, 0}, {0, 0, 1}};
    bool m_is_mouse_down    = false;
    bool m_is_gizmo_hovered = false;
    bool m_show_bones       = false;

    // Persistent state for gizmo dragging
    vec3<real> m_gizmo_scale;
    quat<real> m_gizmo_rotation;

    pointer<spatial> m_hovered_icon = nullptr;
    real m_hovered_icon_dist        = real::max_val();

    symbol_ref cmd_instantiate_prefab = "cmd_instantiate_prefab";
    symbol_ref cmd_select             = "cmd_select";
    symbol_ref cmd_deselect           = "cmd_deselect";
    symbol_ref cmd_focus_selection    = "cmd_focus_selection";
    symbol_ref evt_scene_change       = "evt_scene_change";
    symbol_ref cmd_change_tool        = "cmd_change_tool";
};

} // namespace zabato::editor
