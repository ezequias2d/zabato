#pragma once

#include <zabato/math.hpp>
#include <zabato/real.hpp>
#include <zabato/span.hpp>
#include <zabato/transformation.hpp>

namespace zabato
{
class bounding_volume
{
public:
    virtual ~bounding_volume() {}

    const vec3<real> &center() const { return m_center; }
    void set_center(const vec3<real> &center) { m_center = center; }

    real radius() const { return m_radius; }
    void set_radius(real radius) { m_radius = radius; }

    static bounding_volume *create(const vec3<real> &center, real radius);

    virtual void compute_from_data(const span<const vec3<real>> &points) = 0;
    virtual void transform_by(const transformation &transform,
                              bounding_volume *result)                   = 0;

    virtual int which_side(const plane3<real> &p) = 0;

private:
    vec3<real> m_center;
    real m_radius;

protected:
    bounding_volume() {}
    bounding_volume(const vec3<real> &center, real radius)
        : m_center(center), m_radius(radius)
    {
    }
};

class bv_sphere : public bounding_volume
{
public:
    bv_sphere() = default;
    bv_sphere(const vec3<real> &center, real radius)
        : bounding_volume(center, radius)
    {
    }

    virtual void
    compute_from_data(const span<const vec3<real>> &points) override;
    virtual void transform_by(const transformation &transform,
                              bounding_volume *result) override;
    virtual int which_side(const plane3<real> &p) override;
};

} // namespace zabato