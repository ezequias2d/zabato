#include <zabato/error.hpp>
#include <zabato/mesh.hpp>

namespace zabato
{

const rtti mesh::TYPE("zabato.mesh", &resource::TYPE);

/**
 * @brief Renders the model using a given GPU context.
 * @param gpu The GPU interface to use for drawing commands.
 * @param anim An optional animator instance. If provided, the model will be
 * rendered with skeletal animation. If null, it is rendered in its bind
 * pose.
 */
void mesh::render(gpu &gpu,
                  const vector<mat4<real>> *bone_matrices,
                  const color *override_color) const
{
    const auto primitive_type = get_primitive_type();

    gpu.begin(primitive_type);

    const auto primitive_count = get_primitive_count();
    const auto flags           = get_flags();
    const bool has_color  = (flags & mesh_flags::color) != mesh_flags::none;
    const bool has_normal = (flags & mesh_flags::normal) != mesh_flags::none;
    const bool has_tex    = (flags & mesh_flags::tex) != mesh_flags::none;

    const bool has_bone = bone_matrices && !bone_matrices->empty();
    const vector<mat4<real>> *matrices_ptr = bone_matrices;

    switch (primitive_type)
    {
    case primitive_type::quads:
        for (size_t i = 0; i < primitive_count; ++i)
        {
            quad_primitive primitive = {};
            get_primitive(i, primitive);
            for (size_t j = 0; j < 4; ++j)
            {
                uint16_t index = primitive.v[j];
                vertex(has_normal,
                       has_color,
                       has_tex,
                       has_bone,
                       gpu,
                       index,
                       matrices_ptr,
                       override_color);
            }
        }
        break;
    case primitive_type::triangles:
        for (size_t i = 0; i < primitive_count; ++i)
        {
            triangle_primitive primitive = {};
            get_primitive(i, primitive);
            for (size_t j = 0; j < 3; ++j)
            {
                uint16_t index = primitive.v[j];
                vertex(has_normal,
                       has_color,
                       has_tex,
                       has_bone,
                       gpu,
                       index,
                       matrices_ptr,
                       override_color);
            }
        }
        break;
    case primitive_type::points:
        for (size_t i = 0; i < primitive_count; ++i)
        {
            point_primitive primitive = {};
            get_primitive(i, primitive);
            uint16_t index = primitive.v;
            vertex(has_normal,
                   has_color,
                   has_tex,
                   has_bone,
                   gpu,
                   index,
                   matrices_ptr,
                   override_color);
        }
    case primitive_type::lines:
        for (size_t i = 0; i < primitive_count; ++i)
        {
            line_primitive primitive = {};
            get_primitive(i, primitive);
            for (size_t j = 0; j < 2; ++j)
            {
                uint16_t index = primitive.v[j];
                vertex(has_normal,
                       has_color,
                       has_tex,
                       has_bone,
                       gpu,
                       index,
                       matrices_ptr,
                       override_color);
            }
        }
        break;
    default:
        assert(0 && "unsupported mesh primitive");
        break;
    }
    gpu.end();
}

void mesh::calculate_tangents()
{
    if ((m_flags & mesh_flags::tangent) == mesh_flags::none)
    {
        report(report_type::error,
               "calculate_tangents called on mesh without tangent flag!");
        return;
    }

    vector<vec3<real>> tangents(m_vertex_count);

    auto process_triangle = [&](uint16_t idx0, uint16_t idx1, uint16_t idx2)
    {
        vec3<real> pos0, pos1, pos2;
        vec2<real> uv0, uv1, uv2;

        get_position(idx0, pos0);
        get_position(idx1, pos1);
        get_position(idx2, pos2);

        if ((m_flags & mesh_flags::tex) != mesh_flags::none)
        {
            get_texcoord(idx0, uv0);
            get_texcoord(idx1, uv1);
            get_texcoord(idx2, uv2);
        }

        vec3<real> edge1    = pos1 - pos0;
        vec3<real> edge2    = pos2 - pos0;
        vec2<real> deltaUV1 = uv1 - uv0;
        vec2<real> deltaUV2 = uv2 - uv0;

        real det = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;

        real f = real(1.0f);
        // Avoid division by zero
        if (det > real(1e-6f) || det < real(-1e-6f))
            f = real(1.0f) / det;
        else
            f = real(0.0f);

        vec3<real> tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        tangents[idx0] += tangent;
        tangents[idx1] += tangent;
        tangents[idx2] += tangent;
    };

    switch (m_type)
    {
    case primitive_type::triangles:
        for (size_t i = 0; i < m_primitive_count; ++i)
        {
            triangle_primitive prim;
            get_primitive(i, prim);
            process_triangle(prim.v0, prim.v1, prim.v2);
        }
        break;
    case primitive_type::quads:
        for (size_t i = 0; i < m_primitive_count; ++i)
        {
            quad_primitive prim;
            get_primitive(i, prim);
            process_triangle(prim.v0, prim.v1, prim.v2);
            process_triangle(prim.v0, prim.v2, prim.v3);
        }
        break;
    default:
        break;
    }

    // Normalize and set
    for (size_t i = 0; i < m_vertex_count; ++i)
    {
        vec3<real> t = tangents[i];
        if (length_sq(t) > real(1e-6f))
            t = normalize(t);
        set_tangent(i, t);
    }
}

} // namespace zabato