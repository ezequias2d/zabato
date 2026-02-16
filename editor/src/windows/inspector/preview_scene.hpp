#pragma once

#include <editor/editor.hpp>
#include <imgui.h>
#include <zabato/gpu.hpp>
#include <zabato/light.hpp>
#include <zabato/math.hpp>
#include <zabato/renderer.hpp>
#include <zabato/resource.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/transformation.hpp>

namespace zabato::editor
{

class preview_scene
{
public:
    preview_scene() = default;
    ~preview_scene()
    {
        if (m_renderer)
            delete m_renderer;
        if (m_model)
            delete m_model;
        if (m_cam)
            delete m_cam;
        if (m_light)
            delete m_light;
    }

    void init(editor_app &app)
    {
        m_app = &app;
        ensure_resources();
    }

    // Set the mesh to be rendered
    void set_mesh(const string &path, resource_manager *manager)
    {
        ensure_resources();
        m_model->set_mesh(path.c_str());
        m_model->set_resource_manager(manager);
    }

    // Set the material to be used
    void set_material(const string &path)
    {
        ensure_resources();
        m_model->set_material(path.c_str());
    }

    void set_material(shared_ptr<material> mat)
    {
        ensure_resources();
        m_model->set_material(mat);
    }

    // Override light data if needed
    void set_light_data(const light_data &output)
    {
        ensure_resources();
        m_light->set_data(output);
    }

    // Set focus point for camera
    void set_focus(const vec3<real> &center, real dist)
    {
        m_center    = center;
        m_dist      = dist;
        m_has_focus = true;
    }

    // Returns the camera for custom manipulation or queries
    camera &get_camera()
    {
        ensure_resources();
        return *m_cam;
    }

    model &get_model()
    {
        ensure_resources();
        return *m_model;
    }

    light &get_light()
    {
        ensure_resources();
        return *m_light;
    }

    texture *get_texture()
    {
        if (m_fbo)
            return m_fbo->get_texture();
        return nullptr;
    }

    // Updates camera, handles input, resizes FBO.
    // Call this inside an ImGui window.
    // Returns true if ready to render.
    bool update_ui_and_resize(float width, float height, real dtime)
    {
        ensure_resources();

        // Resize FBO if needed
        if (!m_fbo)
        {
            m_fbo = m_app->get_gpu()->create_framebuffer(width, height);
        }
        else
        {
            auto sz = m_fbo->get_size();
            if (sz.x != width || sz.y != height)
                m_fbo->resize(width, height);
        }

        if (!m_fbo)
            return false;

        // Update Camera Projection
        m_cam->set_perspective(to_rad(real(45)),
                               real(width) / real(height),
                               real(0.1),
                               real(100.0));

        // Auto-Rotate Logic
        const auto &camera_transform = m_cam->get_world_transform();
        auto cam_up                  = camera_transform.up();
        auto cam_right               = camera_transform.right();

        if (m_auto_rotate)
        {
            m_rotation = quat_from_axis_angle(cam_up, dtime) * m_rotation;
        }

        // Camera positioning logic
        vec3<real> offset = vec3<real>(0, 0, m_dist);
        vec3<real> eye    = m_center + offset;
        vec3<real> up     = vec3<real>(0, 1, 0);

        m_cam->look_at(eye, m_center, up);

        return true;
    }

    void draw_controls()
    {
        // Render UI overlay controls
        ImGui::Checkbox("Auto-Rotate", &m_auto_rotate);
        ImGui::SameLine();
        ImGui::TextDisabled("(Drag preview to rotate)");
    }

    // Draws the FBO texture to ImGui and handles mouse interaction
    void draw_image_and_handle_input(float width, float height)
    {
        if (!m_fbo)
            return;

        ImVec2 pos = ImGui::GetCursorPos();
        ImGui::Image((void *)m_fbo->get_texture(),
                     ImVec2(width, height),
                     ImVec2(0, 1),
                     ImVec2(1, 0));
        ImGui::SetCursorPos(pos);
        ImGui::InvisibleButton("##preview_drag", ImVec2(width, height));

        // Mouse Interaction
        if (ImGui::IsItemActive() &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);

            const auto &camera_transform = m_cam->get_world_transform();
            auto cam_up                  = camera_transform.up();
            auto cam_right               = camera_transform.right();

            if (m_cam)
            {
                auto rot_yaw =
                    quat_from_axis_angle(cam_up, (real)(delta.x * 0.01f));
                auto rot_pitch =
                    quat_from_axis_angle(cam_right, (real)(delta.y * 0.01f));

                m_rotation = rot_yaw * rot_pitch * m_rotation;
            }

            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }
    }

    template <typename F>
    void render_pass(float width, float height, F submit_callback, real dtime)
    {
        if (!update_ui_and_resize(width, height, dtime))
            return;

        gpu *g = m_app->get_gpu();

        // Update Model Transform
        transformation t;
        t.make_identity();
        t.set_rotate(m_rotation);
        m_model->set_local(t);

        g->push_state();
        g->bind_framebuffer(m_fbo);
        g->enable_depth_test(true);
        g->set_depth_func(depth_func::less);
        g->set_depth_write(true);
        g->viewport(width, height);
        g->clear(m_clear_color, 1.0);

        m_cam->update_view_from_transform();
        m_renderer->begin(*m_cam);

        submit_callback(*m_renderer);

        m_renderer->end();

        g->unbind_framebuffer();
        g->use_program(nullptr);
        g->pop_state();

        draw_image_and_handle_input(width, height);
    }

    // Default render pass that just draws the light and model
    void render_default(float width, float height, real dtime)
    {
        render_pass(
            width,
            height,
            [&](renderer &r)
            {
                r.submit(m_light);
                r.submit(m_model);
            },
            dtime);
    }

    void set_clear_color(const color &c) { m_clear_color = c; }

private:
    void ensure_resources()
    {
        if (!m_app)
            return;
        if (m_renderer)
            return;

        gpu *g     = m_app->get_gpu();
        m_renderer = new forward_renderer(*g, *m_app->get_script_system());
        m_model    = new model();
        m_model->set_resource_manager(m_app->get_resource_manager());
        m_cam = new camera();
        m_cam->look_at({2, 2, 2}, {0, 0, 0}, {0, 1, 0});

        // Default light
        m_light = new light();
        light_data ld;
        ld.type                 = light_type::directional;
        ld.ambient              = {0.3f, 0.3f, 0.3f, 1.0f};
        ld.diffuse              = {0.9f, 0.9f, 0.9f, 1.0f};
        ld.specular             = {0.5f, 0.5f, 0.5f, 1.0f};
        ld.spot_direction       = normalize(vec3<real>{-1, -2, -1});
        ld.constant_attenuation = 1.0;
        m_light->set_data(ld);
    }

    editor_app *m_app = nullptr;

    forward_renderer *m_renderer = nullptr;
    framebuffer *m_fbo           = nullptr;
    model *m_model               = nullptr;
    camera *m_cam                = nullptr;
    light *m_light               = nullptr;

    bool m_auto_rotate    = true;
    quat<real> m_rotation = {0, 0, 0, 1};
    vec3<real> m_center   = {0, 0, 0};
    real m_dist           = 3.0f;
    bool m_has_focus      = false;
    color m_clear_color   = {0.15, 0.15, 0.15, 1.0};
};

} // namespace zabato::editor
