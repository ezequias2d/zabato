#include <zabato/camera.hpp>
#include <zabato/mesh.hpp>
#include <zabato/model.hpp>
#include <zabato/picking.hpp>
#include <zabato/shape.hpp>
#include <zabato/world.hpp>

namespace zabato
{

ray3<real> get_screen_ray(camera &cam,
                          const vec2<real> &screen_pos,
                          const vec2<real> &screen_size)
{
    real ndc_x = (real(2.0) * screen_pos.x) / screen_size.x - real(1.0);
    real ndc_y = real(1.0) - (real(2.0) * screen_pos.y) / screen_size.y;

    vec4<real> clip_coords(ndc_x, ndc_y, real(-1.0), real(1.0));

    mat4<real> inv_proj;
    mat4<real> inv_view;

    if (!inverse(cam.get_projection(), inv_proj) ||
        !inverse(cam.get_view(), inv_view))
        return {{0, 0, 0}, {0, 0, 1}};

    vec4<real> eye_coords = inv_proj * clip_coords;
    eye_coords = vec4<real>(eye_coords.x, eye_coords.y, real(-1.0), real(0.0));

    vec4<real> world_coords = inv_view * eye_coords;
    vec3<real> direction =
        normalize(vec3<real>(world_coords.x, world_coords.y, world_coords.z));

    return {cam.get_world_transform().translate(), direction};
}

static bool intersect_ray_mesh(const ray3<real> &r,
                               const mesh &m,
                               const vector<mat4<real>> &bone_matrices,
                               real &out_t)
{
    real closest_t = real::max_val();
    bool hit       = false;

    switch (m.get_primitive_type())
    {
    case primitive_type::triangles:
        for (uint16_t i = 0; i < m.get_primitive_count(); ++i)
        {
            triangle_primitive tri;
            m.get_primitive(i, tri);

            triangle3<real> triangle;
            m.get_skinned_position(tri.v0, bone_matrices, triangle.v0);
            m.get_skinned_position(tri.v1, bone_matrices, triangle.v1);
            m.get_skinned_position(tri.v2, bone_matrices, triangle.v2);

            real t = 0;

            if (r.intersects_with(triangle, t))
            {
                if (t < closest_t)
                {
                    closest_t = t;
                    hit       = true;
                }
            }
        }
        break;
    case primitive_type::quads:
        for (uint16_t i = 0; i < m.get_primitive_count(); ++i)
        {
            quad_primitive quad_prim;
            m.get_primitive(i, quad_prim);

            quad3<real> quad;
            m.get_skinned_position(quad_prim.v0, bone_matrices, quad.v0);
            m.get_skinned_position(quad_prim.v1, bone_matrices, quad.v1);
            m.get_skinned_position(quad_prim.v2, bone_matrices, quad.v2);
            m.get_skinned_position(quad_prim.v3, bone_matrices, quad.v3);

            real t = 0;
            if (r.intersects_with(quad, t))
            {
                if (t < closest_t)
                {
                    closest_t = t;
                    hit       = true;
                }
            }
        }
        break;
    default:
        assert(0);
        break;
    }

    if (hit)
    {
        out_t = closest_t;
        return true;
    }

    return false;
}

tuple<model *, real> pick_object(const world &world, const ray3<real> &r)
{
    model *closest_model = nullptr;
    real closest_t       = real::max_val();

    const auto &models = world.get_models();
    for (const auto &mod : models)
    {
        if (!mod)
            continue;

        bounding_volume *bv = mod->get_world_bound();
        if (!bv)
            continue;

        sphere3<real> sphere = {bv->center(), bv->radius()};

        real sphere_t = 0;
        if (r.intersects_with(sphere, sphere_t))
        {
            auto mesh = mod->get_mesh();
            if (!mesh)
                continue;

            transformation transform = mod->get_world_transform();
            vec3<real> local_origin  = transform.apply_backward(r.origin);
            vec3<real> local_target =
                transform.apply_backward(r.origin + r.direction);
            vec3<real> local_direction = normalize(local_target - local_origin);

            ray3<real> local_ray = {local_origin, local_direction};

            real local_t = 0;
            if (intersect_ray_mesh(
                    local_ray, *mesh, mod->get_bone_matrices(), local_t))
            {
                vec3<real> hit_point_local =
                    local_origin + local_direction * local_t;
                vec3<real> hit_point_world =
                    transform.apply_forward(hit_point_local);

                real t = length(hit_point_world - r.origin);

                if (t > 0 && t < closest_t)
                {
                    closest_t     = t;
                    closest_model = mod;
                }
            }
        }
    }

    return {closest_model, closest_t};
}

} // namespace zabato
