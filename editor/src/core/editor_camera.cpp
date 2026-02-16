#include <editor/editor_camera.hpp>
#include <imgui.h>
#include <zabato/math.hpp>

namespace zabato::editor
{

editor_camera::editor_camera()
{
    m_position = {0, 0, 5};
    m_rotation = {0, 0};
    m_rotation = {0, 0};

    // Set position
    auto t = m_camera.get_local();
    t.set_translate(m_position);
    m_camera.set_local(t);

    m_camera.look_at(m_position, {0, 0, 0}, {0, 1, 0});
}

static editor_camera *g_editor_camera_instance = nullptr;

void editor_camera::init(window *win)
{
    m_window                 = win;
    g_editor_camera_instance = this;
    m_window->add_cursor_move_callback(editor_camera::on_cursor_move);
}

void editor_camera::shutdown()
{
    if (m_window)
    {
        m_window->remove_cursor_move_callback(editor_camera::on_cursor_move);
        m_window = nullptr;
    }
    g_editor_camera_instance = nullptr;
}

void editor_camera::focus(const vec3<real> &center, real radius)
{
    real fov = m_camera.get_fov();

    real dist = radius / sin(fov * 0.5);
    dist *= 1.2f;

    if (dist < 0.5f)
        dist = 0.5f;

    vec3<real> forward = (m_camera.get_local().rotate() * vec3<real>(0, 0, -1));
    m_position         = center - forward * dist;

    m_camera.look_at(m_position, m_position + forward, vec3<real>(0, 1, 0));
}

void editor_camera::on_cursor_move(window *w, real x, real y, real dx, real dy)
{
    if (g_editor_camera_instance && g_editor_camera_instance->m_is_dragging)
    {
        g_editor_camera_instance->m_accumulated_delta.x += dx;
        g_editor_camera_instance->m_accumulated_delta.y += dy;
    }
}

void editor_camera::set_position(const vec3<real> &pos)
{
    m_position = pos;
    auto t     = m_camera.get_local();
    t.set_translate(pos);
    m_camera.set_local(t);
}

void editor_camera::look_at(const vec3<real> &target)
{
    m_camera.look_at(m_position, target, {0, 1, 0});
    vec3<real> dir = normalize(target - m_position);
    look_along(dir);
}

void editor_camera::look_along(const vec3<real> &dir)
{
    if (length_sq(dir) < 0.0001f)
        return;

    vec3<real> d = normalize(dir);

    real pitch_rad = asin(d.y);
    m_rotation.x   = to_deg(pitch_rad);

    real yaw_rad = atan2(d.x, -d.z);
    m_rotation.y = to_deg(yaw_rad);
}

void editor_camera::update(real dt)
{
    if (!ImGui::GetCurrentContext())
        return;

    ImGuiIO &io = ImGui::GetIO();

    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        if (!m_is_dragging)
        {
            m_is_dragging = true;
            m_window->hold_cursor();
            // Reset delta when starting
            m_accumulated_delta = {0, 0};
        }

        if (m_accumulated_delta.x != 0 || m_accumulated_delta.y != 0)
        {
            m_rotation.y -=
                m_accumulated_delta.x * m_mouse_sensitivity * dt * 45.0f;
            m_rotation.x -=
                m_accumulated_delta.y * m_mouse_sensitivity * dt * 45.0f;

            // Reset after consuming
            m_accumulated_delta = {0, 0};

            // Clamp pitch
            if (m_rotation.x > 89.0f)
                m_rotation.x = 89.0f;
            if (m_rotation.x < -89.0f)
                m_rotation.x = -89.0f;
        }
    }
    else
    {
        if (m_is_dragging)
        {
            m_is_dragging = true;
            m_is_dragging = false;
            m_window->release_cursor();
        }
    }

    // Calculate Forward vector
    real yaw_rad   = to_rad(m_rotation.y);
    real pitch_rad = to_rad(m_rotation.x);

    mat4<real> rot = mat4<real>::identity();
    rot            = rot * mat4_rotation_y(yaw_rad);
    rot            = rot * mat4_rotation_x(pitch_rad);

    vec3<real> forward = (rot * vec4<real>(0, 0, -1, 0)).xyz();
    vec3<real> right   = (rot * vec4<real>(1, 0, 0, 0)).xyz();
    vec3<real> up      = (rot * vec4<real>(0, 1, 0, 0)).xyz();

    if (m_is_dragging)
    {
        vec3<real> move_dir = {0, 0, 0};

        if (ImGui::IsKeyDown(ImGuiKey_W))
            move_dir += forward;
        if (ImGui::IsKeyDown(ImGuiKey_S))
            move_dir -= forward;
        if (ImGui::IsKeyDown(ImGuiKey_A))
            move_dir -= right;
        if (ImGui::IsKeyDown(ImGuiKey_D))
            move_dir += right;
        if (ImGui::IsKeyDown(ImGuiKey_E))
            move_dir += up;
        if (ImGui::IsKeyDown(ImGuiKey_Q))
            move_dir -= up;

        if (length_sq(move_dir) > 0)
        {
            move_dir = normalize(move_dir);
            float speed_mult =
                ImGui::IsKeyDown(ImGuiKey_LeftShift) ? 3.0f : 1.0f;
            m_position += move_dir * m_speed * speed_mult * dt;
        }
    }

    // Apply transform
    auto t = m_camera.get_local();
    t.set_translate(m_position);
    m_camera.set_local(t);
    m_camera.look_at(m_position, m_position + forward, {0, 1, 0});
}

} // namespace zabato::editor
