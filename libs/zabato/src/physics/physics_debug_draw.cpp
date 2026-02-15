#include <zabato/gizmos.hpp>
#include <zabato/physics/physics_debug_draw.hpp>
#include <zabato/physics/types.hpp>

namespace zabato::physics
{

void draw_shape_config(gpu &g,
                       const shape_config *shape,
                       const transformation &t,
                       const zabato::color &c)
{
    if (!shape)
        return;

    const auto &type = shape->type();

    if (type == box_shape_config::TYPE)
    {
        const auto *box = static_cast<const box_shape_config *>(shape);
        wire_box_options opts{.center       = t.translate(),
                              .half_extents = box->half_extent,
                              .rotation     = t.rotate(),
                              .color        = c};
        draw_wire_box(g, opts);
    }
    else if (type == sphere_shape_config::TYPE)
    {
        const auto *sphere = static_cast<const sphere_shape_config *>(shape);
        wire_sphere_options opts{
            .center = t.translate(), .radius = sphere->radius, .color = c};
        draw_wire_sphere(g, opts);
    }
    else if (type == capsule_shape_config::TYPE)
    {
        const auto *capsule = static_cast<const capsule_shape_config *>(shape);
        wire_capsule_options opts{.center = t.translate(),
                                  .height = capsule->half_height * 2.0 +
                                            capsule->radius * 2.0,
                                  .radius   = capsule->radius,
                                  .rotation = t.rotate(),
                                  .color    = c};
        draw_wire_capsule(g, opts);
    }
    else if (type == cylinder_shape_config::TYPE)
    {
        const auto *cylinder =
            static_cast<const cylinder_shape_config *>(shape);
        wire_cylinder_options opts{.center   = t.translate(),
                                   .height   = cylinder->half_height * 2.0,
                                   .radius   = cylinder->radius,
                                   .rotation = t.rotate(),
                                   .color    = c};
        draw_wire_cylinder(g, opts);
    }
    else if (type == tapered_capsule_shape_config::TYPE)
    {
        const auto *tapered =
            static_cast<const tapered_capsule_shape_config *>(shape);
        wire_tapered_capsule_options opts{
            .center = t.translate(),
            .height = tapered->half_height * 2.0 + tapered->top_radius +
                      tapered->bottom_radius,
            .top_radius    = tapered->top_radius,
            .bottom_radius = tapered->bottom_radius,
            .rotation      = t.rotate(),
            .color         = c};
        draw_wire_tapered_capsule(g, opts);
    }
    else if (type == tapered_cylinder_shape_config::TYPE)
    {
        const auto *tapered =
            static_cast<const tapered_cylinder_shape_config *>(shape);
        wire_tapered_cylinder_options opts{.center = t.translate(),
                                           .height = tapered->half_height * 2.0,
                                           .top_radius = tapered->top_radius,
                                           .bottom_radius =
                                               tapered->bottom_radius,
                                           .rotation = t.rotate(),
                                           .color    = c};
        draw_wire_tapered_cylinder(g, opts);
    }
}

} // namespace zabato::physics
