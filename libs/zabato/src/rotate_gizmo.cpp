#include <zabato/gizmos.hpp>
#include <zabato/math.hpp>
#include <zabato/model.hpp>
#include <zabato/picking.hpp>

namespace zabato
{

static int s_rotate_axis = -1;
static vec3<real> s_start_hit_vec;
static quat<real> s_start_rot;

vec3<real> closest_on_segment(const vec3<real> &r_origin,
                              const vec3<real> &r_dir,
                              const vec3<real> &p1,
                              const vec3<real> &p2,
                              real *out_dist = nullptr)
{
    vec3<real> u = r_dir;
    vec3<real> v = p2 - p1;
    vec3<real> w = r_origin - p1;
    real a       = dot(u, u);
    real b       = dot(u, v);
    real c       = dot(v, v);
    real d       = dot(u, w);
    real e       = dot(v, w);
    real D       = a * c - b * b;
    real sc, tc;

    if (D < 1e-6f)
    {
        sc = 0.0f;
        tc = (b > c ? d / b : e / c);
    }
    else
    {
        sc = (b * e - c * d) / D;
        tc = (a * e - b * d) / D;
    }

    tc = clamp(tc, real(0), real(1));
    sc = (dot(p1, u) + tc * dot(v, u) - dot(r_origin, u)) / dot(u, u);
    if (sc < 0)
        sc = 0;

    vec3<real> close_s = p1 + v * tc;

    if (out_dist)
    {
        vec3<real> close_r = r_origin + u * sc;
        *out_dist          = length(close_r - close_s);
    }

    return close_s;
}

bool draw_rotate_gizmo(gpu &gpu, rotate_gizmo_options &options)
{
    real dist  = length(options.position -
                       options.cam.get_world_transform().translate());
    real scale = dist * real(0.15);
    if (scale < real(0.01))
        scale = real(0.01);

    real radius        = real(1.0) * scale;
    real hit_threshold = real(0.15) * scale;

    int hover_axis = -1;

    auto get_closest_pt_on_ring = [&](int axis,
                                      const vec3<real> &pos,
                                      real r,
                                      real *out_dist) -> vec3<real>
    {
        int segs          = 64;
        real min_d        = real(100000);
        vec3<real> best_p = vec3<real>(0, 0, 0);

        auto get_pt = [&](int i) -> vec3<real>
        {
            real theta = real(i) * real(6.28318) / real(segs);
            real c     = cos(theta) * r;
            real s     = sin(theta) * r;
            if (axis == 0)
                return pos + vec3<real>(0, c, s);
            if (axis == 1)
                return pos + vec3<real>(c, 0, s);
            return pos + vec3<real>(c, s, 0);
        };

        vec3<real> prev    = get_pt(0);
        vec3<real> cam_pos = options.cam.get_world_transform().translate();
        real min_cam_d     = real(1e20);

        for (int i = 1; i <= segs; ++i)
        {
            vec3<real> cur = get_pt(i);
            real d         = 0;
            vec3<real> p   = closest_on_segment(options.mouse_ray.origin,
                                              options.mouse_ray.direction,
                                              prev,
                                              cur,
                                              &d);

            real cam_d = length_sq(p - cam_pos);

            if (d < min_d - real(0.001))
            {
                min_d     = d;
                best_p    = p;
                min_cam_d = cam_d;
            }
            else if (d < min_d + real(0.001))
            {
                if (cam_d < min_cam_d)
                {
                    min_d     = d;
                    best_p    = p;
                    min_cam_d = cam_d;
                }
            }
            prev = cur;
        }
        if (out_dist)
            *out_dist = min_d;
        return best_p;
    };

    real d0, d1, d2, d3;
    get_closest_pt_on_ring(0, options.position, radius, &d0);
    get_closest_pt_on_ring(1, options.position, radius, &d1);
    get_closest_pt_on_ring(2, options.position, radius, &d2);

    // Outer Ring (Axis 3)
    real outer_radius   = radius * real(1.2);
    vec3<real> view_dir = normalize(
        options.position - options.cam.get_world_transform().translate());

    vec3<real> plane_n = -view_dir;
    real denom         = dot(plane_n, options.mouse_ray.direction);
    if (abs(denom) > 1e-6f)
    {
        real t =
            dot(options.position - options.mouse_ray.origin, plane_n) / denom;
        if (t > 0)
        {
            vec3<real> hit_p =
                options.mouse_ray.origin + options.mouse_ray.direction * t;
            real dist_to_center = length(hit_p - options.position);
            d3                  = abs(dist_to_center - outer_radius);
        }
        else
            d3 = 1e9f;
    }
    else
        d3 = 1e9f;

    struct Hit
    {
        int axis;
        real d;
    };
    Hit hits[] = {{0, d0}, {1, d1}, {2, d2}, {3, d3}};

    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3 - i; ++j)
            if (hits[j].d > hits[j + 1].d)
            {
                Hit t       = hits[j];
                hits[j]     = hits[j + 1];
                hits[j + 1] = t;
            }

    if (hits[0].d < hit_threshold)
        hover_axis = hits[0].axis;

    static int s_interaction_mode = 0; // 0 = Plane, 1 = Tangent
    static vec3<real> s_tangent_vec;
    static real s_vis_angle        = 0;
    static real s_start_angle_view = 0;

    if (s_rotate_axis != -1 && !options.is_mouse_down)
        s_rotate_axis = -1;

    if (options.is_mouse_down && s_rotate_axis == -1 && hover_axis != -1)
    {
        s_rotate_axis = hover_axis;
        s_start_rot   = options.rotation;

        if (s_rotate_axis == 3) // View Space
        {
            vec3<real> hit_p;
            vec3<real> plane_n = -view_dir;
            real denom         = dot(plane_n, options.mouse_ray.direction);
            if (abs(denom) > 1e-6f)
            {
                real t =
                    dot(options.position - options.mouse_ray.origin, plane_n) /
                    denom;
                hit_p =
                    options.mouse_ray.origin + options.mouse_ray.direction * t;
            }
            else
                hit_p = options.position;

            vec3<real> local     = hit_p - options.position;
            vec3<real> cam_right = options.cam.get_world_transform().right();
            vec3<real> cam_up    = options.cam.get_world_transform().up();

            real x = dot(local, cam_right);
            real y = dot(local, cam_up);

            s_start_angle_view = atan2(y, x);
            s_start_hit_vec    = normalize(local);
        }
        else
        {
            vec3<real> kp = get_closest_pt_on_ring(
                s_rotate_axis, options.position, radius, nullptr);

            vec3<real> axis_vec =
                (s_rotate_axis == 0
                     ? vec3<real>(1, 0, 0)
                     : (s_rotate_axis == 1 ? vec3<real>(0, 1, 0)
                                           : vec3<real>(0, 0, 1)));

            if (abs(dot(view_dir, axis_vec)) < real(0.2))
            {
                s_interaction_mode = 1; // Tangent

                if (length_sq(kp - options.position) < real(0.0001))
                {
                    vec3<real> edge_dir = normalize(cross(view_dir, axis_vec));
                    s_start_hit_vec     = edge_dir;
                    kp = options.position + s_start_hit_vec * radius;
                }
                else
                {
                    s_start_hit_vec = normalize(kp - options.position);
                }

                s_tangent_vec = cross(view_dir, axis_vec);
                if (length_sq(s_tangent_vec) < real(0.001))
                    s_tangent_vec = vec3<real>(0, 1, 0);
                else
                    s_tangent_vec = normalize(s_tangent_vec);
            }
            else
            {
                s_interaction_mode = 0; // Plane
                s_start_hit_vec    = normalize(kp - options.position);
            }
        }
    }

    bool modified = false;
    if (options.is_mouse_down && s_rotate_axis != -1)
    {
        quat<real> q_delta;
        real angle = 0;

        if (s_rotate_axis == 3) // View Space
        {
            vec3<real> hit_p;
            vec3<real> plane_n = -view_dir;
            real denom         = dot(plane_n, options.mouse_ray.direction);
            if (abs(denom) > 1e-6f)
            {
                real t =
                    dot(options.position - options.mouse_ray.origin, plane_n) /
                    denom;
                hit_p =
                    options.mouse_ray.origin + options.mouse_ray.direction * t;
            }
            else
                hit_p = options.position;

            vec3<real> local     = hit_p - options.position;
            vec3<real> cam_right = options.cam.get_world_transform().right();
            vec3<real> cam_up    = options.cam.get_world_transform().up();

            real x = dot(local, cam_right);
            real y = dot(local, cam_up);

            real cur_angle = atan2(y, x);
            angle          = cur_angle - s_start_angle_view;

            if (angle > real::pi())
                angle -= real::pi() * real(2.0);
            if (angle < -real::pi())
                angle += real::pi() * real(2.0);

            vec3<real> axis = -view_dir;

            real half_a = angle * real(0.5);
            real s      = sin(half_a);
            q_delta =
                quat<real>(axis.x * s, axis.y * s, axis.z * s, cos(half_a));
        }
        else
        {
            vec3<real> axis_vec =
                (s_rotate_axis == 0
                     ? vec3<real>(1, 0, 0)
                     : (s_rotate_axis == 1 ? vec3<real>(0, 1, 0)
                                           : vec3<real>(0, 0, 1)));

            angle       = 0;
            s_vis_angle = 0;

            if (s_interaction_mode == 0) // Plane Mode
            {
                vec3<real> u_vec, v_vec;
                if (s_rotate_axis == 0)
                {
                    u_vec = {0, 1, 0};
                    v_vec = {0, 0, 1};
                }
                else if (s_rotate_axis == 1)
                {
                    u_vec = {0, 0, 1};
                    v_vec = {1, 0, 0};
                }
                else
                {
                    u_vec = {1, 0, 0};
                    v_vec = {0, 1, 0};
                }

                vec3<real> kp = get_closest_pt_on_ring(
                    s_rotate_axis, options.position, radius, nullptr);
                vec3<real> current_vec = normalize(kp - options.position);

                real start_angle = atan2(dot(s_start_hit_vec, v_vec),
                                         dot(s_start_hit_vec, u_vec));
                real cur_angle =
                    atan2(dot(current_vec, v_vec), dot(current_vec, u_vec));

                angle = cur_angle - start_angle;

                if (angle > real::pi())
                    angle -= real::pi() * real(2.0);
                if (angle < -real::pi())
                    angle += real::pi() * real(2.0);
            }
            else // Tangent Mode
            {
                vec3<real> start_pt =
                    options.position + s_start_hit_vec * radius;
                vec3<real> line_p1 = start_pt - s_tangent_vec * real(100.0);
                vec3<real> line_p2 = start_pt + s_tangent_vec * real(100.0);

                real d       = 0;
                vec3<real> p = closest_on_segment(options.mouse_ray.origin,
                                                  options.mouse_ray.direction,
                                                  line_p1,
                                                  line_p2,
                                                  &d);

                real linear_delta = dot(p - start_pt, s_tangent_vec);
                angle             = linear_delta / (radius * real(1.2));
            }

            real half_a = angle * real(0.5);
            real s      = sin(half_a);
            q_delta     = quat<real>(
                axis_vec.x * s, axis_vec.y * s, axis_vec.z * s, cos(half_a));
        }

        options.rotation = q_delta * s_start_rot;
        options.rotation = normalize(options.rotation);
        modified         = true;
        s_vis_angle      = angle;
    }

    gpu.push_state();
    gpu.bind_texture(nullptr);
    gpu.clear_depth(1.0);
    gpu.enable_depth_test(true);
    gpu.enable_blend(true);
    gpu.enable_lighting(false);

    int segments   = 64;
    auto draw_ring = [&](int axis, const color &color)
    {
        gpu.color(color);
        gpu.begin(primitive_type::quads);

        vec3<real> cam_pos = options.cam.get_world_transform().translate();
        real my_radius     = radius;
        if (axis == 3)
            my_radius *= real(1.2);

        real thickness = radius * real(0.04);

        for (int i = 0; i < segments; ++i)
        {
            real th1 = real(i) * real(6.28318) / real(segments);
            real th2 = real(i + 1) * real(6.28318) / real(segments);

            auto get_pos = [&](real th) -> vec3<real>
            {
                real c = cos(th) * my_radius;
                real s = sin(th) * my_radius;
                if (axis == 0)
                    return options.position + vec3<real>(0, c, s);
                if (axis == 1)
                    return options.position + vec3<real>(c, 0, s);
                if (axis == 2)
                    return options.position + vec3<real>(c, s, 0);

                quat<real> cam_rot = options.cam.get_world_transform().rotate();
                vec3<real> r       = cam_rot * vec3<real>(1, 0, 0);
                vec3<real> u       = cam_rot * vec3<real>(0, 1, 0);
                return options.position + r * c + u * s;
            };

            vec3<real> p_curr = get_pos(th1);
            vec3<real> p_next = get_pos(th2);

            auto get_offset = [&](const vec3<real> &p, real th) -> vec3<real>
            {
                real c = cos(th), s = sin(th);
                vec3<real> tangent;
                if (axis == 0)
                    tangent = vec3<real>(0, -s, c);
                else if (axis == 1)
                    tangent = vec3<real>(-s, 0, c);
                else if (axis == 2)
                    tangent = vec3<real>(-s, c, 0);
                else
                {
                    vec3<real> r = options.cam.get_world_transform().right();
                    vec3<real> u = options.cam.get_world_transform().up();
                    tangent      = -r * s + u * c;
                }

                vec3<real> view   = normalize(p - cam_pos);
                vec3<real> offset = normalize(cross(view, tangent));
                return offset * thickness;
            };

            vec3<real> off1 = get_offset(p_curr, th1);
            vec3<real> off2 = get_offset(p_next, th2);

            gpu.vertex(p_curr - off1);
            gpu.vertex(p_curr + off1);
            gpu.vertex(p_next + off2);
            gpu.vertex(p_next - off2);
        }
        gpu.end();
    };

    draw_ring(0,
              (hover_axis == 0 || s_rotate_axis == 0) ? color::yellow()
                                                      : color::red());
    draw_ring(1,
              (hover_axis == 1 || s_rotate_axis == 1) ? color::yellow()
                                                      : color::green());
    draw_ring(2,
              (hover_axis == 2 || s_rotate_axis == 2) ? color::yellow()
                                                      : color::blue());
    draw_ring(3,
              (hover_axis == 3 || s_rotate_axis == 3) ? color::yellow()
                                                      : color::white());

    gpu.enable_depth_test(true);

    if (s_rotate_axis != -1)
    {
        gpu.push_state();
        gpu.bind_texture(nullptr);
        gpu.enable_depth_test(true);
        gpu.enable_blend(true);
        gpu.enable_lighting(false);
        gpu.set_blend_func(blend_factor::src_alpha,
                           blend_factor::one_minus_src_alpha);

        if (s_rotate_axis == 3)
        {
            vec3<real> r   = options.cam.get_world_transform().right();
            vec3<real> u   = options.cam.get_world_transform().up();
            real my_radius = radius * real(1.2);

            gpu.color(1.0f, 1.0f, 1.0f, 0.2f);
            gpu.begin(primitive_type::triangle_fan);
            gpu.vertex(options.position);

            int pie_segments = 32;
            for (int i = 0; i <= pie_segments; ++i)
            {
                real t           = (real)i / (real)pie_segments;
                real current_rad = s_start_angle_view + s_vis_angle * t;
                real c           = cos(current_rad) * my_radius;
                real s           = sin(current_rad) * my_radius;
                vec3<real> p     = options.position + r * c + u * s;
                gpu.vertex(p);
            }
            gpu.end();
        }
        else
        {
            vec3<real> u_vec, v_vec;
            if (s_rotate_axis == 0)
            {
                u_vec = {0, 1, 0};
                v_vec = {0, 0, 1};
            }
            else if (s_rotate_axis == 1)
            {
                u_vec = {0, 0, 1};
                v_vec = {1, 0, 0};
            }
            else
            {
                u_vec = {1, 0, 0};
                v_vec = {0, 1, 0};
            }

            color pie_color;
            if (s_rotate_axis == 0)
                pie_color = color(1, 0, 0, 0.333);
            else if (s_rotate_axis == 1)
                pie_color = color(0, 1, 0, 0.333);
            else
                pie_color = color(0, 0, 1, 0.333);

            real start_rad =
                atan2(dot(s_start_hit_vec, v_vec), dot(s_start_hit_vec, u_vec));

            gpu.color(pie_color);
            gpu.begin(primitive_type::triangle_fan);
            gpu.vertex(options.position);

            int pie_segments = 32;
            for (int i = 0; i <= pie_segments; ++i)
            {
                real t = (real)i / (real)pie_segments;

                real current_rad = start_rad + s_vis_angle * t;

                real c = cos(current_rad) * radius;
                real s = sin(current_rad) * radius;

                vec3<real> p = options.position + u_vec * c + v_vec * s;
                gpu.vertex(p);
            }
            gpu.end();
        }
        gpu.pop_state();
    }

    gpu.pop_state();

    if (options.out_hovered)
        *options.out_hovered = (hover_axis != -1 || s_rotate_axis != -1);

    return modified;
}
} // namespace zabato
