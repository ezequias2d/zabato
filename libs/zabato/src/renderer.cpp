#include <zabato/camera.hpp>
#include <zabato/gpu.hpp>
#include <zabato/light.hpp>
#include <zabato/material.hpp>
#include <zabato/mesh.hpp>
#include <zabato/model.hpp>
#include <zabato/node.hpp>
#include <zabato/renderer.hpp>

namespace zabato
{
void simple_renderer::begin(camera &cam)
{
    m_cam = &cam;

    m_gpu.set_matrix_mode(matrix_mode::projection);
    mat4<real> proj = cam.get_projection();
    m_gpu.load_matrix(proj);

    m_gpu.set_matrix_mode(matrix_mode::modelview);
    mat4<real> view = cam.get_view();
    m_gpu.load_matrix(view);

    m_gpu.set_matrix_mode(matrix_mode::texture);
    m_gpu.load_identity();

    m_active_lights = 0;
    m_gpu.enable_lighting(true);
}

void simple_renderer::end()
{
    for (int i = m_active_lights; i < 8; ++i)
    {
        m_gpu.set_light(i, nullptr);
    }
}

void simple_renderer::submit(model *model)
{
    if (!model)
        return;

    auto mesh = model->get_mesh();
    if (!mesh)
        return;

    const auto &bones = model->get_bones();

    // Apply Model Transform
    m_gpu.set_matrix_mode(matrix_mode::modelview);

    transformation t     = model->get_world_transform();
    mat4<real> model_mat = mat4_translation(t.translate()) *
                           mat4_from_quat(t.rotate()) * mat4_scaling(t.scale());

    // ModelView = View * Model
    mat4<real> mv = m_cam->get_view() * model_mat;
    m_gpu.load_matrix(mv);

    // Handle Texture
    if ((mesh->get_flags() & mesh_flags::tex) == mesh_flags::none)
    {
        m_gpu.unbind_texture();
    }

    m_gpu.color(1, 1, 1, 1);

    mesh->render(m_gpu, bones);
}

void simple_renderer::submit(light *light)
{
    if (!light)
        return;
    if (m_active_lights >= 8)
        return; // Max 8 lights usually

    m_gpu.set_light(m_active_lights, &light->get_data());
    m_active_lights++;
}

// Forward Renderer
void forward_renderer::begin(camera &cam)
{
    m_cam = &cam;

    m_gpu.set_matrix_mode(matrix_mode::projection);
    mat4<real> proj = cam.get_projection();
    m_gpu.load_matrix(proj);

    m_gpu.set_matrix_mode(matrix_mode::modelview);
    mat4<real> view = cam.get_view();
    m_gpu.load_matrix(view);

    m_gpu.set_matrix_mode(matrix_mode::texture);
    m_gpu.load_identity();

    m_active_lights = 0;
    m_gpu.enable_lighting(true);
}

void forward_renderer::end()
{
    for (int i = m_active_lights; i < 8; ++i)
    {
        m_gpu.set_light(i, nullptr);
    }

    // Reset State
    m_gpu.use_program(nullptr);
    m_gpu.bind_texture(nullptr);
    m_gpu.set_active_texture(0);
    m_gpu.unbind_texture();
    m_gpu.color(1, 1, 1, 1);
    m_gpu.enable_lighting(false);
}

void forward_renderer::submit(model *model)
{
    if (!model)
        return;

    auto mesh = model->get_mesh();
    if (!mesh)
        return;

    auto material     = model->get_material();
    const auto &bones = model->get_bones();

    // Calculate ModelView
    m_gpu.set_matrix_mode(matrix_mode::modelview);
    transformation t     = model->get_world_transform();
    mat4<real> model_mat = mat4_translation(t.translate()) *
                           mat4_from_quat(t.rotate()) * mat4_scaling(t.scale());
    mat4<real> mv = m_cam->get_view() * model_mat;
    m_gpu.load_matrix(mv);

    // Render
    if (material)
    {
        if (material->get_script_system() != &m_script_system)
            material->set_script_system(&m_script_system);
        material->apply(m_gpu);

        // Draw
        mesh->render(m_gpu, bones);
    }
    else
    {
        m_gpu.use_program(nullptr);
        m_gpu.bind_texture(nullptr);

        m_gpu.color(1, 1, 1, 1);
        mesh->render(m_gpu, bones);
    }
}

void forward_renderer::submit(light *light)
{
    if (!light)
        return;
    if (m_active_lights >= 8)
        return;

    m_gpu.set_light(m_active_lights, &light->get_data());
    m_active_lights++;
}

} // namespace zabato
