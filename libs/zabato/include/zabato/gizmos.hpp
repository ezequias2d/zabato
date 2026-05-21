#pragma once

#include <zabato/camera.hpp>
#include <zabato/collision.hpp>
#include <zabato/color.hpp>
#include <zabato/gpu.hpp>
#include <zabato/picking.hpp>
#include <zabato/real.hpp>

namespace zabato
{

/**
 * @brief Options for drawing the grid.
 */
struct grid_options
{
    /** @brief The camera used for rendering. */
    camera &cam;
    /** @brief The total size of the grid. */
    int size = 100;
    /** @brief The number of steps (lines) in the grid. */
    int steps = 10;
    /** @brief The color of the grid lines. */
    zabato::color color = {0.5f, 0.5f, 0.5f, 1.0f};
};

/**
 * @brief Draws a grid on the XZ plane.
 * @param gpu The GPU interface.
 * @param options The grid options.
 */
void draw_grid(gpu &gpu, const grid_options &options);

real dist_ray_segment(const vec3<real> &r_origin,
                      const vec3<real> &r_dir,
                      const vec3<real> &p1,
                      const vec3<real> &p2);

struct orientation_gizmo_options
{
    /** @brief The camera used for rendering. */
    camera &cam;
    /** @brief The position of the gizmo in world space. */
    vec2<real> position;
    /** @brief The size of the gizmo. */
    real size;
    /** @brief The direction of the gizmo. */
    vec3<real> &out_dir;
};

bool draw_orientation_gizmo(gpu &gpu, const orientation_gizmo_options &options);

/**
 * @brief Options for the move gizmo (translation manipulator).
 */
struct move_gizmo_options
{
    /** @brief The camera used for rendering. */
    camera &cam;
    /** @brief The position of the gizmo in world space. */
    vec3<real> position;
    /** @brief Whether the mouse button is currently pressed. */
    bool is_mouse_down;
    /** @brief The ray cast from the mouse cursor. */
    ray3<real> mouse_ray;
    /** @brief Optional output flag indicating if the gizmo is hovered. */
    bool *out_hovered = nullptr;
};

/**
 * @brief Draws and handles interaction for a move gizmo.
 * @param gpu The GPU interface.
 * @param cam The camera.
 * @param options The gizmo options.
 * @return True if the gizmo position was modified.
 */
bool draw_move_gizmo(gpu &gpu, move_gizmo_options &options);

/**
 * @brief Options for the rotate gizmo (rotation manipulator).
 */
struct rotate_gizmo_options
{
    /** @brief The camera used for rendering. */
    camera &cam;
    /** @brief The position of the gizmo in world space. */
    vec3<real> position;
    /** @brief The current rotation of the gizmo. */
    quat<real> rotation;
    /** @brief Whether the mouse button is currently pressed. */
    bool is_mouse_down;
    /** @brief The ray cast from the mouse cursor. */
    ray3<real> mouse_ray;
    /** @brief Optional output flag indicating if the gizmo is hovered. */
    bool *out_hovered = nullptr;
};

/**
 * @brief Draws and handles interaction for a rotate gizmo.
 * @param gpu The GPU interface.
 * @param options The gizmo options.
 * @return True if the gizmo rotation was modified.
 */
bool draw_rotate_gizmo(gpu &gpu, rotate_gizmo_options &options);

/**
 * @brief Options for the scale gizmo (scaling manipulator).
 */
struct scale_gizmo_options
{
    /** @brief The camera used for rendering. */
    camera &cam;
    /** @brief The position of the gizmo in world space. */
    vec3<real> position;
    /** @brief The current scale of the gizmo. */
    vec3<real> scale;
    /** @brief The rotation of the gizmo. */
    quat<real> rotation;
    /** @brief Whether the mouse button is currently pressed. */
    bool is_mouse_down;
    /** @brief The ray cast from the mouse cursor. */
    ray3<real> mouse_ray;
    /** @brief Optional output flag indicating if the gizmo is hovered. */
    bool *out_hovered = nullptr;
};

/**
 * @brief Draws and handles interaction for a scale gizmo.
 * @param gpu The GPU interface.
 * @param options The gizmo options.
 * @return True if the gizmo scale was modified.
 */
bool draw_scale_gizmo(gpu &gpu, scale_gizmo_options &options);

/**
 * @brief Options for drawing an icon gizmo.
 */
struct icon_gizmo_options
{
    /** @brief The camera used for rendering. */
    camera &cam;
    /** @brief The texture to use as an icon. */
    texture *icon;
    /** @brief The UV coordinates for the icon within the texture. */
    box2<real> uv;
    /** @brief The position of the icon in world space. */
    vec3<real> position;
    /** @brief The tint color of the icon. */
    zabato::color color;
    /** @brief The ray used for hit testing. */
    ray3<real> mouse_ray;
    /** @brief Optional output flag indicating if the icon is hovered. */
    bool *out_hovered = nullptr;
    /** @brief Optional output for the hit distance along the ray. */
    real *out_t = nullptr;
};

/**
 * @brief Draws a billboarded icon gizmo that faces the camera.
 * @param gpu The GPU interface.
 * @param options The icon options.
 * @return True if the icon was intersected by the mouse ray.
 */
bool draw_icon_gizmo(gpu &gpu, icon_gizmo_options &options);

/**
 * @brief Options for drawing a 3D line.
 */
struct line_options
{
    /** @brief The starting point of the line. */
    vec3<real> start;
    /** @brief The ending point of the line. */
    vec3<real> end;
    /** @brief The color of the line. */
    zabato::color color;
};

/**
 * @brief Draws a line between two points.
 * @param gpu The GPU interface.
 * @param options The line options.
 */
void draw_line(gpu &gpu, const line_options &options);

/**
 * @brief Options for drawing a solid cube.
 */
struct cube_options
{
    /** @brief The center of the cube. */
    vec3<real> center;
    /** @brief The size (dimensions) of the cube. */
    vec3<real> size;
    /** @brief The color of the cube. */
    zabato::color color;
};

/**
 * @brief Draws a solid cube.
 * @param gpu The GPU interface.
 * @param options The cube options.
 */
void draw_cube(gpu &gpu, const cube_options &options);

/**
 * @brief Options for drawing a solid cylinder.
 */
struct cylinder_options
{
    /** @brief The starting point of the cylinder's axis. */
    vec3<real> start;
    /** @brief The ending point of the cylinder's axis. */
    vec3<real> end;
    /** @brief The radius of the cylinder. */
    real radius;
    /** @brief The color of the cylinder. */
    zabato::color color;
};

/**
 * @brief Draws a solid cylinder.
 * @param gpu The GPU interface.
 * @param options The cylinder options.
 */
void draw_cylinder(gpu &gpu, const cylinder_options &options);

/**
 * @brief Options for drawing a solid cone.
 */
struct cone_options
{
    /** @brief The center position of the cone's base. */
    vec3<real> base;
    /** @brief The position of the cone's tip. */
    vec3<real> tip;
    /** @brief The radius of the cone's base. */
    real radius;
    /** @brief The color of the cone. */
    zabato::color color;
};

/**
 * @brief Draws a solid cone.
 * @param gpu The GPU interface.
 * @param options The cone options.
 */
void draw_cone(gpu &gpu, const cone_options &options);

/**
 * @brief Options for drawing a wireframe sphere.
 */
struct wire_sphere_options
{
    /** @brief The center of the sphere. */
    vec3<real> center;
    /** @brief The radius of the sphere. */
    real radius;
    /** @brief The color of the wireframe. */
    zabato::color color;
};

/**
 * @brief Draws a wireframe sphere.
 * @param gpu The GPU interface.
 * @param options The wireframe sphere options.
 */
void draw_wire_sphere(gpu &gpu, const wire_sphere_options &options);

/**
 * @brief Options for drawing a wireframe cone.
 */
struct wire_cone_options
{
    /** @brief The center position of the cone's base. */
    vec3<real> base;
    /** @brief The position of the cone's tip. */
    vec3<real> tip;
    /** @brief The radius of the cone's base. */
    real radius;
    /** @brief The color of the wireframe. */
    zabato::color color;
};

/**
 * @brief Draws a wireframe cone.
 * @param gpu The GPU interface.
 * @param options The wireframe cone options.
 */
void draw_wire_cone(gpu &gpu, const wire_cone_options &options);

/**
 * @brief Options for drawing a camera frustum wireframe.
 */
struct wire_frustum_options
{
    /** @brief The camera whose frustum to draw. */
    camera &cam;
    /** @brief The color of the wireframe. */
    zabato::color color;
};

/**
 * @brief Draws a wireframe representation of a camera frustum.
 * @param gpu The GPU interface.
 * @param options The frustum options.
 */
void draw_wire_frustum(gpu &gpu, const wire_frustum_options &options);

/**
 * @brief Options for drawing a wireframe mesh.
 */
struct wire_mesh_options
{
    /** @brief The mesh to draw. */
    mesh &m;
    /** @brief The bone matrices for skinning, if applicable. */
    const vector<mat4<real>> *bone_matrices = nullptr;
    /** @brief The color of the wireframe. */
    zabato::color color;
};

/**
 * @brief Draws a wireframe representation of a mesh.
 * @param gpu The GPU interface.
 * @param options The wireframe mesh options.
 */
void draw_wire_mesh(gpu &gpu, const wire_mesh_options &options);

/**
 * @brief Options for drawing a model's skeleton.
 */
struct draw_skeleton_options
{
    /** @brief The model to extract bones from. */
    model &m;
    /** @brief The color for the joint connections (bones). */
    color bone_color = color::green();
    /** @brief The color for the joints. */
    color joint_color = color::yellow();
    /** @brief Radius of the joints. */
    real joint_radius = 0.05f;
    /** @brief If true, the skeleton renders on top of everything. */
    bool x_ray = true;
};

/**
 * @brief Draws a skeleton over a model (useful for X-Ray debugging).
 * @param gpu The GPU interface.
 * @param options The skeleton drawing options.
 */
void draw_skeleton(gpu &gpu, const draw_skeleton_options &options);

/**
 * @brief Options for drawing a bone gizmo.
 */
struct bone_gizmo_options
{
    /** @brief The start point of the bone (parent joint). */
    vec3<real> start;
    /** @brief The end point of the bone (child joint). */
    vec3<real> end;
    /** @brief The radius/thickness of the bone. */
    real radius;
    /** @brief The color of the bone. */
    zabato::color color;
    /**
     * @brief World-space orientation of the bone. Used to rotate the
     * octahedron's cross-section around its head-tail axis so the gizmo
     * visibly rolls with the bone (matches Blender/Unity). Defaults to
     * identity, in which case a world-axis fallback is used.
     */
    quat<real> orientation = quat<real>();
};

/**
 * @brief Draws a bone (octahedron-like shape) from start to end.
 * @param gpu The GPU interface.
 * @param options The bone options.
 */
void draw_bone(gpu &gpu, const bone_gizmo_options &options);

/**
 * @brief Options for drawing a wireframe box.
 */
struct wire_box_options
{
    /** @brief The center of the box. */
    vec3<real> center;
    /** @brief The half-extents (dimensions/2) of the box. */
    vec3<real> half_extents;
    /** @brief The rotation of the box. */
    quat<real> rotation;
    /** @brief The color of the wireframe. */
    zabato::color color;
};

/**
 * @brief Draws a wireframe box.
 * @param gpu The GPU interface.
 * @param options The wireframe box options.
 */
void draw_wire_box(gpu &gpu, const wire_box_options &options);

/**
 * @brief Options for drawing a wireframe capsule.
 */
struct wire_capsule_options
{
    /** @brief The center of the capsule. */
    vec3<real> center;
    /** @brief The total height of the capsule(including hemispherical ends). */
    real height;
    /** @brief The radius of the capsule. */
    real radius;
    /** @brief The rotation of the capsule. */
    quat<real> rotation;
    /** @brief The color of the wireframe. */
    zabato::color color;
};

/**
 * @brief Draws a wireframe capsule.
 * @param gpu The GPU interface.
 * @param options The wireframe capsule options.
 */
void draw_wire_capsule(gpu &gpu, const wire_capsule_options &options);

/**
 * @brief Options for drawing a wireframe cylinder.
 */
struct wire_cylinder_options
{
    /** @brief The geometric center of the cylinder. */
    vec3<real> center;
    /** @brief The height of the cylinder. */
    real height;
    /** @brief The radius of the cylinder. */
    real radius;
    /** @brief The rotation of the cylinder. */
    quat<real> rotation;
    /** @brief The color of the wireframe. */
    zabato::color color;
};

/**
 * @brief Draws a wireframe cylinder.
 * @param gpu The GPU interface.
 * @param options The wireframe cylinder options.
 */
void draw_wire_cylinder(gpu &gpu, const wire_cylinder_options &options);

/**
 * @brief Options for drawing a wireframe tapered capsule.
 */
struct wire_tapered_capsule_options
{
    /** @brief The center of the capsule. */
    vec3<real> center;
    /** @brief The total height of the capsule(including hemispherical ends). */
    real height;
    /** @brief The top radius of the capsule. */
    real top_radius;
    /** @brief The bottom radius of the capsule. */
    real bottom_radius;
    /** @brief The rotation of the capsule. */
    quat<real> rotation;
    /** @brief The color of the wireframe. */
    zabato::color color;
};

/**
 * @brief Draws a wireframe tapered capsule.
 * @param gpu The GPU interface.
 * @param options The wireframe tapered capsule options.
 */
void draw_wire_tapered_capsule(gpu &gpu,
                               const wire_tapered_capsule_options &options);

/**
 * @brief Options for drawing a wireframe tapered cylinder.
 */
struct wire_tapered_cylinder_options
{
    /** @brief The geometric center of the cylinder. */
    vec3<real> center;
    /** @brief The height of the cylinder. */
    real height;
    /** @brief The top radius of the cylinder. */
    real top_radius;
    /** @brief The bottom radius of the cylinder. */
    real bottom_radius;
    /** @brief The rotation of the cylinder. */
    quat<real> rotation;
    /** @brief The color of the wireframe. */
    zabato::color color;
};

/**
 * @brief Draws a wireframe tapered cylinder.
 * @param gpu The GPU interface.
 * @param options The wireframe tapered cylinder options.
 */
void draw_wire_tapered_cylinder(gpu &gpu,
                                const wire_tapered_cylinder_options &options);

} // namespace zabato
