#include <zabato/gizmos.hpp>
#include <zabato/math.hpp>
#include <zabato/mesh.hpp>
#include <zabato/physics/types.hpp>
#include <zabato/picking.hpp>

namespace zabato
{

void draw_grid(gpu &gpu, const grid_options &options)
{
    gpu.push_state();
    gpu.set_matrix_mode(matrix_mode::modelview);
    gpu.load_matrix(options.cam.get_view());

    gpu.bind_texture(nullptr);
    gpu.enable_depth_test(true);
    gpu.enable_blend(true);
    gpu.enable_lighting(false);

    gpu.begin(primitive_type::lines);

    vec3<real> cam_pos = options.cam.get_world_transform().translate();
    real step_size     = (real)options.size / (real)options.steps;

    // Snap center to grid step
    real center_x = round(cam_pos.x / step_size) * step_size;
    real center_z = round(cam_pos.z / step_size) * step_size;

    real half_size   = (real)options.size / real(2.0);
    real fade_radius = half_size;

    int half_steps = options.steps / 2;
    for (int i = -half_steps; i <= half_steps; ++i)
    {
        real offset = real(i) * step_size;

        // Calculate alpha clipping based on perpendicular distance from center
        real perp_dist = abs(offset);
        real base_alpha =
            clamp(real(1.0) - perp_dist / fade_radius, real(0.0), real(1.0));

        base_alpha = base_alpha * base_alpha;
        base_alpha *= options.color.a;

        if (base_alpha <= 0.1)
            continue;

        // X-Line (Varying X, Constant Z)
        // Z position relative to world
        real z = center_z + offset;

        // Line extents
        real x_start = center_x - half_size;
        real x_end   = center_x + half_size;

        // Start (Alpha 0) -> Center (Alpha Max)
        gpu.color(options.color.alpha(0));
        gpu.vertex(x_start, 0, z);

        gpu.color(options.color.alpha(base_alpha));
        gpu.vertex(center_x, 0, z);

        // Center (Alpha Max) -> End (Alpha 0)
        gpu.color(with_alpha(options.color, base_alpha));
        gpu.vertex(center_x, 0, z);

        gpu.color(with_alpha(options.color, 0));
        gpu.vertex(x_end, 0, z);

        // Z-Line (Varying Z, Constant X)
        // X position relative to world
        real x = center_x + offset;

        real z_start = center_z - half_size;
        real z_end   = center_z + half_size;

        // Start (Alpha 0) -> Center (Alpha Max)
        gpu.color(with_alpha(options.color, 0));
        gpu.vertex(x, 0, z_start);

        gpu.color(with_alpha(options.color, base_alpha));
        gpu.vertex(x, 0, center_z);

        // Center (Alpha Max) -> End (Alpha 0)
        gpu.color(with_alpha(options.color, base_alpha));
        gpu.vertex(x, 0, center_z);

        gpu.color(with_alpha(options.color, 0));
        gpu.vertex(x, 0, z_end);
    }

    gpu.end();
    gpu.pop_state();
}

/**
 * @brief Calculates the shortest distance between a ray and a line segment.
 *
 * This function finds the points on the ray and the segment that are closest to
 * each other and returns the distance between them.
 *
 * @param r_origin The origin of the ray.
 * @param r_dir The direction of the ray (should be normalized).
 * @param p1 The start point of the segment.
 * @param p2 The end point of the segment.
 * @return The shortest distance.
 */
real dist_ray_segment(const vec3<real> &r_origin,
                      const vec3<real> &r_dir,
                      const vec3<real> &p1,
                      const vec3<real> &p2)
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

    vec3<real> close_r = r_origin + u * sc;
    vec3<real> close_s = p1 + v * tc;

    return length(close_r - close_s);
}

static int s_drag_axis = -1;
static vec3<real> s_start_obj_pos;

bool draw_move_gizmo(gpu &gpu, move_gizmo_options &options)
{
    real dist  = length(options.position -
                       options.cam.get_world_transform().translate());
    real scale = dist * real(0.15);
    if (scale < real(0.01))
        scale = real(0.01);

    real len          = real(1.0) * scale;
    real arrow_len    = real(0.25) * scale;
    real axis_len     = len - arrow_len;
    real radius       = real(0.02) * scale;
    real arrow_radius = real(0.08) * scale;
    real plane_size   = real(0.35) * scale;
    real plane_offset = real(0.1) * scale;

    real hit_threshold = real(0.2) * scale;

    vec3<real> x_axis = {1, 0, 0};
    vec3<real> y_axis = {0, 1, 0};
    vec3<real> z_axis = {0, 0, 1};

    vec3<real> x_end = options.position + x_axis * len;
    vec3<real> y_end = options.position + y_axis * len;
    vec3<real> z_end = options.position + z_axis * len;

    vec3<real> yz_size(real(0.01) * scale, plane_size, plane_size);
    vec3<real> xz_size(plane_size, real(0.01) * scale, plane_size);
    vec3<real> xy_size(plane_size, plane_size, real(0.01) * scale);

    vec3<real> yz_center =
        options.position +
        (y_axis + z_axis) * (plane_size * real(0.5) + plane_offset);
    vec3<real> xz_center =
        options.position +
        (x_axis + z_axis) * (plane_size * real(0.5) + plane_offset);
    vec3<real> xy_center =
        options.position +
        (x_axis + y_axis) * (plane_size * real(0.5) + plane_offset);

    real strict_axis_thresh = real(0.05) * scale;
    real loose_axis_thresh  = real(0.4) * scale;
    real plane_tolerance    = real(0.2) * scale;

    int hover_axis = -1;
    if (s_drag_axis == -1)
    {
        real dx = dist_ray_segment(options.mouse_ray.origin,
                                   options.mouse_ray.direction,
                                   options.position,
                                   x_end);
        real dy = dist_ray_segment(options.mouse_ray.origin,
                                   options.mouse_ray.direction,
                                   options.position,
                                   y_end);
        real dz = dist_ray_segment(options.mouse_ray.origin,
                                   options.mouse_ray.direction,
                                   options.position,
                                   z_end);

        auto check_plane = [&](const vec3<real> &normal,
                               const vec3<real> &u_axis,
                               const vec3<real> &v_axis) -> real
        {
            real denom = dot(options.mouse_ray.direction, normal);
            if (abs(denom) < 1e-6f)
                return 1e9f;

            vec3<real> plane_origin = options.position;
            real t =
                dot(plane_origin - options.mouse_ray.origin, normal) / denom;
            if (t < 0)
                return 1e9f;

            vec3<real> p =
                options.mouse_ray.origin + options.mouse_ray.direction * t;
            vec3<real> local = p - options.position;

            real u = dot(local, u_axis);
            real v = dot(local, v_axis);

            real min_val = plane_offset;
            real max_val = plane_offset + plane_size;
            real tol     = plane_tolerance;

            bool strict =
                (u >= min_val && u <= max_val && v >= min_val && v <= max_val);
            bool fuzzy = (u >= min_val - tol && u <= max_val + tol &&
                          v >= min_val - tol && v <= max_val + tol);

            if (strict)
                return 0.0f;
            if (fuzzy)
                return 1.0f;
            return 1e9f;
        };

        real hit_yz = check_plane(x_axis, y_axis, z_axis);
        real hit_xz = check_plane(y_axis, x_axis, z_axis);
        real hit_xy = check_plane(z_axis, x_axis, y_axis);

        if (hit_yz < 0.5f)
            hover_axis = 3;
        else if (hit_xz < 0.5f)
            hover_axis = 4;
        else if (hit_xy < 0.5f)
            hover_axis = 5;
        else if (dx < strict_axis_thresh)
            hover_axis = 0;
        else if (dy < strict_axis_thresh)
            hover_axis = 1;
        else if (dz < strict_axis_thresh)
            hover_axis = 2;
        else if (hit_yz < 2.0f)
            hover_axis = 3;
        else if (hit_xz < 2.0f)
            hover_axis = 4;
        else if (hit_xy < 2.0f)
            hover_axis = 5;
        else if (dx < loose_axis_thresh)
            hover_axis = 0;
        else if (dy < loose_axis_thresh)
            hover_axis = 1;
        else if (dz < loose_axis_thresh)
            hover_axis = 2;
    }
    else if (!options.is_mouse_down)
    {
        s_drag_axis = -1;
    }

    static real s_drag_start_t = 0;
    static vec3<real> s_drag_start_point;

    if (options.is_mouse_down && s_drag_axis == -1 && hover_axis != -1)
    {
        s_drag_axis     = hover_axis;
        s_start_obj_pos = options.position;

        if (s_drag_axis < 3)
        {
            vec3<real> axis_dir;
            if (s_drag_axis == 0)
                axis_dir = x_axis;
            else if (s_drag_axis == 1)
                axis_dir = y_axis;
            else
                axis_dir = z_axis;

            vec3<real> view_dir =
                normalize(options.cam.get_world_transform().translate() -
                          options.position);
            vec3<real> plane_normal =
                view_dir - axis_dir * dot(view_dir, axis_dir);
            if (length_sq(plane_normal) < 1e-6f)
                plane_normal = axis_dir.x > real(0.9) ? vec3<real>{0, 1, 0}
                                                      : vec3<real>{1, 0, 0};
            else
                plane_normal = normalize(plane_normal);

            real denom = dot(options.mouse_ray.direction, plane_normal);
            if (abs(denom) > 1e-6f)
            {
                real t_ray = dot(options.position - options.mouse_ray.origin,
                                 plane_normal) /
                             denom;
                vec3<real> hit_point = options.mouse_ray.origin +
                                       options.mouse_ray.direction * t_ray;
                s_drag_start_t = dot(hit_point - s_start_obj_pos, axis_dir);
            }
            else
                s_drag_start_t = 0;
        }
        else
        {
            vec3<real> plane_normal;
            if (s_drag_axis == 3)
                plane_normal = x_axis;
            else if (s_drag_axis == 4)
                plane_normal = y_axis;
            else
                plane_normal = z_axis;

            real denom = dot(options.mouse_ray.direction, plane_normal);
            if (abs(denom) > 1e-6f)
            {
                real t_ray = dot(options.position - options.mouse_ray.origin,
                                 plane_normal) /
                             denom;
                s_drag_start_point = options.mouse_ray.origin +
                                     options.mouse_ray.direction * t_ray;
            }
        }
    }

    bool modified = false;
    if (options.is_mouse_down && s_drag_axis != -1)
    {
        if (s_drag_axis < 3)
        {
            vec3<real> axis_dir;
            if (s_drag_axis == 0)
                axis_dir = x_axis;
            else if (s_drag_axis == 1)
                axis_dir = y_axis;
            else
                axis_dir = z_axis;

            vec3<real> view_dir =
                normalize(options.cam.get_world_transform().translate() -
                          s_start_obj_pos);
            vec3<real> plane_normal =
                view_dir - axis_dir * dot(view_dir, axis_dir);
            if (length_sq(plane_normal) < 1e-6f)
                plane_normal = axis_dir.x > real(0.9) ? vec3<real>{0, 1, 0}
                                                      : vec3<real>{1, 0, 0};
            else
                plane_normal = normalize(plane_normal);

            real denom     = dot(options.mouse_ray.direction, plane_normal);
            real current_t = 0;
            if (abs(denom) > 1e-6f)
            {
                real t_ray = dot(s_start_obj_pos - options.mouse_ray.origin,
                                 plane_normal) /
                             denom;
                vec3<real> hit_point = options.mouse_ray.origin +
                                       options.mouse_ray.direction * t_ray;
                current_t = dot(hit_point - s_start_obj_pos, axis_dir);
            }

            real delta       = current_t - s_drag_start_t;
            options.position = s_start_obj_pos + axis_dir * delta;
        }
        else
        {
            vec3<real> plane_normal;
            if (s_drag_axis == 3)
                plane_normal = x_axis;
            else if (s_drag_axis == 4)
                plane_normal = y_axis;
            else
                plane_normal = z_axis;

            real denom = dot(options.mouse_ray.direction, plane_normal);
            if (abs(denom) > 1e-6f)
            {
                real t_ray = dot(s_start_obj_pos - options.mouse_ray.origin,
                                 plane_normal) /
                             denom;
                vec3<real> hit_point = options.mouse_ray.origin +
                                       options.mouse_ray.direction * t_ray;
                vec3<real> delta = hit_point - s_drag_start_point;

                options.position = s_start_obj_pos + delta;
            }
        }

        modified = true;

        x_end = options.position + x_axis * len;
        y_end = options.position + y_axis * len;
        z_end = options.position + z_axis * len;

        yz_center = options.position +
                    (y_axis + z_axis) * (plane_size * real(0.5) + plane_offset);
        xz_center = options.position +
                    (x_axis + z_axis) * (plane_size * real(0.5) + plane_offset);
        xy_center = options.position +
                    (x_axis + y_axis) * (plane_size * real(0.5) + plane_offset);
    }

    gpu.push_state();
    gpu.bind_texture(nullptr);
    gpu.enable_depth_test(false);
    gpu.enable_lighting(false);

    gpu.push_state();
    gpu.enable_blend(true);
    gpu.enable_depth_test(false);
    gpu.set_blend_func(blend_factor::src_alpha,
                       blend_factor::one_minus_src_alpha);

    cube_options yz_cube_options;
    yz_cube_options.center = yz_center;
    yz_cube_options.size   = yz_size;
    yz_cube_options.color =
        (hover_axis == 3 || s_drag_axis == 3) ? color::yellow() : color::red();
    draw_cube(gpu, yz_cube_options);

    cube_options xz_cube_options;
    xz_cube_options.center = xz_center;
    xz_cube_options.size   = xz_size;
    xz_cube_options.color  = (hover_axis == 4 || s_drag_axis == 4)
                                 ? color::yellow()
                                 : color::green();
    draw_cube(gpu, xz_cube_options);

    cube_options xy_cube_options;
    xy_cube_options.center = xy_center;
    xy_cube_options.size   = xy_size;
    xy_cube_options.color =
        (hover_axis == 5 || s_drag_axis == 5) ? color::yellow() : color::blue();
    draw_cube(gpu, xy_cube_options);

    gpu.pop_state();

    cylinder_options x_cylinder_options;
    x_cylinder_options.start  = options.position;
    x_cylinder_options.end    = options.position + x_axis * axis_len;
    x_cylinder_options.radius = radius;
    x_cylinder_options.color =
        (hover_axis == 0 || s_drag_axis == 0) ? color::yellow() : color::red();
    draw_cylinder(gpu, x_cylinder_options);

    cylinder_options y_cylinder_options;
    y_cylinder_options.start  = options.position;
    y_cylinder_options.end    = options.position + y_axis * axis_len;
    y_cylinder_options.radius = radius;
    y_cylinder_options.color  = (hover_axis == 1 || s_drag_axis == 1)
                                    ? color::yellow()
                                    : color::green();
    draw_cylinder(gpu, y_cylinder_options);

    cylinder_options z_cylinder_options;
    z_cylinder_options.start  = options.position;
    z_cylinder_options.end    = options.position + z_axis * axis_len;
    z_cylinder_options.radius = radius;
    z_cylinder_options.color =
        (hover_axis == 2 || s_drag_axis == 2) ? color::yellow() : color::blue();
    draw_cylinder(gpu, z_cylinder_options);

    cone_options x_cone_options;
    x_cone_options.base   = options.position + x_axis * axis_len;
    x_cone_options.tip    = x_end;
    x_cone_options.radius = arrow_radius;
    x_cone_options.color =
        (hover_axis == 0 || s_drag_axis == 0) ? color::yellow() : color::red();
    draw_cone(gpu, x_cone_options);

    cone_options y_cone_options;
    y_cone_options.base   = options.position + y_axis * axis_len;
    y_cone_options.tip    = y_end;
    y_cone_options.radius = arrow_radius;
    y_cone_options.color  = (hover_axis == 1 || s_drag_axis == 1)
                                ? color::yellow()
                                : color::green();
    draw_cone(gpu, y_cone_options);

    cone_options z_cone_options;
    z_cone_options.base   = options.position + z_axis * axis_len;
    z_cone_options.tip    = z_end;
    z_cone_options.radius = arrow_radius;
    z_cone_options.color =
        (hover_axis == 2 || s_drag_axis == 2) ? color::yellow() : color::blue();
    draw_cone(gpu, z_cone_options);

    gpu.enable_depth_test(true);
    gpu.pop_state();

    if (options.out_hovered)
        *options.out_hovered = (hover_axis != -1 || s_drag_axis != -1);

    return modified;
}

static int s_scale_axis = -1;
static vec3<real> s_start_scale;
static real s_scale_start_t = 0;

/**
 * @brief Draws a scale gizmo and handles interaction.
 *
 * @param gpu The GPU interface.
 * @param cam The camera.
 * @param position The position of the object.
 * @param scale The scale of the object (in/out).
 * @param rotation The rotation of the object.
 * @param is_mouse_down Whether the mouse button is pressed.
 * @param mouse_ray The ray from the mouse cursor.
 * @param out_hovered Output flag for hover state.
 * @return True if the scale was modified.
 */
bool draw_scale_gizmo(gpu &gpu, scale_gizmo_options &options)
{
    real dist         = length(options.position -
                       options.cam.get_world_transform().translate());
    real scale_factor = dist * real(0.15);
    if (scale_factor < real(0.01))
        scale_factor = real(0.01);

    real len_val  = real(1.0) * scale_factor;
    real tip_size = real(0.15) * scale_factor;

    vec3<real> axis_x = options.rotation * vec3<real>(1, 0, 0);
    vec3<real> axis_y = options.rotation * vec3<real>(0, 1, 0);
    vec3<real> axis_z = options.rotation * vec3<real>(0, 0, 1);

    vec3<real> vis_scale = options.scale;
    if (abs(vis_scale.x) < 0.1)
        vis_scale.x = (vis_scale.x >= 0 ? 0.1 : -0.1);
    if (abs(vis_scale.y) < 0.1)
        vis_scale.y = (vis_scale.y >= 0 ? 0.1 : -0.1);
    if (abs(vis_scale.z) < 0.1)
        vis_scale.z = (vis_scale.z >= 0 ? 0.1 : -0.1);

    vec3<real> x_end = options.position + axis_x * len_val * vis_scale.x;
    vec3<real> y_end = options.position + axis_y * len_val * vis_scale.y;
    vec3<real> z_end = options.position + axis_z * len_val * vis_scale.z;

    int hover_axis     = -1;
    real hit_threshold = real(0.2);

    // Check and find the axis that the mouse is hovering over
    if (s_scale_axis == -1)
    {
        real dx = dist_ray_segment(options.mouse_ray.origin,
                                   options.mouse_ray.direction,
                                   options.position,
                                   x_end);
        real dy = dist_ray_segment(options.mouse_ray.origin,
                                   options.mouse_ray.direction,
                                   options.position,
                                   y_end);
        real dz = dist_ray_segment(options.mouse_ray.origin,
                                   options.mouse_ray.direction,
                                   options.position,
                                   z_end);

        if (dx < hit_threshold)
            hover_axis = 0;
        else if (dy < hit_threshold)
            hover_axis = 1;
        else if (dz < hit_threshold)
            hover_axis = 2;
    }
    else if (!options.is_mouse_down)
    {
        s_scale_axis = -1;
    }

    bool modified = false;
    vec3<real> active_axis =
        (s_scale_axis == 0 ? axis_x : (s_scale_axis == 1 ? axis_y : axis_z));

    // Plane logic for dragging
    vec3<real> view_dir = normalize(
        options.cam.get_world_transform().translate() - options.position);
    vec3<real> plane_normal =
        view_dir - active_axis * dot(view_dir, active_axis);
    if (length_sq(plane_normal) < real(0.001))
        plane_normal = active_axis.x > real(0.9) ? vec3<real>(0, 1, 0)
                                                 : vec3<real>(1, 0, 0);
    else
        plane_normal = normalize(plane_normal);

    // Initial Hit
    if (options.is_mouse_down && s_scale_axis == -1 && hover_axis != -1)
    {
        s_scale_axis  = hover_axis;
        s_start_scale = options.scale;

        // Re-calc active axis based on new selection
        active_axis =
            (s_scale_axis == 0 ? axis_x
                               : (s_scale_axis == 1 ? axis_y : axis_z));
        plane_normal = view_dir - active_axis * dot(view_dir, active_axis);
        if (length_sq(plane_normal) < real(0.001))
            plane_normal = active_axis.x > real(0.9) ? vec3<real>(0, 1, 0)
                                                     : vec3<real>(1, 0, 0);
        else
            plane_normal = normalize(plane_normal);

        real denom = dot(options.mouse_ray.direction, plane_normal);
        if (abs(denom) > real(0.0001))
        {
            real t_ray =
                dot(options.position - options.mouse_ray.origin, plane_normal) /
                denom;
            vec3<real> hit_point =
                options.mouse_ray.origin + options.mouse_ray.direction * t_ray;
            s_scale_start_t = dot(hit_point - options.position, active_axis);
        }
    }

    // Dragging
    if (options.is_mouse_down && s_scale_axis != -1)
    {
        real denom = dot(options.mouse_ray.direction, plane_normal);
        if (abs(denom) > real(0.0001))
        {
            real t_ray =
                dot(options.position - options.mouse_ray.origin, plane_normal) /
                denom;
            vec3<real> hit_point =
                options.mouse_ray.origin + options.mouse_ray.direction * t_ray;
            real current_t = dot(hit_point - options.position, active_axis);

            real delta  = current_t - s_scale_start_t;
            real factor = delta * real(0.5);

            vec3<real> new_scale = s_start_scale;
            if (s_scale_axis == 0)
                new_scale.x += factor;
            if (s_scale_axis == 1)
                new_scale.y += factor;
            if (s_scale_axis == 2)
                new_scale.z += factor;

            options.scale = new_scale;
            modified      = true;

            vis_scale = options.scale;
            if (abs(vis_scale.x) < 0.1)
                vis_scale.x = (vis_scale.x >= 0 ? 0.1 : -0.1);
            if (abs(vis_scale.y) < 0.1)
                vis_scale.y = (vis_scale.y >= 0 ? 0.1 : -0.1);
            if (abs(vis_scale.z) < 0.1)
                vis_scale.z = (vis_scale.z >= 0 ? 0.1 : -0.1);

            x_end = options.position + axis_x * len_val * vis_scale.x;
            y_end = options.position + axis_y * len_val * vis_scale.y;
            z_end = options.position + axis_z * len_val * vis_scale.z;
        }
    }

    // Render
    gpu.push_state();
    gpu.bind_texture(nullptr);
    gpu.enable_depth_test(false);
    gpu.enable_lighting(false);

    color x_color =
        (hover_axis == 0 || s_scale_axis == 0) ? color::yellow() : color::red();
    color y_color = (hover_axis == 1 || s_scale_axis == 1) ? color::yellow()
                                                           : color::green();
    color z_color = (hover_axis == 2 || s_scale_axis == 2) ? color::yellow()
                                                           : color::blue();

    gpu.begin(primitive_type::lines);
    gpu.color(x_color);
    gpu.vertex(options.position);
    gpu.vertex(x_end);
    gpu.color(y_color);
    gpu.vertex(options.position);
    gpu.vertex(y_end);
    gpu.color(z_color);
    gpu.vertex(options.position);
    gpu.vertex(z_end);
    gpu.end();

    real hover_scale = 1.5f;

    // Base box size
    real ts_x = tip_size;
    real ts_y = tip_size;
    real ts_z = tip_size;

    // If active, widen (scale) the box
    if (s_scale_axis == 0 || hover_axis == 0)
        ts_x *= hover_scale;
    if (s_scale_axis == 1 || hover_axis == 1)
        ts_y *= hover_scale;
    if (s_scale_axis == 2 || hover_axis == 2)
        ts_z *= hover_scale;

    cube_options x_cube_options;
    x_cube_options.center = x_end;
    x_cube_options.size   = vec3<real>(ts_x);
    x_cube_options.color  = x_color;
    draw_cube(gpu, x_cube_options);

    cube_options y_cube_options;
    y_cube_options.center = y_end;
    y_cube_options.size   = vec3<real>(ts_y);
    y_cube_options.color  = y_color;
    draw_cube(gpu, y_cube_options);

    cube_options z_cube_options;
    z_cube_options.center = z_end;
    z_cube_options.size   = vec3<real>(ts_z);
    z_cube_options.color  = z_color;
    draw_cube(gpu, z_cube_options);

    gpu.enable_depth_test(true);
    gpu.pop_state();

    if (options.out_hovered)
        *options.out_hovered = (hover_axis != -1 || s_scale_axis != -1);

    return modified;
}

/**
 * @brief Draws a billboarded icon gizmo.
 *
 * @param gpu The GPU interface.
 * @param options The icon options.
 * @return True if the icon was hit/hovered.
 */
bool draw_icon_gizmo(gpu &gpu, icon_gizmo_options &options)
{
    real dist  = length(options.position -
                       options.cam.get_world_transform().translate());
    real scale = dist * real(0.05);
    if (scale < real(0.1))
        scale = real(0.1);

    bool hit = false;
    real t   = 0;

    real radius            = scale * real(0.5);
    sphere3<real> collider = {options.position, radius};
    if (options.mouse_ray.intersects_with(collider, t))
    {
        hit = true;
        if (options.out_t)
            *options.out_t = t;
    }

    const mat4<real> &view = options.cam.get_view();

    vec3<real> right(view[0][0], view[1][0], view[2][0]);
    vec3<real> up(view[0][1], view[1][1], view[2][1]);

    vec3<real> half_r = right * scale * real(0.5);
    vec3<real> half_u = up * scale * real(0.5);

    vec3<real> p0 = options.position - half_r - half_u;
    vec3<real> p1 = options.position + half_r - half_u;
    vec3<real> p2 = options.position + half_r + half_u;
    vec3<real> p3 = options.position - half_r + half_u;

    gpu.push_state();
    gpu.set_active_texture(0);
    gpu.enable_texture(true);
    gpu.bind_texture(options.icon);

    gpu.enable_alpha_test(true);
    gpu.set_alpha_func(alpha_func::greater, real(0.1));

    gpu.enable_blend(true);
    gpu.enable_lighting(false);

    gpu.begin(primitive_type::quads);
    gpu.color(options.color);

    gpu.tex_coord(options.uv.min.x, options.uv.max.y);
    gpu.vertex(p0);
    gpu.tex_coord(options.uv.max.x, options.uv.max.y);
    gpu.vertex(p1);
    gpu.tex_coord(options.uv.max.x, options.uv.min.y);
    gpu.vertex(p2);
    gpu.tex_coord(options.uv.min.x, options.uv.min.y);
    gpu.vertex(p3);
    gpu.end();

    gpu.enable_depth_test(true);
    gpu.pop_state();
    return hit;
}

void draw_line(gpu &gpu, const line_options &options)
{
    gpu.push_state();
    gpu.enable_lighting(false);
    gpu.enable_texture(false);
    gpu.color(options.color);
    gpu.begin(primitive_type::lines);
    gpu.vertex(options.start);
    gpu.vertex(options.end);
    gpu.end();
    gpu.pop_state();
}

void draw_cube(gpu &gpu, const cube_options &options)
{
    vec3<real> s     = options.size * real(0.5);
    vec3<real> min_b = options.center - s;
    vec3<real> max_b = options.center + s;

    gpu.color(options.color);

    gpu.begin(primitive_type::quads);
    gpu.vertex(min_b.x, min_b.y, max_b.z);
    gpu.vertex(max_b.x, min_b.y, max_b.z);
    gpu.vertex(max_b.x, max_b.y, max_b.z);
    gpu.vertex(min_b.x, max_b.y, max_b.z);

    gpu.vertex(max_b.x, min_b.y, min_b.z);
    gpu.vertex(min_b.x, min_b.y, min_b.z);
    gpu.vertex(min_b.x, max_b.y, min_b.z);
    gpu.vertex(max_b.x, max_b.y, min_b.z);

    gpu.vertex(min_b.x, max_b.y, max_b.z);
    gpu.vertex(max_b.x, max_b.y, max_b.z);
    gpu.vertex(max_b.x, max_b.y, min_b.z);
    gpu.vertex(min_b.x, max_b.y, min_b.z);

    gpu.vertex(min_b.x, min_b.y, min_b.z);
    gpu.vertex(max_b.x, min_b.y, min_b.z);
    gpu.vertex(max_b.x, min_b.y, max_b.z);
    gpu.vertex(min_b.x, min_b.y, max_b.z);

    gpu.vertex(max_b.x, min_b.y, max_b.z);
    gpu.vertex(max_b.x, min_b.y, min_b.z);
    gpu.vertex(max_b.x, max_b.y, min_b.z);
    gpu.vertex(max_b.x, max_b.y, max_b.z);

    gpu.vertex(min_b.x, min_b.y, min_b.z);
    gpu.vertex(min_b.x, min_b.y, max_b.z);
    gpu.vertex(min_b.x, max_b.y, max_b.z);
    gpu.vertex(min_b.x, max_b.y, min_b.z);

    gpu.end();
}

void draw_cylinder(gpu &gpu, const cylinder_options &options)
{
    vec3<real> axis = options.end - options.start;
    vec3<real> dir  = normalize(axis);

    vec3<real> up =
        abs(dir.y) < 0.99 ? vec3<real>(0, 1, 0) : vec3<real>(1, 0, 0);
    vec3<real> right = normalize(cross(dir, up));
    up               = normalize(cross(right, dir));

    int segments = 12;
    gpu.color(options.color);

    gpu.begin(primitive_type::quads);
    for (int i = 0; i < segments; ++i)
    {
        real angle1 = (real)i / (real)segments * real(2.0) * real::pi();
        real angle2 = (real)(i + 1) / (real)segments * real(2.0) * real::pi();

        real c1 = cos(angle1), s1 = sin(angle1);
        real c2 = cos(angle2), s2 = sin(angle2);

        vec3<real> p1 = options.start + right * (c1 * options.radius) +
                        up * (s1 * options.radius);
        vec3<real> p2 = options.start + right * (c2 * options.radius) +
                        up * (s2 * options.radius);
        vec3<real> p3 = options.end + right * (c2 * options.radius) +
                        up * (s2 * options.radius);
        vec3<real> p4 = options.end + right * (c1 * options.radius) +
                        up * (s1 * options.radius);

        gpu.vertex(p1);
        gpu.vertex(p2);
        gpu.vertex(p3);
        gpu.vertex(p4);
    }
    gpu.end();
}

void draw_cone(gpu &gpu, const cone_options &options)
{
    vec3<real> axis = options.tip - options.base;
    vec3<real> dir  = normalize(axis);

    vec3<real> up =
        abs(dir.y) < 0.99 ? vec3<real>(0, 1, 0) : vec3<real>(1, 0, 0);
    vec3<real> right = normalize(cross(dir, up));
    up               = normalize(cross(right, dir));

    int segments = 16;
    gpu.color(options.color);

    gpu.begin(primitive_type::triangles);
    for (int i = 0; i < segments; ++i)
    {
        real angle1 = (real)i / (real)segments * 2.0f * 3.14159f;
        real angle2 = (real)(i + 1) / (real)segments * 2.0f * 3.14159f;

        real c1 = cos(angle1), s1 = sin(angle1);
        real c2 = cos(angle2), s2 = sin(angle2);

        vec3<real> p1 = options.base + right * (c1 * options.radius) +
                        up * (s1 * options.radius);
        vec3<real> p2 = options.base + right * (c2 * options.radius) +
                        up * (s2 * options.radius);

        gpu.vertex(options.tip);
        gpu.vertex(p1);
        gpu.vertex(p2);
    }
    gpu.end();

    gpu.begin(primitive_type::triangles);
    for (int i = 0; i < segments; ++i)
    {
        real angle1 = (real)i / (real)segments * 2.0f * 3.14159f;
        real angle2 = (real)(i + 1) / (real)segments * 2.0f * 3.14159f;

        real c1 = cos(angle1), s1 = sin(angle1);
        real c2 = cos(angle2), s2 = sin(angle2);

        vec3<real> p1 = options.base + right * (c1 * options.radius) +
                        up * (s1 * options.radius);
        vec3<real> p2 = options.base + right * (c2 * options.radius) +
                        up * (s2 * options.radius);

        gpu.vertex(options.base);
        gpu.vertex(p2);
        gpu.vertex(p1);
    }
    gpu.end();
}

void draw_wire_sphere(gpu &gpu, const wire_sphere_options &options)
{
    int segments = 24;
    gpu.push_state();
    gpu.enable_lighting(false);
    gpu.enable_texture(false);
    gpu.color(options.color);
    gpu.begin(primitive_type::lines);

    auto draw_circle = [&](int axis)
    {
        for (int i = 0; i < segments; ++i)
        {
            real th1 = real(i) * real(6.28318) / real(segments);
            real th2 = real(i + 1) * real(6.28318) / real(segments);

            vec3<real> p1, p2;
            if (axis == 0) // YZ
            {
                p1 = options.center +
                     vec3<real>(0, cos(th1), sin(th1)) * options.radius;
                p2 = options.center +
                     vec3<real>(0, cos(th2), sin(th2)) * options.radius;
            }
            else if (axis == 1) // XZ
            {
                p1 = options.center +
                     vec3<real>(cos(th1), 0, sin(th1)) * options.radius;
                p2 = options.center +
                     vec3<real>(cos(th2), 0, sin(th2)) * options.radius;
            }
            else
            {
                p1 = options.center +
                     vec3<real>(cos(th1), sin(th1), 0) * options.radius;
                p2 = options.center +
                     vec3<real>(cos(th2), sin(th2), 0) * options.radius;
            }
            gpu.vertex(p1);
            gpu.vertex(p2);
        }
    };

    draw_circle(0);
    draw_circle(1);
    draw_circle(2);

    gpu.end();
    gpu.pop_state();
}

void draw_wire_cone(gpu &gpu, const wire_cone_options &options)
{
    vec3<real> axis = options.tip - options.base;
    vec3<real> dir  = normalize(axis);

    vec3<real> up =
        abs(dir.y) < 0.99 ? vec3<real>(0, 1, 0) : vec3<real>(1, 0, 0);
    vec3<real> right = normalize(cross(dir, up));
    up               = normalize(cross(right, dir));

    int segments = 32;
    gpu.color(options.color);
    gpu.push_state();
    gpu.enable_depth_test(false);
    gpu.enable_lighting(false);

    gpu.begin(primitive_type::lines);
    for (int i = 0; i < segments; ++i)
    {
        real angle1 = (real)i / (real)segments * real(2.0) * real(3.14159f);
        real angle2 =
            (real)(i + 1) / (real)segments * real(2.0) * real(3.14159f);

        real c1 = cos(angle1), s1 = sin(angle1);
        real c2 = cos(angle2), s2 = sin(angle2);

        vec3<real> p1 = options.base + right * (c1 * options.radius) +
                        up * (s1 * options.radius);
        vec3<real> p2 = options.base + right * (c2 * options.radius) +
                        up * (s2 * options.radius);

        gpu.vertex(p1);
        gpu.vertex(p2);

        if (i % (segments / 4) == 0)
        {
            gpu.vertex(options.tip);
            gpu.vertex(p1);
        }
    }
    gpu.end();
    gpu.pop_state();
}

void draw_wire_frustum(gpu &gpu, const wire_frustum_options &options)
{
    mat4<real> inv_vp;
    inverse(options.cam.get_projection() * options.cam.get_view(), inv_vp);

    vec3<real> corners[8] = {
        {-1, -1, -1},
        {1, -1, -1},
        {1, 1, -1},
        {-1, 1, -1}, // Near
        {-1, -1, 1},
        {1, -1, 1},
        {1, 1, 1},
        {-1, 1, 1} // Far
    };

    gpu.push_state();
    gpu.bind_texture(nullptr);
    gpu.enable_depth_test(true);
    gpu.enable_blend(true);
    gpu.enable_lighting(false);

    gpu.begin(primitive_type::lines);
    gpu.color(options.color);

    vec3<real> world_corners[8];
    for (int i = 0; i < 8; ++i)
    {
        vec4<real> v(corners[i].x, corners[i].y, corners[i].z, real(1.0));
        vec4<real> world_v = inv_vp * v;
        world_corners[i]   = vec3<real>(world_v.x, world_v.y, world_v.z) *
                           (real(1.0) / world_v.w);
    }

    // Near Plane
    for (int i = 0; i < 4; ++i)
    {
        gpu.vertex(world_corners[i]);
        gpu.vertex(world_corners[(i + 1) % 4]);
    }
    // Far Plane
    for (int i = 0; i < 4; ++i)
    {
        gpu.vertex(world_corners[4 + i]);
        gpu.vertex(world_corners[4 + (i + 1) % 4]);
    }
    // Connections
    for (int i = 0; i < 4; ++i)
    {
        gpu.vertex(world_corners[i]);
        gpu.vertex(world_corners[4 + i]);
    }

    gpu.end();
    gpu.pop_state();
}

void draw_wire_mesh(gpu &gpu, const wire_mesh_options &options)
{
    gpu.push_state();
    gpu.enable_lighting(false);
    gpu.enable_depth_test(true);
    gpu.set_depth_func(depth_func::less_equal);
    gpu.set_blend_func(blend_factor::src_alpha,
                       blend_factor::one_minus_src_alpha);
    gpu.enable_blend(true);
    gpu.set_polygon_mode(polygon_mode::line);
    // Offset lines toward camera/viewer to appear on top of filled surfaces
    gpu.set_polygon_offset(true, -1.0, -1.0);

    gpu.color(options.color);

    // Render mesh
    options.m.render(gpu, options.bone_matrices, &options.color);

    gpu.set_polygon_offset(false, 0, 0); // Cleanup offset specifically
    gpu.pop_state();
}

void draw_skeleton(gpu &gpu, const draw_skeleton_options &options)
{
    const auto &bones = options.m.get_bones();
    if (bones.empty())
        return;

    gpu.push_state();
    gpu.enable_lighting(false);
    gpu.enable_texture(false);

    if (options.x_ray)
        gpu.enable_depth_test(false);
    else
    {
        gpu.enable_depth_test(true);
        gpu.set_depth_func(depth_func::less_equal);
    }

    vector<spatial *> visited;

    for (const auto &bone : bones)
    {
        if (!bone)
            continue;

        spatial *current = bone.get();

        // Traverse up drawing lines and joints
        while (current && current != options.m.get_skeleton_root().get() &&
               current != options.m.parent())
        {
            bool already_drawn = false;
            for (auto *v : visited)
            {
                if (v == current)
                {
                    already_drawn = true;
                    break;
                }
            }
            if (already_drawn)
                break;

            visited.push_back(current);

            vec3<real> pos1 = current->get_world_transform().translate();

            spatial *p = current->parent();
            if (p && p != options.m.get_skeleton_root().get() &&
                p != options.m.parent())
            {
                vec3<real> pos2 = p->get_world_transform().translate();
                bone_gizmo_options bone_opts = {.start = pos2,
                                                .end   = pos1,
                                                .radius =
                                                    options.joint_radius * 2.0f,
                                                .color = options.bone_color};
                draw_bone(gpu, bone_opts);
            }

            gpu.color(options.joint_color);
            wire_sphere_options sphere_opts = {.center = pos1,
                                               .radius = options.joint_radius,
                                               .color  = options.joint_color};
            draw_wire_sphere(gpu, sphere_opts);

            if (!p)
                break;

            current = p;
        }
    }

    gpu.pop_state();
}

void draw_bone(gpu &gpu, const bone_gizmo_options &options)
{
    vec3<real> dir = options.end - options.start;
    real len       = length(dir);
    if (len < 1e-4f)
        return;

    dir = dir / len;

    // Find orthogonal vectors
    vec3<real> up =
        abs(dir.y) < 0.99f ? vec3<real>(0, 1, 0) : vec3<real>(1, 0, 0);
    vec3<real> right = normalize(cross(dir, up));
    up               = normalize(cross(right, dir));

    // Calculate the bone's cross section at ~10% of its length
    real base_offset       = len * 0.1f;
    vec3<real> base_center = options.start + dir * base_offset;

    // Scale horizontal radius based on length to avoid overly fat short bones
    real final_radius = min(options.radius, len * 0.15f);

    vec3<real> p1 = base_center + right * final_radius;
    vec3<real> p2 = base_center + up * final_radius;
    vec3<real> p3 = base_center - right * final_radius;
    vec3<real> p4 = base_center - up * final_radius;

    gpu.push_state();
    gpu.enable_lighting(false);
    gpu.enable_texture(false);
    gpu.color(options.color);
    gpu.begin(primitive_type::lines);

    // Draw pyramid from start to base
    gpu.vertex(options.start);
    gpu.vertex(p1);
    gpu.vertex(options.start);
    gpu.vertex(p2);
    gpu.vertex(options.start);
    gpu.vertex(p3);
    gpu.vertex(options.start);
    gpu.vertex(p4);

    // Draw base square
    gpu.vertex(p1);
    gpu.vertex(p2);
    gpu.vertex(p2);
    gpu.vertex(p3);
    gpu.vertex(p3);
    gpu.vertex(p4);
    gpu.vertex(p4);
    gpu.vertex(p1);

    // Draw pyramid from base to end
    gpu.vertex(options.end);
    gpu.vertex(p1);
    gpu.vertex(options.end);
    gpu.vertex(p2);
    gpu.vertex(options.end);
    gpu.vertex(p3);
    gpu.vertex(options.end);
    gpu.vertex(p4);

    gpu.end();
    gpu.pop_state();
}

void draw_wire_box(gpu &gpu, const wire_box_options &options)
{
    gpu.push_state();
    gpu.enable_lighting(false);
    gpu.enable_texture(false);
    gpu.color(options.color);

    vec3<real> pts[8];
    vec3<real> h = options.half_extents;

    pts[0] = {-h.x, -h.y, -h.z};
    pts[1] = {h.x, -h.y, -h.z};
    pts[2] = {h.x, h.y, -h.z};
    pts[3] = {-h.x, h.y, -h.z};
    pts[4] = {-h.x, -h.y, h.z};
    pts[5] = {h.x, -h.y, h.z};
    pts[6] = {h.x, h.y, h.z};
    pts[7] = {-h.x, h.y, h.z};

    for (int i = 0; i < 8; ++i)
        pts[i] = options.rotation * pts[i] + options.center;

    gpu.begin(primitive_type::lines);
    // Back face
    gpu.vertex(pts[0]);
    gpu.vertex(pts[1]);
    gpu.vertex(pts[1]);
    gpu.vertex(pts[2]);
    gpu.vertex(pts[2]);
    gpu.vertex(pts[3]);
    gpu.vertex(pts[3]);
    gpu.vertex(pts[0]);
    // Front face
    gpu.vertex(pts[4]);
    gpu.vertex(pts[5]);
    gpu.vertex(pts[5]);
    gpu.vertex(pts[6]);
    gpu.vertex(pts[6]);
    gpu.vertex(pts[7]);
    gpu.vertex(pts[7]);
    gpu.vertex(pts[4]);
    // Connecting
    gpu.vertex(pts[0]);
    gpu.vertex(pts[4]);
    gpu.vertex(pts[1]);
    gpu.vertex(pts[5]);
    gpu.vertex(pts[2]);
    gpu.vertex(pts[6]);
    gpu.vertex(pts[3]);
    gpu.vertex(pts[7]);

    gpu.end();
    gpu.pop_state();
}

void draw_wire_capsule(gpu &gpu, const wire_capsule_options &options)
{
    // Draw two spheres and a cylinder body
    vec3<real> half_h_vec =
        options.rotation * vec3<real>(0, options.height * 0.5f, 0);
    vec3<real> top = options.center + half_h_vec;
    vec3<real> bot = options.center - half_h_vec;

    wire_sphere_options top_sphere = {top, options.radius, options.color};
    wire_sphere_options bot_sphere = {bot, options.radius, options.color};
    draw_wire_sphere(gpu, top_sphere);
    draw_wire_sphere(gpu, bot_sphere);

    // Connecting lines
    gpu.push_state();
    gpu.enable_lighting(false);
    gpu.enable_texture(false);
    gpu.color(options.color);
    gpu.begin(primitive_type::lines);

    // Local axes
    vec3<real> r = options.rotation * vec3<real>(1, 0, 0) * options.radius;
    vec3<real> f = options.rotation * vec3<real>(0, 0, 1) * options.radius;

    gpu.vertex(top + r);
    gpu.vertex(bot + r);
    gpu.vertex(top - r);
    gpu.vertex(bot - r);
    gpu.vertex(top + f);
    gpu.vertex(bot + f);
    gpu.vertex(top - f);
    gpu.vertex(bot - f);

    gpu.end();
    gpu.pop_state();
}

void draw_wire_cylinder(gpu &gpu, const wire_cylinder_options &options)
{
    vec3<real> half_h_vec =
        options.rotation * vec3<real>(0, options.height * 0.5f, 0);
    vec3<real> top = options.center + half_h_vec;
    vec3<real> bot = options.center - half_h_vec;

    gpu.push_state();
    gpu.enable_lighting(false);
    gpu.enable_texture(false);
    gpu.color(options.color);
    gpu.begin(primitive_type::lines);

    int segments = 24;
    for (int i = 0; i < segments; ++i)
    {
        real th1 = real(i) * real(6.28318) / real(segments);
        real th2 = real(i + 1) * real(6.28318) / real(segments);

        vec3<real> p1_local(
            cos(th1) * options.radius, 0, sin(th1) * options.radius);
        vec3<real> p2_local(
            cos(th2) * options.radius, 0, sin(th2) * options.radius);

        gpu.vertex(top + options.rotation * p1_local);
        gpu.vertex(top + options.rotation * p2_local);

        gpu.vertex(bot + options.rotation * p1_local);
        gpu.vertex(bot + options.rotation * p2_local);
    }

    // Connecting lines
    vec3<real> r = options.rotation * vec3<real>(1, 0, 0) * options.radius;
    vec3<real> f = options.rotation * vec3<real>(0, 0, 1) * options.radius;

    gpu.vertex(top + r);
    gpu.vertex(bot + r);
    gpu.vertex(top - r);
    gpu.vertex(bot - r);
    gpu.vertex(top + f);
    gpu.vertex(bot + f);
    gpu.vertex(top - f);
    gpu.vertex(bot - f);

    gpu.end();
    gpu.pop_state();
}

void draw_wire_tapered_capsule(gpu &gpu,
                               const wire_tapered_capsule_options &options)
{
    // Draw two spheres and a tapered cylinder body
    vec3<real> half_h_vec =
        options.rotation * vec3<real>(0, options.height * 0.5f, 0);

    // Centers of the spheres
    // Total height includes radii.
    // Distance between sphere centers = height - top_radius - bottom_radius
    real cylinder_height =
        options.height - options.top_radius - options.bottom_radius;
    vec3<real> half_cyl_vec =
        options.rotation * vec3<real>(0, cylinder_height * 0.5f, 0);

    vec3<real> top = options.center + half_cyl_vec;
    vec3<real> bot = options.center - half_cyl_vec;

    wire_sphere_options top_sphere = {top, options.top_radius, options.color};
    wire_sphere_options bot_sphere = {
        bot, options.bottom_radius, options.color};
    draw_wire_sphere(gpu, top_sphere);
    draw_wire_sphere(gpu, bot_sphere);

    // Connecting lines
    gpu.push_state();
    gpu.enable_lighting(false);
    gpu.enable_texture(false);
    gpu.color(options.color);
    gpu.begin(primitive_type::lines);

    // Local axes
    vec3<real> r = options.rotation * vec3<real>(1, 0, 0);
    vec3<real> f = options.rotation * vec3<real>(0, 0, 1);

    gpu.vertex(top + r * options.top_radius);
    gpu.vertex(bot + r * options.bottom_radius);

    gpu.vertex(top - r * options.top_radius);
    gpu.vertex(bot - r * options.bottom_radius);

    gpu.vertex(top + f * options.top_radius);
    gpu.vertex(bot + f * options.bottom_radius);

    gpu.vertex(top - f * options.top_radius);
    gpu.vertex(bot - f * options.bottom_radius);

    gpu.end();
    gpu.pop_state();
}

void draw_wire_tapered_cylinder(gpu &gpu,
                                const wire_tapered_cylinder_options &options)
{
    vec3<real> half_h_vec =
        options.rotation * vec3<real>(0, options.height * 0.5f, 0);
    vec3<real> top = options.center + half_h_vec;
    vec3<real> bot = options.center - half_h_vec;

    gpu.push_state();
    gpu.enable_lighting(false);
    gpu.enable_texture(false);
    gpu.color(options.color);
    gpu.begin(primitive_type::lines);

    int segments = 24;
    for (int i = 0; i < segments; ++i)
    {
        real th1 = real(i) * real(6.28318) / real(segments);
        real th2 = real(i + 1) * real(6.28318) / real(segments);

        vec3<real> p1_local_unit(cos(th1), 0, sin(th1));
        vec3<real> p2_local_unit(cos(th2), 0, sin(th2));

        // Top circle
        gpu.vertex(top +
                   options.rotation * (p1_local_unit * options.top_radius));
        gpu.vertex(top +
                   options.rotation * (p2_local_unit * options.top_radius));

        // Bottom circle
        gpu.vertex(bot +
                   options.rotation * (p1_local_unit * options.bottom_radius));
        gpu.vertex(bot +
                   options.rotation * (p2_local_unit * options.bottom_radius));
    }

    // Connecting lines
    vec3<real> r = options.rotation * vec3<real>(1, 0, 0);
    vec3<real> f = options.rotation * vec3<real>(0, 0, 1);

    gpu.vertex(top + r * options.top_radius);
    gpu.vertex(bot + r * options.bottom_radius);

    gpu.vertex(top - r * options.top_radius);
    gpu.vertex(bot - r * options.bottom_radius);

    gpu.vertex(top + f * options.top_radius);
    gpu.vertex(bot + f * options.bottom_radius);

    gpu.vertex(top - f * options.top_radius);
    gpu.vertex(bot - f * options.bottom_radius);

    gpu.end();
    gpu.pop_state();
}
} // namespace zabato