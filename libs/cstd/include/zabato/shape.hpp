#pragma once

#include <zabato/math.hpp>

namespace zabato
{

template <typename T> struct sphere3;

template <typename T> struct box2
{
    vec2<T> min;
    vec2<T> max;

    constexpr box2() : min(0, 0), max(0, 0) {}
    constexpr box2(const vec2<T> &min_val, const vec2<T> &max_val)
        : min(min_val), max(max_val)
    {
    }

    constexpr bool contains(const vec2<T> &p) const
    {
        return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
    }

    constexpr bool contains(const box2<T> &other) const
    {
        return min.x <= other.min.x && max.x >= other.max.x &&
               min.y <= other.min.y && max.y >= other.max.y;
    }

    constexpr bool intersects_with(const box2<T> &other) const
    {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y;
    }

    constexpr vec2<T> center() const { return (min + max) * T(0.5); }
    constexpr vec2<T> size() const { return max - min; }

    constexpr box2<T> union_with(const box2<T> &other) const
    {
        return box2<T>(min(min, other.min), max(max, other.max));
    }

    constexpr box2<T> intersect_with(const box2<T> &other) const
    {
        return box2<T>(max(min, other.min), min(max, other.max));
    }

    constexpr box2<T> inflate(T delta) const
    {
        return box2<T>(min - vec2<T>(delta), max + vec2<T>(delta));
    }

    constexpr box2<T> inflate(const vec2<T> &delta) const
    {
        return box2<T>(min - delta, max + delta);
    }

    constexpr box2<T> offset(const vec2<T> &delta) const
    {
        return box2<T>(min + delta, max + delta);
    }
};

template <typename T> struct box3
{
    vec3<T> min;
    vec3<T> max;

    constexpr box3() : min(0, 0, 0), max(0, 0, 0) {}
    constexpr box3(const vec3<T> &min_val, const vec3<T> &max_val)
        : min(min_val), max(max_val)
    {
    }

    constexpr bool contains(const vec3<T> &p) const
    {
        return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y &&
               p.z >= min.z && p.z <= max.z;
    }

    constexpr bool contains(const box3<T> &other) const
    {
        return min.x <= other.min.x && max.x >= other.max.x &&
               min.y <= other.min.y && max.y >= other.max.y &&
               min.z <= other.min.z && max.z >= other.max.z;
    }

    constexpr bool intersects_with(const box3<T> &other) const
    {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    constexpr bool intersects_with(const sphere3<T> &sphere) const;

    constexpr vec3<T> center() const { return (min + max) * T(0.5); }
    constexpr vec3<T> size() const { return max - min; }

    constexpr box3<T> union_with(const box3<T> &other) const
    {
        return box3<T>(min(min, other.min), max(max, other.max));
    }

    constexpr box3<T> intersect_with(const box3<T> &other) const
    {
        return box3<T>(max(min, other.min), min(max, other.max));
    }

    constexpr box3<T> inflate(T delta) const
    {
        return box3<T>(min - vec3<T>(delta), max + vec3<T>(delta));
    }

    constexpr box3<T> inflate(const vec3<T> &delta) const
    {
        return box3<T>(min - delta, max + delta);
    }

    constexpr box3<T> offset(const vec3<T> &delta) const
    {
        return box3<T>(min + delta, max + delta);
    }
};

template <typename T> struct sphere3
{
    vec3<T> center;
    T radius;

    constexpr sphere3() : center(0, 0, 0), radius(0) {}
    constexpr sphere3(const vec3<T> &center_val, T radius_val)
        : center(center_val), radius(radius_val)
    {
    }

    constexpr bool contains(const vec3<T> &p) const
    {
        return distance_sq(center, p) <= radius * radius;
    }

    constexpr bool contains(const sphere3<T> &other) const
    {
        return distance(center, other.center) + other.radius <= radius;
    }

    constexpr bool intersects_with(const sphere3<T> &other) const
    {
        return distance_sq(center, other.center) <=
               (radius + other.radius) * (radius + other.radius);
    }

    constexpr bool intersects_with(const box3<T> &box) const
    {
        return box.intersects_with(*this);
    }

    constexpr sphere3<T> inflate(T delta) const
    {
        return sphere3<T>(center, radius + delta);
    }

    constexpr sphere3<T> offset(const vec3<T> &delta) const
    {
        return sphere3<T>(center + delta, radius);
    }
};

template <typename T>
constexpr bool box3<T>::intersects_with(const sphere3<T> &sphere) const
{
    // Find point on box closest to sphere center
    vec3<T> closest = zabato::max(min, zabato::min(sphere.center, this->max));
    return distance_sq(closest, sphere.center) <= sphere.radius * sphere.radius;
}

/**
 * @brief A structure representing a plane in 3D space.
 *
 * Defined by a normal vector and a distance from the origin (Hessian normal
 * form).
 *
 * @tparam T The underlying numeric type.
 */
template <typename T> struct plane3
{
    /** @brief The normal vector of the plane. */
    vec3<T> normal;
    /** @brief The distance from the origin to the plane. */
    T d;

    /**
     * @brief Default constructor. Initializes plane with normal (0, 1, 0) and
     * distance 0.
     */
    constexpr plane3() : normal(0, 1, 0), d(0) {}

    /**
     * @brief Constructs a plane from a normal and a distance.
     * @param n The normal vector.
     * @param d_val The distance value.
     */
    constexpr plane3(const vec3<T> &n, T d_val) : normal(n), d(d_val) {}

    /**
     * @brief Constructs a plane from normal components and a distance.
     * @param a The x component of the normal.
     * @param b The y component of the normal.
     * @param c The z component of the normal.
     * @param d_val The distance value.
     */
    constexpr plane3(T a, T b, T c, T d_val) : normal(a, b, c), d(d_val) {}

    /**
     * @brief Creates a plane from three points.
     * @param p1 The first point.
     * @param p2 The second point.
     * @param p3 The third point.
     * @return The constructed plane.
     */
    static constexpr plane3<T>
    from_points(const vec3<T> &p1, const vec3<T> &p2, const vec3<T> &p3)
    {
        vec3<T> normal = normalize(cross(p2 - p1, p3 - p1));
        return plane3<T>(normal, -dot(normal, p1));
    }

    /**
     * @brief Creates a plane from a point and a normal.
     * @param point A point on the plane.
     * @param normal The normal vector of the plane.
     * @return The constructed plane.
     */
    static constexpr plane3<T> from_normal(const vec3<T> &point,
                                           const vec3<T> &normal)
    {
        return plane3<T>(normal, -dot(normal, point));
    }

    /**
     * @brief Calculates the signed distance from a point to a plane.
     * @param v The point.
     * @return The signed distance (positive if point is on the side of the
     * normal).
     */
    constexpr T signed_distance(const vec3<T> &v) const
    {
        return dot(normal, v) + d;
    }
};

/**
 * @brief Normalizes a plane (normalizes the normal vector and scales distance).
 * @param p The plane to normalize.
 * @return The normalized plane.
 */
template <typename T> constexpr plane3<T> normalize(const plane3<T> &p)
{
    T len     = length(p.normal);
    T inv_len = T(1) / len;
    return plane3<T>(p.normal * inv_len, p.d * inv_len);
}

template <typename T> struct triangle3
{
    union
    {
        struct
        {
            vec3<T> v0, v1, v2;
        };
        vec3<T> v[3];
    };

    constexpr triangle3() : v0(0, 0, 0), v1(0, 0, 0), v2(0, 0, 0) {}
    constexpr triangle3(const vec3<T> &v0_val,
                        const vec3<T> &v1_val,
                        const vec3<T> &v2_val)
        : v0(v0_val), v1(v1_val), v2(v2_val)
    {
    }

    constexpr bool contains(const vec3<T> &p) const
    {
        vec3<T> e1    = v1 - v0;
        vec3<T> e2    = v2 - v0;
        vec3<T> p_rel = p - v0;
        T det         = e1.x * e2.y - e1.y * e2.x;
        T u           = (p_rel.x * e2.y - p_rel.y * e2.x) / det;
        T v           = (p_rel.y * e1.x - p_rel.x * e1.y) / det;
        return u >= T(0) && v >= T(0) && u + v <= T(1);
    }

    constexpr bool intersects_with(const plane3<T> &plane,
                                   vec3<T> &out_p1,
                                   vec3<T> &out_p2) const
    {
        T d[3] = {plane.signed_distance(v0),
                  plane.signed_distance(v1),
                  plane.signed_distance(v2)};

        // If all vertices are on the same side, no intersection
        if ((d[0] > 0 && d[1] > 0 && d[2] > 0) ||
            (d[0] < 0 && d[1] < 0 && d[2] < 0))
            return false;

        vec3<T> pts[2];
        int count = 0;

        auto add_unique_point = [&](const vec3<T> &p)
        {
            if (count >= 2)
                return;

            // Check for duplicates
            for (int k = 0; k < count; ++k)
                if (distance_sq(pts[k], p) < T::epsilon())
                    return;

            pts[count++] = p;
        };

        for (int i = 0; i < 3; i++)
        {
            // Check if vertex is on plane
            if (abs(d[i]) < T::epsilon())
                add_unique_point(v[i]);

            // Check if edge crosses plane strictly
            int next_i = (i + 1) % 3;
            if (d[i] * d[next_i] < -T::epsilon())
            {
                T t = d[i] / (d[i] - d[next_i]);
                add_unique_point(v[i] + (v[next_i] - v[i]) * t);
            }
        }

        if (count == 2)
        {
            out_p1 = pts[0];
            out_p2 = pts[1];
            return true;
        }

        // Handle cases where we found less than 2 distinct points
        if (count == 1)
        {
            out_p1 = pts[0];
            out_p2 = pts[0];
            return true;
        }

        return false;
    }
};

template <typename T> struct quad3
{
    union
    {
        struct
        {
            vec3<T> v0, v1, v2, v3;
        };
        vec3<T> v[4];
    };

    constexpr quad3() : v0(0, 0, 0), v1(0, 0, 0), v2(0, 0, 0), v3(0, 0, 0) {}
    constexpr quad3(const vec3<T> &v0_val,
                    const vec3<T> &v1_val,
                    const vec3<T> &v2_val,
                    const vec3<T> &v3_val)
        : v0(v0_val), v1(v1_val), v2(v2_val), v3(v3_val)
    {
    }
    constexpr bool contains(const vec3<T> &p) const
    {
        triangle3<T> t1(v0, v1, v2);
        triangle3<T> t2(v0, v2, v3);
        return t1.contains(p) || t2.contains(p);
    }

    constexpr bool intersects_with(const plane3<T> &plane,
                                   vec3<T> &out_p1,
                                   vec3<T> &out_p2) const
    {
        T d[4] = {plane.signed_distance(v0),
                  plane.signed_distance(v1),
                  plane.signed_distance(v2),
                  plane.signed_distance(v3)};

        // If all vertices are on the same side, no intersection
        bool all_pos = true;
        bool all_neg = true;
        for (int i = 0; i < 4; ++i)
        {
            if (d[i] <= 0)
                all_pos = false;
            if (d[i] >= 0)
                all_neg = false;
        }
        if (all_pos || all_neg)
            return false;

        vec3<T> pts[2];
        int count = 0;

        auto add_unique_point = [&](const vec3<T> &p)
        {
            if (count >= 2)
                return;
            for (int k = 0; k < count; ++k)
            {
                if (distance_sq(pts[k], p) < T::epsilon())
                    return;
            }
            pts[count++] = p;
        };

        for (int i = 0; i < 4; i++)
        {
            if (abs(d[i]) < T::epsilon())
            {
                add_unique_point(v[i]);
            }

            int next_i = (i + 1) % 4;
            if (d[i] * d[next_i] < -T::epsilon())
            {
                T t = d[i] / (d[i] - d[next_i]);
                add_unique_point(v[i] + (v[next_i] - v[i]) * t);
            }
        }

        if (count == 2)
        {
            out_p1 = pts[0];
            out_p2 = pts[1];
            return true;
        }

        if (count == 1)
        {
            out_p1 = pts[0];
            out_p2 = pts[0];
            return true;
        }

        return false;
    }
};

template <typename T> struct ray3
{
    vec3<T> origin;
    vec3<T> direction;

    constexpr ray3() : origin(0, 0, 0), direction(0, 0, 1) {}
    constexpr ray3(const vec3<T> &origin_val, const vec3<T> &direction_val)
        : origin(origin_val), direction(direction_val)
    {
    }

    constexpr vec3<T> point_at(T t) const { return origin + direction * t; }

    constexpr bool intersects_with(const box3<T> &box, T &t) const
    {

        T tmin = T::min_val();
        T tmax = T::max_val();

        for (int i = 0; i < 3; i++)
        {
            if (abs(direction[i]) < T::epsilon())
            {
                if (origin[i] < box.min[i] || origin[i] > box.max[i])
                    return false;
            }
            else
            {
                T invd = T(1) / direction[i];
                T t1   = (box.min[i] - origin[i]) * invd;
                T t2   = (box.max[i] - origin[i]) * invd;
                if (t1 > t2)
                    swap(t1, t2);
                tmin = max(tmin, t1);
                tmax = min(tmax, t2);
                if (tmin > tmax)
                    return false;
            }
        }

        t = tmin;
        return true;
    }

    constexpr bool intersects_with(const sphere3<T> &sphere, T &t) const
    {
        vec3<T> oc     = origin - sphere.center;
        T a            = dot(direction, direction);
        T b            = T(2.0) * dot(oc, direction);
        T c            = dot(oc, oc) - sphere.radius * sphere.radius;
        T discriminant = b * b - T(4.0) * a * c;
        if (discriminant < 0)
            return false;
        t = (-b - sqrt(discriminant)) / (T(2.0) * a);
        return true;
    }

    constexpr bool intersects_with(const plane3<T> &plane, T &t) const
    {
        T denom = dot(plane.normal, direction);
        if (abs(denom) < T::epsilon())
            return false;
        t = (plane.distance - dot(plane.normal, origin)) / denom;
        return true;
    }

    constexpr bool intersects_with(const triangle3<T> &triangle, T &t) const
    {
        vec3<T> edge1 = triangle.v1 - triangle.v0;
        vec3<T> edge2 = triangle.v2 - triangle.v0;
        vec3<T> pvec  = cross(direction, edge2);
        T det         = dot(edge1, pvec);

        if (abs(det) < T::epsilon())
            return false;

        T invDet     = T(1) / det;
        vec3<T> tvec = origin - triangle.v0;

        T u = dot(tvec, pvec) * invDet;
        if (u < 0 || u > 1)
            return false;

        vec3<T> qvec = cross(tvec, edge1);
        T v          = dot(direction, qvec) * invDet;
        if (v < 0 || u + v > 1)
            return false;

        t = dot(edge2, qvec) * invDet;
        return t > T::epsilon();
    }

    constexpr bool intersects_with(const quad3<T> &quad, T &t) const
    {
        triangle3<T> t1(quad.v0, quad.v1, quad.v2);
        triangle3<T> t2(quad.v0, quad.v2, quad.v3);

        T t1_val, t2_val;
        bool hit1 = intersects_with(t1, t1_val);
        bool hit2 = intersects_with(t2, t2_val);

        if (hit1 && hit2)
        {
            t = min(t1_val, t2_val);
            return true;
        }
        if (hit1)
        {
            t = t1_val;
            return true;
        }
        if (hit2)
        {
            t = t2_val;
            return true;
        }
        return false;
    }
};

} // namespace zabato