#pragma once

#include <zabato/gpu.hpp>
#include <zabato/list.hpp>
#include <zabato/math.hpp>
#include <zabato/real.hpp>
#include <zabato/vector.hpp>
#include <stdint.h>

namespace zabato
{
/** @brief The type of a collision primitive. */
enum class collision_type
{
    none,
    rect,
    circle,
    polygon
};

/** @brief Occlusion levels for determining visibility. */
enum class occlusion_level
{
    none,    ///< Fully visible
    partial, ///< Partially visible
    full,    ///< Fully hidden
};

/** @brief Bitflags for debug drawing of collision shapes. */
enum class collision_debug_flags : uint32_t
{
    none      = 0,
    filled    = 1 << 0, ///< Draw filled shapes.
    wireframe = 1 << 1, ///< Draw wireframe outlines.
    normals   = 1 << 2, ///< Draw normal vectors (for polygons).
    vertices  = 1 << 3, ///< Draw vertices as points (for polygons).
    bounds    = 1 << 4, ///< Draw axis-aligned bounding boxes.
};

inline collision_debug_flags operator|(collision_debug_flags a,
                                       collision_debug_flags b)
{
    return static_cast<collision_debug_flags>(static_cast<uint32_t>(a) |
                                              static_cast<uint32_t>(b));
}

inline collision_debug_flags operator&(collision_debug_flags a,
                                       collision_debug_flags b)
{
    return static_cast<collision_debug_flags>(static_cast<uint32_t>(a) &
                                              static_cast<uint32_t>(b));
}

/** @brief Contains detailed information about a collision event. */
struct collision_result
{
    bool collides = false;
    vec2<real> penetration{};
    vec2<real> normal{};
    real distance = real(0);
};

/** @brief Contains information about a raycast intersection. */
struct raycast_result
{
    bool hit = false;        ///< Whether the ray hit anything
    vec2<real> point{};      ///< Point where the ray hit
    vec2<real> normal{};     ///< Surface normal at hit point
    real distance = real(0); ///< Distance from ray origin to hit point

    const class collision_shape *object = nullptr; ///< Object that was hit
};

/** @brief Defines a vision cone for field-of-view calculations. */
struct vision_cone
{
    vec2<real> position;  ///< Position of the vision cone origin
    vec2<real> direction; ///< Forward direction of vision
    real angle;           ///< Half angle of the cone in radians
    real near_dist;       ///< Minimum vision distance
    real far_dist;        ///< Maximum vision distance
};

/** @brief Contains information about the visibility of an object. */
struct visibility_result
{
    bool in_vision_cone;       ///< Whether the object is in the vision cone
    real distance_to_observer; ///< Distance to the observer
    occlusion_level occlusion; ///< Occlusion level
    real
        visibility_factor; ///< Visibility factor (0-1) where 1 is fully visible

    visibility_result()
    {
        in_vision_cone       = false;
        distance_to_observer = real(0);
        occlusion            = occlusion_level::none;
        visibility_factor    = real(0);
    }
};

/** @brief Contains detailed information about a collision event. */
struct collision_info
{
    bool collision;
    vec2<real> position;
    vec2<real> normal;
    uint32_t collision_id;
    uint32_t material_type;
    uint32_t flags;
    uint32_t height_level;
    void *user_data;
};

/** @brief Abstract base class for all 2D collision shapes. */
class collision_shape
{
public:
    virtual ~collision_shape()              = default;
    virtual collision_type get_type() const = 0;

private:
    list_node link;

public:
    using list = list<collision_shape, &collision_shape::link>;
};

/** @brief A rectangle collision shape. */
class rect_shape : public collision_shape
{
public:
    vec2<real> position;
    vec2<real> size;
    collision_type get_type() const override { return collision_type::rect; }
};

/** @brief A circle collision shape. */
class circle_shape : public collision_shape
{
public:
    vec2<real> position;
    real radius;
    collision_type get_type() const override { return collision_type::circle; }
};

/** @brief A convex polygon collision shape. */
class polygon_shape : public collision_shape
{
public:
    vec2<real> position;
    vector<vec2<real>> vertices;
    collision_type get_type() const override { return collision_type::polygon; }
};
/**
 * @brief Checks if a point is inside a collision shape.
 * @param shape The collision shape to test against.
 * @param position The point to check.
 * @return True if the point is inside the shape, false otherwise.
 */
bool check_collision(const collision_shape &shape, vec2<real> position);

/**
 * @brief Tests for collision between two collision shapes.
 * @param first The first collision shape.
 * @param second The second collision shape.
 * @return A `collision_result` struct containing detailed information about the
 * collision.
 */
collision_result test_collision(const collision_shape &first,
                                const collision_shape &second);

/**
 * @brief Casts a ray and checks for the closest intersection with a set of
 * collision objects.
 * @param origin The starting point of the ray.
 * @param direction The normalized direction of the ray.
 * @param max_distance The maximum distance the ray should travel.
 * @param objects A vector of pointers to collision shapes to test against.
 * @return A `raycast_result` struct. Check `result.hit` to see if an
 * intersection occurred.
 */
raycast_result raycast(vec2<real> origin,
                       vec2<real> direction,
                       real max_distance,
                       const vector<const collision_shape *> &objects);

/**
 * @brief Checks if a point is within the area of a vision cone.
 * @param cone The vision cone to test against.
 * @param point The point to test.
 * @return True if the point is within the cone's angle and distance, false
 * otherwise.
 */
bool is_point_in_vision_cone(const vision_cone &cone, vec2<real> point);

/**
 * @brief Determines the visibility of an object from a vision cone, considering
 * potential obstacles.
 * @param cone The vision cone of the observer.
 * @param object The collision shape of the object to check.
 * @param obstacles A vector of pointers to collision shapes that can block
 * sight.
 * @return A `visibility_result` struct with detailed information about the
 * object's visibility.
 */
visibility_result
check_visibility(const vision_cone &cone,
                 const collision_shape &object,
                 const vector<const collision_shape *> &obstacles);

/**
 * @brief Creates a convex hull from a set of 2D points using the Graham scan
 * algorithm.
 * @param points A vector of input points.
 * @return A vector containing the vertices of the resulting convex hull.
 */
vector<vec2<real>> create_convex_hull(const vector<vec2<real>> &points);

/**
 * @brief Debug draws a collision shape using the provided GPU context.
 * @param gpu The GPU interface to use for drawing.
 * @param shape The collision shape to draw.
 * @param z The Z-depth to draw the 2D shape at.
 * @param c The color to use for drawing.
 * @param flags A bitmask of flags to control the drawing style (e.g., filled,
 * wireframe).
 */
void draw_collision(gpu &gpu,
                    const collision_shape &shape,
                    real z,
                    color c,
                    collision_debug_flags flags);

void draw_vision_cone(gpu &gpu,
                      const vision_cone &cone,
                      real z,
                      color c,
                      uint32_t segments);

} // namespace zabato