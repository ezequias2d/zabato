#pragma once

#include <zabato/camera.hpp>
#include <zabato/math.hpp>
#include <zabato/window.hpp>

namespace zabato::editor
{

class editor_camera
{
public:
    editor_camera();
    ~editor_camera() = default;

    void init(window *win);
    void shutdown();

    void update(real dt);
    camera &get_camera() { return m_camera; }

    void set_position(const vec3<real> &pos);
    void look_at(const vec3<real> &target);
    void look_along(const vec3<real> &dir);
    void focus(const vec3<real> &center, real radius);

    bool is_dragging() const { return m_is_dragging; }

private:
    camera m_camera;
    vec3<real> m_position;
    vec2<real> m_rotation; // Pitch, Yaw

    real m_speed             = 5.0f;
    real m_mouse_sensitivity = 0.1f;
    bool m_is_dragging       = false;
    vec2<real> m_last_mouse_pos;

    window *m_window               = nullptr;
    vec2<real> m_accumulated_delta = {0, 0};

    static void on_cursor_move(window *w, real x, real y, real dx, real dy);
};

} // namespace zabato::editor
