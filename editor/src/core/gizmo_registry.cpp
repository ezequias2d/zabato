#include <editor/editor.hpp>
#include <editor/gizmo_registry.hpp>

#include <zabato/controller.hpp>
#include <zabato/gizmos.hpp>
#include <zabato/light.hpp>
#include <zabato/rtti.hpp>
#include <zabato/spatial.hpp>

namespace zabato::editor
{

hash_map<const rtti *, gizmo_draw_callback> gizmo_registry::s_drawers;

void gizmo_registry::register_drawer(const rtti &type,
                                     gizmo_draw_callback callback)
{
    s_drawers.add_or_set(&type, callback);
}

void gizmo_registry::draw(const spatial *node, gizmo_context &ctx)
{
    if (!node)
        return;

    for (auto it = s_drawers.begin(); it != s_drawers.end(); ++it)
    {
        const rtti *type = it->key;
        if (node->type().is_derived(*type))
            it->value(node, ctx);
    }

    const auto &controllers = node->get_controllers();
    for (const auto &ctrl : controllers)
    {
        if (ctrl)
        {
            ctrl->on_draw_gizmos(ctx.gpu, ctx.selected);
        }
    }
}

struct BuiltLayouts
{
    BuiltLayouts()
    {
        // Camera Gizmo
        gizmo_registry::register_drawer(
            camera::TYPE,
            [](const spatial *s, gizmo_context &ctx)
            {
                // Don't draw the editor camera itself if it is in the scene
                if (s != &ctx.cam)
                {
                    vec3<real> pos = const_cast<spatial *>(s)
                                         ->get_world_transform()
                                         .translate();

                    real dist =
                        length(pos - ctx.cam.get_world_transform().translate());
                    real alpha_mod = 1.0;

                    // Fade out logic: start at 100, end at 150
                    real fade_start = 100.0;
                    real fade_end   = 150.0;

                    if (dist > fade_start)
                    {
                        alpha_mod = real(1.0) - (dist - fade_start) /
                                                    (fade_end - fade_start);
                        if (alpha_mod < 0.0)
                            alpha_mod = 0.0;
                    }

                    color final_color = ctx.color;
                    final_color.a *= alpha_mod;

                    if (final_color.a <= 0.0f)
                        return; // Fully transparent

                    real t          = 0;
                    auto [icon, uv] = ctx.app.get_resources()->get_icon(
                        editor::editor_icon::camera);

                    icon_gizmo_options opts = {
                        .cam         = const_cast<camera &>(ctx.cam),
                        .icon        = icon,
                        .uv          = uv,
                        .position    = pos,
                        .color       = final_color,
                        .mouse_ray   = *ctx.mouse_ray,
                        .out_hovered = nullptr,
                        .out_t       = &t,
                    };

                    if (draw_icon_gizmo(ctx.gpu, opts))
                    {
                        if (ctx.hit_dist < 0 || t < ctx.hit_dist)
                        {
                            ctx.hovered  = const_cast<spatial *>(s);
                            ctx.hit_dist = t;
                        }
                    }

                    if (ctx.selected)
                    {
                        draw_wire_frustum(
                            ctx.gpu,
                            {.cam = *(camera *)s, .color = final_color});
                    }
                }
            });

        // Light Gizmo
        gizmo_registry::register_drawer(
            light::TYPE,
            [](const spatial *s, gizmo_context &ctx)
            {
                light *l = const_cast<light *>(static_cast<const light *>(s));

                vec3<real> pos = l->get_world_transform().translate();
                real dist =
                    length(pos - ctx.cam.get_world_transform().translate());
                real alpha_mod = 1.0;

                // Fade out
                real fade_start = 100.0;
                real fade_end   = 150.0;

                if (dist > fade_start)
                {
                    alpha_mod = real(1.0) -
                                (dist - fade_start) / (fade_end - fade_start);
                    if (alpha_mod < 0.0)
                        alpha_mod = 0.0;
                }

                color final_color = ctx.color;
                final_color.a *= alpha_mod;

                if (final_color.a <= 0.0f)
                    return;

                editor::editor_icon icon_id = editor::editor_icon::unknown;
                switch (l->get_data().type)
                {
                case light_type::directional:
                    icon_id = editor::editor_icon::directonal_light;
                    break;
                case light_type::point:
                    icon_id = editor::editor_icon::point_light;
                    break;
                case light_type::spot:
                    icon_id = editor::editor_icon::spot_light;
                    break;
                }

                real t          = 0;
                auto [icon, uv] = ctx.app.get_resources()->get_icon(icon_id);

                icon_gizmo_options opts = {
                    .cam         = const_cast<camera &>(ctx.cam),
                    .icon        = icon,
                    .uv          = uv,
                    .position    = l->get_world_transform().translate(),
                    .color       = final_color,
                    .mouse_ray   = *ctx.mouse_ray,
                    .out_hovered = nullptr,
                    .out_t       = &t};

                if (draw_icon_gizmo(ctx.gpu, opts))
                {
                    if (ctx.hit_dist < 0 || t < ctx.hit_dist)
                    {
                        ctx.hovered  = const_cast<spatial *>(s);
                        ctx.hit_dist = t;
                    }
                }

                if (ctx.selected)
                {
                    auto calc_range = [](const light_data &d) -> real
                    {
                        color diff(d.diffuse);
                        color spec(d.specular);
                        real intensity = max(diff.r, max(diff.g, diff.b));
                        intensity =
                            max(intensity, max(spec.r, max(spec.g, spec.b)));

                        if (intensity < 0.01)
                            return 0.5;

                        real i_min = 0.05;
                        real val   = intensity / i_min;

                        real Kc = d.constant_attenuation;
                        real Kl = d.linear_attenuation;
                        real Kq = d.quadratic_attenuation;

                        real C = Kc - val;

                        if (Kq > 0.00001)
                        {
                            real delta = Kl * Kl - real(4) * Kq * C;
                            if (delta < 0)
                                return 1000.0;
                            return (-Kl + sqrt(delta)) / (real(2) * Kq);
                        }
                        else if (Kl > 0.00001)
                        {
                            return -C / Kl;
                        }
                        else
                        {
                            return 1000.0;
                        }
                    };

                    real range = calc_range(l->get_data());
                    if (range < 0.5)
                        range = 0.5;
                    if (range > 1000.0)
                        range = 1000.0;

                    switch (l->get_data().type)
                    {
                    case light_type::directional:
                    {
                        vec3<real> dir    = l->get_data().spot_direction;
                        vec3<real> offset = vec3<real>(real(0.1), 0, 0);

                        draw_line(ctx.gpu,
                                  {.start = pos - offset,
                                   .end   = pos - offset + dir * real(2.0),
                                   .color = final_color});

                        draw_line(ctx.gpu,
                                  {.start = pos + offset,
                                   .end   = pos + offset + dir * real(2.0),
                                   .color = final_color});
                        break;
                    }
                    case light_type::point:
                        draw_wire_sphere(ctx.gpu,
                                         {.center = pos,
                                          .radius = range,
                                          .color  = final_color});
                        break;
                    case light_type::spot:
                    {
                        real cutoff = l->get_data().spot_cutoff;
                        if (cutoff <= real(90.0))
                        {
                            real vis_cutoff = cutoff;
                            if (vis_cutoff > real(89.0))
                                vis_cutoff = real(89.0);

                            real length = range;
                            real radius = length * tan(to_rad(vis_cutoff));

                            transformation w = l->get_world_transform();
                            vec3<real> forward =
                                w.rotate() * vec3<real>(0, 0, -1);
                            vec3<real> base_center = pos + forward * length;

                            draw_wire_cone(ctx.gpu,
                                           {.base   = base_center,
                                            .tip    = pos,
                                            .radius = radius,
                                            .color  = final_color});
                        }
                        break;
                    }
                    }
                }
            });
    }
};

static BuiltLayouts s_built_layouts;

} // namespace zabato::editor
