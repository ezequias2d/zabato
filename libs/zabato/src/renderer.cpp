#include <zabato/camera.hpp>
#include <zabato/gpu.hpp>
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
}

void simple_renderer::end() {}

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

} // namespace zabato
