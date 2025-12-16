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
void mesh::render(gpu &gpu, const animator *anim) const
{
    const auto primitive_type = get_primitive_type();

    gpu.begin(primitive_type);

    const auto final_bone_matrices =
        anim ? &anim->get_final_bone_matrices() : nullptr;

    const auto primitive_count = get_primitive_count();
    const auto flags           = get_flags();
    const bool has_color  = (flags & mesh_flags::color) != mesh_flags::none;
    const bool has_normal = (flags & mesh_flags::normal) != mesh_flags::none;
    const bool has_tex    = (flags & mesh_flags::tex) != mesh_flags::none;
    const bool has_bone   = (flags & mesh_flags::bone) != mesh_flags::none;

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
                       final_bone_matrices);
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
                       final_bone_matrices);
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
                   final_bone_matrices);
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
                       final_bone_matrices);
            }
        }
        break;
    }
    gpu.end();
}
} // namespace zabato