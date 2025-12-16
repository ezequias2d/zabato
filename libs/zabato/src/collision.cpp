#include <stdlib.h>
#include <zabato/collision.hpp>
#include <zabato/gpu.hpp>
#include <zabato/math.hpp>
#include <zabato/real.hpp>

namespace zabato
{
namespace
{

bool point_vs_rect(vec2<real> point, const rect_shape &rect)
{
    vec2<real> half_size = rect.size * real(0.5);
    real left            = rect.position.x - half_size.x;
    real right           = rect.position.x + half_size.x;
    real top             = rect.position.y - half_size.y;
    real bottom          = rect.position.y + half_size.y;
    return point.x >= left && point.x <= right && point.y >= top &&
           point.y <= bottom;
}

bool rect_vs_rect(const rect_shape &a,
                  const rect_shape &b,
                  collision_result &result)
{
    vec2<real> a_half = a.size * 0.5;
    vec2<real> b_half = b.size * 0.5;

    real aLeft   = a.position.x - a_half.x;
    real aRight  = a.position.x + a_half.x;
    real aTop    = a.position.y - a_half.y;
    real aBottom = a.position.y + a_half.y;

    real bLeft   = b.position.x - b_half.x;
    real bRight  = b.position.x + b_half.x;
    real bTop    = b.position.y - b_half.y;
    real bBottom = b.position.y + b_half.y;

    if (aRight < bLeft || aLeft > bRight || aBottom < bTop || aTop > bBottom)
    {
        result.collides = false;
        return false;
    }

    vec2<real> n       = b.position - a.position;
    vec2<real> overlap = a_half + b_half - abs(n);

    if (overlap.x < overlap.y)
    {
        result.distance = overlap.x;
        result.normal   = {(n.x < real(0)) ? -1 : 1, 0};
    }
    else
    {
        result.distance = overlap.y;
        result.normal   = {0, (n.y < real(0)) ? -1 : 1};
    }

    result.penetration = result.normal * result.distance;
    result.collides    = true;
    return true;
}

bool point_vs_circle(vec2<real> point, const circle_shape &circle)
{
    return length_sq(point - circle.position) <= circle.radius * circle.radius;
}

bool circle_vs_circle(const circle_shape &a,
                      const circle_shape &b,
                      collision_result &result)
{
    real r       = a.radius + b.radius;
    real r2      = r * r;
    vec2<real> n = b.position - a.position;
    real d2      = length_sq(n);

    if (d2 > r2)
    {
        result.collides = false;
        return false;
    }

    real d = sqrt(d2);

    result.collides    = true;
    result.distance    = r - d;
    result.normal      = (d > 0) ? n / d : vec2<real>(1, 0);
    result.penetration = result.normal * result.distance;
    return true;
}

bool rect_vs_circle(const rect_shape &rect,
                    const circle_shape &circle,
                    collision_result &result)
{
    vec2<real> n         = circle.position - rect.position;
    vec2<real> half_size = rect.size * 0.5;
    vec2<real> clamped   = clamp(n, -half_size, half_size);
    vec2<real> closest   = rect.position + clamped;
    vec2<real> diff      = closest - circle.position;

    real d2 = length_sq(diff);
    if (d2 > circle.radius * circle.radius)
    {
        result.collides = false;
        return false;
    }

    real d            = sqrt(d2);
    real distance     = circle.radius - d;
    vec2<real> normal = (d > 0) ? diff / d : vec2<real>(1, 0);

    result.collides    = true;
    result.distance    = distance;
    result.normal      = normal;
    result.penetration = normal * distance;
    return true;
}

void project_polygon(const vec2<real> &axis,
                     const polygon_shape &poly,
                     real &minValue,
                     real &maxValue)
{
    minValue = dot(poly.vertices[0], axis);
    maxValue = minValue;
    for (size_t i = 1; i < poly.vertices.size(); ++i)
    {
        real p   = dot(poly.vertices[i], axis);
        minValue = min(minValue, p);
        maxValue = max(maxValue, p);
    }
}

void project_circle(const vec2<real> &axis,
                    const circle_shape &circle,
                    real &min,
                    real &max)
{
    real center_proj = dot(circle.position, axis);
    min              = center_proj - circle.radius;
    max              = center_proj + circle.radius;
}

bool polygon_vs_polygon(const polygon_shape &a,
                        const polygon_shape &b,
                        collision_result &result)
{
    result.collides = false;
    result.distance = real::max_val();

    vec2<real> center[2] = {vec2<real>(0, 0), vec2<real>(0, 0)};

    const polygon_shape *list[] = {&a, &b};
    for (size_t j = 0; j < 2; j++)
    {
        const polygon_shape &poly = *list[j];
        for (size_t i = 0; i < poly.vertices.size(); ++i)
        {
            center[j] += poly.vertices[i];
            vec2<real> edge = poly.vertices[(i + 1) % poly.vertices.size()] -
                              poly.vertices[i];
            vec2<real> axis = normalize(vec2<real>(-edge.y, edge.x));

            real a_min, a_max, b_min, b_max;
            project_polygon(axis, a, a_min, a_max);
            project_polygon(axis, b, b_min, b_max);

            if (a_max < b_min || b_max < a_min) // found a separating axis
                return false;

            real overlap = min(a_max, b_max) - max(a_min, b_min);
            if (overlap < result.distance)
            {
                result.distance = overlap;
                result.normal   = axis;
            }
        }
    }

    center[0] /= real((uint32_t)a.vertices.size());
    center[1] /= real((uint32_t)b.vertices.size());

    if (dot(center[1] - center[0], result.normal) < 0)
        result.normal = -result.normal;

    result.penetration = result.normal * result.distance;
    result.collides    = true;
    return true;
}

bool circle_vs_polygon(const circle_shape &circle,
                       const polygon_shape &poly,
                       collision_result &result)
{
    result.collides = false;
    result.distance = real::max_val();

    vec2<real> poly_center = vec2<real>(0, 0);

    vec2<real> closest_vertex;
    real min_dist_sq = real::max_val();
    for (size_t i = 0; i < poly.vertices.size(); ++i)
    {
        vec2<real> vertex = poly.vertices[i];
        vec2<real> edge =
            poly.vertices[(i + 1) % poly.vertices.size()] - vertex;
        vec2<real> axis = normalize(vec2<real>(-edge.y, edge.x));

        poly_center += vertex;

        real poly_min, poly_max, circle_min, circle_max;
        project_polygon(axis, poly, poly_min, poly_max);
        project_circle(axis, circle, circle_min, circle_max);

        if (poly_max < circle_min && circle_max < poly_min)
            return false;

        real overlap = min(poly_max, circle_max) - max(poly_min, circle_min);
        if (overlap < result.distance)
        {
            result.distance = overlap;
            result.normal   = axis;
        }

        real d2 = length_sq(vertex - circle.position);
        if (d2 < min_dist_sq)
        {
            min_dist_sq    = d2;
            closest_vertex = vertex;
        }
    }

    vec2<real> axis = normalize(closest_vertex - circle.position);
    real poly_min, poly_max, circle_min, circle_max;
    project_polygon(axis, poly, poly_min, poly_max);
    project_circle(axis, circle, circle_min, circle_max);

    if (poly_max < circle_min || circle_max < poly_min)
        return false;

    real overlap = min(poly_max, circle_max) - max(poly_min, circle_min);
    if (overlap < result.distance)
    {
        result.distance = overlap;
        result.normal   = axis;
    }

    result.collides = true;
    poly_center /= real((uint32_t)poly.vertices.size());
    if (dot(poly_center - circle.position, result.normal) < 0)
        result.normal = -result.normal;

    result.penetration = result.normal * result.distance;
    return true;
}

bool rect_vs_polygon(const rect_shape &rect,
                     const polygon_shape &poly,
                     collision_result &result)
{
    vec2<real> half = rect.size * 0.5;
    polygon_shape rect_as_poly;
    rect_as_poly.vertices.resize(4);
    rect_as_poly.vertices[0] = {rect.position.x - half.x,
                                rect.position.y - half.y};
    rect_as_poly.vertices[1] = {rect.position.x + half.x,
                                rect.position.y - half.y};
    rect_as_poly.vertices[2] = {rect.position.x + half.x,
                                rect.position.y + half.y};
    rect_as_poly.vertices[3] = {rect.position.x - half.x,
                                rect.position.y + half.y};
    return polygon_vs_polygon(rect_as_poly, poly, result);
}

bool ray_vs_rectr(vec2<real> origin,
                  vec2<real> dir,
                  const rect_shape &rect,
                  raycast_result &result)
{
    vec2<real> inv_dir   = vec2<real>(1.0, 1.0) / dir;
    vec2<real> half_size = rect.size * 0.5;
    vec2<real> t_near    = (rect.position - half_size.x - origin) * inv_dir;
    vec2<real> t_far     = (rect.position + half_size.y - origin) * inv_dir;

    if (t_near.x > t_far.x)
        swap(t_near.x, t_far.x);
    if (t_near.y > t_far.y)
        swap(t_near.y, t_far.y);

    if (t_near.x > t_far.y || t_near.y > t_far.x)
        return false;

    real t_hit_near = max(t_near.x, t_near.y);
    real t_hit_far  = max(t_near.x, t_far.y);

    if (t_hit_far < 0)
        return false;

    result.hit      = true;
    result.distance = t_hit_near;
    result.point    = origin + dir * t_hit_near;

    if (t_near.x > t_near.y)
        result.normal = (inv_dir.x < 0) ? vec2<real>(1, 0) : vec2<real>(1, 0);
    else
        result.normal = (inv_dir.y < 0) ? vec2<real>(0, 1) : vec2<real>(0, -1);

    return true;
}

bool ray_vs_circle(vec2<real> origin,
                   vec2<real> dir,
                   const circle_shape &circle,
                   raycast_result &result)
{
    vec2<real> oc = origin - circle.position;

    real b = dot(oc, dir);
    real c = dot(oc, oc) - circle.radius * circle.radius;

    if (c > 0 && b > 0)
        return false;

    real discriminant = b * b - c;
    if (discriminant < 0)
        return false;

    result.hit      = true;
    result.distance = -b - sqrt(discriminant);
    if (result.distance < 0)
        result.distance = 0;

    result.point  = origin + dir * result.distance;
    result.normal = normalize(result.point - circle.position);

    return true;
}

bool ray_vs_polygon(vec2<real> origin,
                    vec2<real> dir,
                    const polygon_shape &poly,
                    raycast_result &result)
{
    real min_dist = real::max_val();
    bool hit      = false;

    for (size_t i = 0; i < poly.vertices.size(); ++i)
    {
        const vec2<real> v1 = poly.vertices[i];
        const vec2<real> v2 = poly.vertices[(i + 1) % poly.vertices.size()];
        vec2<real> edge     = v2 - v1;
        vec2<real> normal   = normalize(vec2<real>(-edge.y, edge.x));

        real dot_dir_norm = dot(dir, normal);
        if (abs(dot_dir_norm) < real::epsilon())
            continue; // Ray is parallel to edge

        real t = dot(v1 - origin, normal) / dot_dir_norm;
        if (t >= 0 && t < min_dist)
        {
            vec2<real> intersection_point = origin + dir * t;
            vec2<real> v1_to_intersect    = intersection_point - v1;
            real edge_len_sq              = length_sq(edge);

            if (dot(v1_to_intersect, edge) >= 0 &&
                length_sq(v1_to_intersect) <= edge_len_sq)
            {
                min_dist      = t;
                result.point  = intersection_point;
                result.normal = dot_dir_norm < real(0) ? normal : -normal;
                hit           = true;
            }
        }
    }

    if (hit)
        result.distance = min_dist;
    result.hit = hit;
    return hit;
}

void get_collision_bounds(const collision_shape &shape,
                          vec2<real> &min_out,
                          vec2<real> &max_out)
{
    switch (shape.get_type())
    {
    case collision_type::rect:
    {
        const auto rect      = *static_cast<const rect_shape *>(&shape);
        vec2<real> half_size = rect.size * real(0.5);
        min_out              = rect.position - half_size;
        max_out              = rect.position + half_size;
        break;
    }
    case collision_type::circle:
    {
        const auto circle = *static_cast<const circle_shape *>(&shape);
        min_out           = {circle.position.x - circle.radius,
                             circle.position.y - circle.radius};
        max_out           = {circle.position.x + circle.radius,
                             circle.position.y + circle.radius};
        break;
    }
    case collision_type::polygon:
    {
        const auto poly = *static_cast<const polygon_shape *>(&shape);
        if (poly.vertices.empty())
        {
            min_out = max_out = {real(0), real(0)};
            return;
        }
        min_out = max_out = poly.vertices[0];
        for (size_t i = 1; i < poly.vertices.size(); ++i)
        {
            min_out.x = min(min_out.x, poly.vertices[i].x);
            min_out.y = min(min_out.y, poly.vertices[i].y);
            max_out.x = max(max_out.x, poly.vertices[i].x);
            max_out.y = max(max_out.y, poly.vertices[i].y);
        }
        break;
    }
    default:
        min_out = max_out = {real(0), real(0)};
        break;
    }
}

void draw_cone_helper(gpu &gpu_api,
                      const vision_cone &cone,
                      real z,
                      uint32_t segments)
{
    vec2<real> norm_dir = normalize(cone.direction);

    // Draw the two edge lines of the cone
    gpu_api.begin(primitive_type::lines);

    // Left edge
    real left_angle           = atan2(norm_dir.y, norm_dir.x) - cone.angle;
    vec2<real> left_far_point = {
        cone.position.x + cone.far_dist * cos(left_angle),
        cone.position.y + cone.far_dist * sin(left_angle)};
    gpu_api.vertex(cone.position.x, z, cone.position.y);
    gpu_api.vertex(left_far_point.x, z, left_far_point.y);

    // Right edge
    real right_angle           = atan2(norm_dir.y, norm_dir.x) + cone.angle;
    vec2<real> right_far_point = {
        cone.position.x + cone.far_dist * cos(right_angle),
        cone.position.y + cone.far_dist * sin(right_angle)};
    gpu_api.vertex(cone.position.x, z, cone.position.y);
    gpu_api.vertex(right_far_point.x, z, right_far_point.y);

    gpu_api.end();

    // Draw the far arc
    gpu_api.begin(primitive_type::lines);
    real angle_step = (cone.angle * real(2.0)) / real(segments);
    for (uint32_t i = 0; i < segments; ++i)
    {
        real angle1 = left_angle + real(i) * angle_step;
        real angle2 = left_angle + real(i + 1) * angle_step;

        vec2<real> p1 = {cone.position.x + cone.far_dist * cos(angle1),
                         cone.position.y + cone.far_dist * sin(angle1)};
        vec2<real> p2 = {cone.position.x + cone.far_dist * cos(angle2),
                         cone.position.y + cone.far_dist * sin(angle2)};

        gpu_api.vertex(p1.x, z, p1.y);
        gpu_api.vertex(p2.x, z, p2.y);
    }
    gpu_api.end();
}

} // anonymous namespace

inline bool check_collision(const collision_shape &shape, vec2<real> position)
{
    switch (shape.get_type())
    {
    case collision_type::rect:
        return point_vs_rect(position,
                             *static_cast<const rect_shape *>(&shape));
    case collision_type::circle:
        return point_vs_circle(position,
                               *static_cast<const circle_shape *>(&shape));
    case collision_type::polygon:
    {
        const auto *poly = static_cast<const polygon_shape *>(&shape);
        if (poly->vertices.size() < 3)
            return false;
        bool inside = false;
        for (size_t i = 0, j = poly->vertices.size() - 1;
             i < poly->vertices.size();
             j = i++)
        {
            const vec2<real> &vi = poly->vertices[i];
            const vec2<real> &vj = poly->vertices[j];
            if (((vi.y > position.y) != (vj.y > position.y)) &&
                (position.x <
                 (vj.x - vi.x) * (position.y - vi.y) / (vj.y - vi.y) + vi.x))
            {
                inside = !inside;
            }
        }
        return inside;
    }
    default:
        return false;
    }
}

inline collision_result test_collision(const collision_shape &first,
                                       const collision_shape &second)
{
    collision_result result;
    result.collides = false;
    auto type1      = first.get_type();
    auto type2      = second.get_type();

    if (type1 == collision_type::rect && type2 == collision_type::rect)
    {
        rect_vs_rect(*static_cast<const rect_shape *>(&first),
                     *static_cast<const rect_shape *>(&second),
                     result);
    }
    else if (type1 == collision_type::circle && type2 == collision_type::circle)
    {
        circle_vs_circle(*static_cast<const circle_shape *>(&first),
                         *static_cast<const circle_shape *>(&second),
                         result);
    }
    else if (type1 == collision_type::rect && type2 == collision_type::circle)
    {
        rect_vs_circle(*static_cast<const rect_shape *>(&first),
                       *static_cast<const circle_shape *>(&second),
                       result);
    }
    else if (type1 == collision_type::circle && type2 == collision_type::rect)
    {
        rect_vs_circle(*static_cast<const rect_shape *>(&second),
                       *static_cast<const circle_shape *>(&first),
                       result);
        if (result.collides)
        {
            result.normal      = -result.normal;
            result.penetration = -result.penetration;
        }
    }
    else if (type1 == collision_type::polygon &&
             type2 == collision_type::polygon)
    {
        polygon_vs_polygon(*static_cast<const polygon_shape *>(&first),
                           *static_cast<const polygon_shape *>(&second),
                           result);
    }
    else if (type1 == collision_type::circle &&
             type2 == collision_type::polygon)
    {
        circle_vs_polygon(*static_cast<const circle_shape *>(&first),
                          *static_cast<const polygon_shape *>(&second),
                          result);
    }
    else if (type1 == collision_type::polygon &&
             type2 == collision_type::circle)
    {
        circle_vs_polygon(*static_cast<const circle_shape *>(&second),
                          *static_cast<const polygon_shape *>(&first),
                          result);
        if (result.collides)
        {
            result.normal      = -result.normal;
            result.penetration = -result.penetration;
        }
    }
    else if (type1 == collision_type::rect && type2 == collision_type::polygon)
    {
        rect_vs_polygon(*static_cast<const rect_shape *>(&first),
                        *static_cast<const polygon_shape *>(&second),
                        result);
    }
    else if (type1 == collision_type::polygon && type2 == collision_type::rect)
    {
        rect_vs_polygon(*static_cast<const rect_shape *>(&second),
                        *static_cast<const polygon_shape *>(&first),
                        result);
        if (result.collides)
        {
            result.normal      = -result.normal;
            result.penetration = -result.penetration;
        }
    }

    return result;
}

inline raycast_result raycast(vec2<real> origin,
                              vec2<real> direction,
                              real max_distance,
                              const vector<const collision_shape *> &objects)
{

    raycast_result final_result;
    final_result.distance = max_distance;
    final_result.hit      = false;

    vec2<real> norm_dir = normalize(direction);

    for (const auto &obj : objects)
    {
        raycast_result temp_result;
        bool object_hit = false;

        switch (obj->get_type())
        {
        case collision_type::rect:
            object_hit = ray_vs_rectr(origin,
                                      norm_dir,
                                      *static_cast<const rect_shape *>(obj),
                                      temp_result);
            break;
        case collision_type::circle:
            object_hit = ray_vs_circle(origin,
                                       norm_dir,
                                       *static_cast<const circle_shape *>(obj),
                                       temp_result);
        case collision_type::polygon:
            object_hit =
                ray_vs_polygon(origin,
                               norm_dir,
                               *static_cast<const polygon_shape *>(obj),
                               temp_result);
        case collision_type::none:
        default:
            break;
        }

        if (object_hit && temp_result.distance >= 0 &&
            temp_result.distance < final_result.distance)
        {
            final_result        = temp_result;
            final_result.object = obj;
            final_result.hit    = true;
        }
    }

    return final_result;
}

bool is_point_in_vision_cone(const vision_cone &cone, vec2<real> point)
{
    vec2<real> to_point = point - cone.position;
    real dist_sq        = length_sq(to_point);

    // Check if the point is within the near and far planes
    if (dist_sq < cone.near_dist * cone.near_dist ||
        dist_sq > cone.far_dist * cone.far_dist)
        return false;

    // If the point is effectivelly at the cone's origin, it's inside (if near
    // istance allows it)
    if (dist_sq < real::epsilon())
        return cone.near_dist <= 0;

    // Check if the point is within the cone's angle
    vec2<real> to_point_norm = to_point / sqrt(dist_sq); // normalize
    real cos_angle           = dot(normalize(cone.direction), to_point_norm);

    return cos_angle >= cos(cone.angle);
}

visibility_result
check_visibility(const vision_cone &cone,
                 const collision_shape &object,
                 const vector<const collision_shape *> &obstacles)
{
    visibility_result result;
    vec2<real> obj_center, min_b, max_b;
    real obj_radius;

    get_collision_bounds(object, min_b, max_b);
    obj_center = min_b + (max_b - min_b) * 0.5;
    obj_radius = length(max_b - obj_center);

    result.distance_to_observer = length(obj_center - cone.position);
    result.in_vision_cone       = is_point_in_vision_cone(cone, obj_center);

    if (!result.in_vision_cone)
    {
        result.occlusion         = occlusion_level::full;
        result.visibility_factor = 0;
        return result;
    }

    const int ray_count = 5;
    int visible_rays    = 0;

    for (int i = 0; i < ray_count; ++i)
    {
        vec2<real> target_point;
        if (i == 0)
            target_point = obj_center;
        else
        {
            real angle = (real(i + 1) / real(ray_count) - 0.5) * real::pi() *
                         real(3) / real(18);
            vec2<real> offset = {
                cos(angle) * obj_radius * 0.8,
                sin(angle) * obj_radius * 0.8,
            };
            target_point = obj_center + offset;
        }

        vec2<real> dir = normalize(target_point - cone.position);

        raycast_result ray_res =
            raycast(cone.position, dir, result.distance_to_observer, obstacles);

        if (!ray_res.hit ||
            ray_res.distance >= result.distance_to_observer - real::epsilon())
            visible_rays++;
    }

    result.visibility_factor = real(visible_rays) / real(ray_count);
    if (result.visibility_factor >= 0.99)
        result.occlusion = occlusion_level::none;
    else if (result.visibility_factor > real(0.01))
        result.occlusion = occlusion_level::partial;
    else
        result.occlusion = occlusion_level::full;

    return result;
}

inline bool create_convex_hull(const vec2<real> *points,
                               uint32_t point_count,
                               vector<vec2<real>> &out_vertices)
{
    if (point_count < 3)
        return false;

    // Find pivot point (lowest y, then lowest x)
    size_t pivot_idx = 0;
    for (size_t i = 1; i < point_count; ++i)
    {
        if (points[i].y < points[pivot_idx].y ||
            (points[i].y == points[pivot_idx].y &&
             points[i].x < points[pivot_idx].x))
        {
            pivot_idx = i;
        }
    }

    vec2<real> pivot = points[pivot_idx];
    out_vertices.assign(points, points + point_count);

    swap(out_vertices[0], out_vertices[pivot_idx]);

    // Sort points by polar angle around pivot
    sort(out_vertices.begin(),
         out_vertices.end(),
         [&](const vec2<real> &a, const vec2<real> &b)
         {
             if (a == pivot)
                 return true;
             if (b == pivot)
                 return false;
             real cross_product = (a.x - pivot.x) * (b.y - pivot.y) -
                                  (a.y - pivot.y) * (b.x - pivot.x);
             if (cross_product == real(0))
             {
                 return length_sq(a - pivot) < length_sq(b - pivot);
             }
             return cross_product > real(0);
         });

    // Build the hull
    vector<vec2<real>> hull;
    for (const auto &pt : out_vertices)
    {
        while (hull.size() >= 2)
        {
            vec2<real> p1 = hull[hull.size() - 2];
            vec2<real> p2 = hull[hull.size() - 1];
            if ((p2.x - p1.x) * (pt.y - p1.y) - (p2.y - p1.y) * (pt.x - p1.x) <=
                real(0))
            {
                hull.pop_back();
            }
            else
            {
                break;
            }
        }
        hull.push_back(pt);
    }
    out_vertices = hull;
    return true;
}

/**
 * @brief Helper function to draw a rectangle
 */
static void
draw_rect(gpu &gpu, vec2<real> position, vec2<real> size, real z, bool filled)
{
    vec2<real> halfSize = size * 0.5;

    real left   = position.x - halfSize.x;
    real right  = position.x + halfSize.x;
    real top    = position.y - halfSize.y;
    real bottom = position.y + halfSize.y;

    if (filled)
    {
        gpu.begin(primitive_type::quads);
        gpu.vertex(left, z, top);
        gpu.vertex(right, z, top);
        gpu.vertex(right, z, bottom);
        gpu.vertex(left, z, bottom);
        gpu.end();
    }
    else
    {
        gpu.begin(primitive_type::lines);

        // Top edge
        gpu.vertex(left, z, top);
        gpu.vertex(right, z, top);

        // Right edge
        gpu.vertex(right, z, top);
        gpu.vertex(right, z, bottom);

        // Bottom edge
        gpu.vertex(right, z, bottom);
        gpu.vertex(left, z, bottom);

        // Left edge
        gpu.vertex(left, z, bottom);
        gpu.vertex(left, z, top);

        gpu.end();
    }
}

static void draw_circle(gpu &gpu,
                        vec2<real> position,
                        real radius,
                        real z,
                        uint32_t segments,
                        bool filled)
{
    const real angleStep = (real::pi() * real(2.0)) / real(segments);

    if (filled)
    {
        gpu.begin(primitive_type::triangles);
        for (uint32_t i = 0; i < segments; ++i)
        {
            const real angle1 = real(i) * angleStep;
            const real angle2 = real(i + 1) * angleStep;

            const auto [s1, c1] = sincos(angle1);
            const auto [s2, c2] = sincos(angle2);

            const vec2<real> p1 = position + vec2(c1, s1) * radius;
            const vec2<real> p2 = position + vec2(c2, s2) * radius;

            gpu.vertex(p1.x, z, p1.y);
            gpu.vertex(p2.x, z, p2.y);
        }
        gpu.end();
    }
    else
    {
        gpu.begin(primitive_type::lines);
        for (uint32_t i = 0; i < segments; ++i)
        {
            const real angle1 = real(i) * angleStep;
            const real angle2 = real(i + 1) * angleStep;

            const auto [s1, c1] = sincos(angle1);
            const auto [s2, c2] = sincos(angle2);

            const vec2<real> p1 = position + vec2(c1, s1) * radius;
            const vec2<real> p2 = position + vec2(c2, s2) * radius;

            // Draw line segment between two points on circumference
            gpu.vertex(p1.x, z, p1.y);
            gpu.vertex(p2.x, z, p2.y);
        }
        gpu.end();
    }
}

static void
draw_polygon(gpu &gpu, const vector<vec2<real>> &vertices, real z, bool filled)
{
    if (filled)
    {
        gpu.begin(primitive_type::triangles);
        for (size_t i = 1; i < vertices.size() - 1; ++i)
        {
            const vec2<real> &v0 = vertices[0];
            const vec2<real> &v1 = vertices[i];
            const vec2<real> &v2 = vertices[i + 1];

            gpu.vertex(v0.x, z, v0.y); //< center of fan
            gpu.vertex(v1.x, z, v1.y); //< second
            gpu.vertex(v2.x, z, v2.y); //< third
        }
        gpu.end();
    }
    else
    {
        gpu.begin(primitive_type::lines);
        for (size_t i = 0; i < vertices.size(); i++)
        {
            const vec2<real> &v0 = vertices[i];
            const vec2<real> &v1 = vertices[(i + 1) % vertices.size()];
            gpu.vertex(v0.x, z, v0.y);
            gpu.vertex(v1.x, z, v1.y);
        }
        gpu.end();
    }
}

static void draw_polygon_normals(gpu &gpu,
                                 const vector<vec2<real>> &vertices,
                                 real z,
                                 real normalLength)
{
    gpu.begin(primitive_type::lines);
    for (size_t i = 0; i < vertices.size(); i++)
    {
        const vec2<real> &v0 = vertices[i];
        const vec2<real> &v1 = vertices[(i + 1) % vertices.size()];

        vec2<real> edge     = v1 - v0;
        vec2<real> midpoint = v0 + edge * 0.5;
        vec2<real> normal   = {-edge.y, edge.x};
        normal              = normalize(normal) * normalLength;

        // draw normal line
        gpu.vertex(midpoint.x, z, midpoint.y);
        gpu.vertex(midpoint.x + normal.x, z, midpoint.y + normal.y);
    }
    gpu.end();
}

static void
draw_polygon_vertices(gpu &gpu, const vector<vec2<real>> &vertices, real z)
{
    gpu.begin(primitive_type::points);
    for (size_t i = 0; i < vertices.size(); i++)
    {
        const vec2<real> &v0 = vertices[i];
        gpu.vertex(v0.x, z, v0.y);
    }
    gpu.end();
}

inline void draw_collision(gpu &gpu,
                           const collision_shape *shape,
                           real z,
                           color c,
                           collision_debug_flags flags)
{
    if (!shape)
        return;

    gpu.color(c);
    switch (shape->get_type())
    {
    case collision_type::rect:
    {
        const rect_shape &rect = *(const rect_shape *)shape;
        if ((flags & collision_debug_flags::filled) ==
            collision_debug_flags::filled)
            draw_rect(gpu, rect.position, rect.size, z, true);
        else if ((flags & collision_debug_flags::wireframe) ==
                 collision_debug_flags::wireframe)
            draw_rect(gpu, rect.position, rect.size, z, false);
        break;
    }
    case collision_type::circle:
    {
        const circle_shape &circle = *(const circle_shape *)shape;
        const uint32_t segments    = 32;

        if ((flags & collision_debug_flags::filled) ==
            collision_debug_flags::filled)
            draw_circle(gpu, circle.position, circle.radius, z, segments, true);

        if ((flags & collision_debug_flags::wireframe) ==
            collision_debug_flags::wireframe)
            draw_circle(
                gpu, circle.position, circle.radius, z, segments, false);
        break;
    }
    case collision_type::polygon:
    {
        const polygon_shape &poly = *(const polygon_shape *)shape;

        if (poly.vertices.size() == 0)
            return;

        if ((flags & collision_debug_flags::filled) ==
            collision_debug_flags::filled)
            draw_polygon(gpu, poly.vertices, z, true);

        if ((flags & collision_debug_flags::wireframe) ==
            collision_debug_flags::wireframe)
            draw_polygon(gpu, poly.vertices, z, false);

        if ((flags & collision_debug_flags::normals) ==
            collision_debug_flags::normals)
            draw_polygon_normals(gpu, poly.vertices, z, 1.0);

        if ((flags & collision_debug_flags::vertices) ==
            collision_debug_flags::vertices)
            draw_polygon_vertices(gpu, poly.vertices, z);
        break;
    }
    case collision_type::none:
    default:
        break;
    }
}

inline void draw_vision_cone(gpu &gpu,
                             const vision_cone &cone,
                             real z,
                             color c,
                             uint32_t segments)
{
    gpu.color(c);
    draw_cone_helper(gpu, cone, z, segments);
}

} // namespace zabato
