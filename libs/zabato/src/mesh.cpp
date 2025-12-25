#include <zabato/mesh.hpp>

namespace zabato
{
/**
 * @brief Renders the model using a given GPU context.
 * @param gpu The GPU interface to use for drawing commands.
 * @param anim An optional animator instance. If provided, the model will be
 * rendered with skeletal animation. If null, it is rendered in its bind
 * pose.
 */
void mesh::render(gpu &gpu, const vector<spatial *> &bones) const
{
    const auto primitive_type = get_primitive_type();

    gpu.begin(primitive_type);

    vector<mat4<real>> final_bone_matrices;
    if (!bones.empty() && !m_bone_infos.empty())
    {
        final_bone_matrices.resize(m_bone_infos.size());
        size_t bone_count = min(bones.size(), m_bone_infos.size());

        // Compute transforms
        for (size_t i = 0; i < bone_count; ++i)
        {
            spatial *bone = bones[i];
            if (bone)
            {
                // Calculate World Space transform: BoneWorld * Offset
                transformation bone_world = bone->get_world_transform();
                mat4<real> m_bone_world =
                    mat4_translation(bone_world.translate()) *
                    mat4_from_quat(bone_world.rotate()) *
                    mat4_scaling(bone_world.scale());

                final_bone_matrices[i] =
                    m_bone_world * (mat4<real>)m_bone_infos[i].offset_transform;
            }
            else
            {
                final_bone_matrices[i] = mat4<real>::identity();
            }
        }
    }

    const auto primitive_count = get_primitive_count();
    const auto flags           = get_flags();
    const bool has_color  = (flags & mesh_flags::color) != mesh_flags::none;
    const bool has_normal = (flags & mesh_flags::normal) != mesh_flags::none;
    const bool has_tex    = (flags & mesh_flags::tex) != mesh_flags::none;

    // We only enable bone logic if we actually calculated matrices
    const bool has_bone = !final_bone_matrices.empty();
    const vector<mat4<real>> *matrices_ptr =
        final_bone_matrices.empty() ? nullptr : &final_bone_matrices;

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
                       matrices_ptr);
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
                       matrices_ptr);
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
                   matrices_ptr);
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
                       matrices_ptr);
            }
        }
        break;
    }
    gpu.end();
}
} // namespace zabato