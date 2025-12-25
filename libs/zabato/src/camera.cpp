#include "zabato/math.hpp"
#include <zabato/camera.hpp>

namespace zabato
{

const rtti camera::TYPE("zabato.camera", &spatial::TYPE);

void frustum::extract_from_matrix(const mat4<real> &vp)
{
    // Left
    planes[0].normal.x = vp[0][3] + vp[0][0];
    planes[0].normal.y = vp[1][3] + vp[1][0];
    planes[0].normal.z = vp[2][3] + vp[2][0];
    planes[0].d        = vp[3][3] + vp[3][0];

    // Right
    planes[1].normal.x = vp[0][3] - vp[0][0];
    planes[1].normal.y = vp[1][3] - vp[1][0];
    planes[1].normal.z = vp[2][3] - vp[2][0];
    planes[1].d        = vp[3][3] - vp[3][0];

    // Bottom
    planes[2].normal.x = vp[0][3] + vp[0][1];
    planes[2].normal.y = vp[1][3] + vp[1][1];
    planes[2].normal.z = vp[2][3] + vp[2][1];
    planes[2].d        = vp[3][3] + vp[3][1];

    // Top
    planes[3].normal.x = vp[0][3] - vp[0][1];
    planes[3].normal.y = vp[1][3] - vp[1][1];
    planes[3].normal.z = vp[2][3] - vp[2][1];
    planes[3].d        = vp[3][3] - vp[3][1];

    // Near
    planes[4].normal.x = vp[0][3] + vp[0][2];
    planes[4].normal.y = vp[1][3] + vp[1][2];
    planes[4].normal.z = vp[2][3] + vp[2][2];
    planes[4].d        = vp[3][3] + vp[3][2];

    // Far
    planes[5].normal.x = vp[0][3] - vp[0][2];
    planes[5].normal.y = vp[1][3] - vp[1][2];
    planes[5].normal.z = vp[2][3] - vp[2][2];
    planes[5].d        = vp[3][3] - vp[3][2];

    // Normalize planes
    for (int i = 0; i < 6; ++i)
    {
        real len = length(planes[i].normal);
        planes[i].normal /= len;
        planes[i].d /= len;
    }
}

camera::camera() : m_frustum_dirty(true)
{
    m_projection = mat4<real>::identity();
    m_view       = mat4<real>::identity();
}

camera::~camera() {}

void camera::set_perspective(real fovY, real aspect, real near, real far)
{
    m_projection    = mat4_perspective_fov(fovY, aspect, near, far);
    m_frustum_dirty = true;
}

// Helper to extract transform from matrix
void extract_transform(const mat4<real> &m, transformation &t)
{
    vec3<real> scale, trans;
    quat<real> rot;
    mat4_decompose(m, trans, scale, rot);

    t.set_translate(trans);
    t.set_scale(scale);
    t.set_rotate(rot);
}

void camera::look_at(const vec3<real> &eye,
                     const vec3<real> &center,
                     const vec3<real> &up)
{
    m_view = mat4_look_at(eye, center, up);

    mat4<real> world_mat;
    if (inverse(m_view, world_mat))
    {
        transformation t;
        extract_transform(world_mat, t);
        set_local(t);
    }

    m_frustum_dirty = true;
}

mat4<real> camera::get_view_projection() const { return m_projection * m_view; }

const frustum &camera::get_frustum() const
{
    if (m_frustum_dirty)
    {
        m_frustum.extract_from_matrix(get_view_projection());
        m_frustum_dirty = false;
    }
    return m_frustum;
}

void camera::update_view_from_transform()
{
    transformation t     = get_world_transform();
    mat4<real> world_mat = mat4_translation(t.translate()) *
                           mat4_from_quat(t.rotate()) * mat4_scaling(t.scale());

    // Invert to get view matrix
    mat4<real> view_mat;
    if (inverse(world_mat, view_mat))
    {
        m_view          = view_mat;
        m_frustum_dirty = true;
    }
}

} // namespace zabato
