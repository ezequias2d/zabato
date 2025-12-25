#include <zabato/bounding_volume.hpp>

namespace zabato
{

bounding_volume *bounding_volume::create(const vec3<real> &center, real radius)
{
    return new bv_sphere(center, radius);
}

void bv_sphere::compute_from_data(const span<const vec3<real>> &points)
{
    if (points.empty())
    {
        set_center(vec3<real>(0, 0, 0));
        set_radius(0);
        return;
    }

    vec3<real> min_pt = points[0];
    vec3<real> max_pt = points[0];

    for (const auto &pt : points)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (pt[i] < min_pt[i])
                min_pt[i] = pt[i];
            if (pt[i] > max_pt[i])
                max_pt[i] = pt[i];
        }
    }

    vec3<real> center = (min_pt + max_pt) * real(0.5);
    set_center(center);

    real max_sq_dist = 0;
    for (const auto &pt : points)
    {
        vec3<real> d = pt - center;
        real sq_dist = dot(d, d);
        if (sq_dist > max_sq_dist)
        {
            max_sq_dist = sq_dist;
        }
    }

    set_radius(sqrt(max_sq_dist));
}

void bv_sphere::transform_by(const transformation &transform,
                             bounding_volume *result)
{
    bv_sphere *res_sphere = (bv_sphere *)(result);
    if (!res_sphere)
        return;

    vec3<real> new_center = transform.apply_forward(center());
    res_sphere->set_center(new_center);

    vec3<real> scale = transform.scale();
    real max_scale   = scale.x;
    if (scale.y > max_scale)
        max_scale = scale.y;
    if (scale.z > max_scale)
        max_scale = scale.z;

    res_sphere->set_radius(radius() * max_scale);
}

int bv_sphere::which_side(const plane3<real> &p)
{
    real dist = dot(p.normal, center()) + p.d;
    if (dist > radius())
        return 1;
    if (dist < -radius())
        return -1;
    return 0;
}

} // namespace zabato
