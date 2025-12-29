#pragma once

#include <zabato/math.hpp>
#include <zabato/real.hpp>

namespace zabato
{

class transformation
{
public:
    transformation() { make_identity(); }
    ~transformation() {}

    static const transformation IDENTITY;

    void set_rotate(const quat<real> &rotate)
    {
        m_rotation = rotate;
        update_identity_flag();
    }

    quat<real> rotate() const { return m_rotation; }

    void set_translate(const vec3<real> &translate)
    {
        m_translation = translate;
        update_identity_flag();
    }

    vec3<real> translate() const { return m_translation; }

    void set_scale(const vec3<real> &scale)
    {
        m_scale            = scale;
        m_is_uniform_scale = scale.x == scale.y && scale.y == scale.z;
        update_identity_flag();
    }

    vec3<real> scale() const { return m_scale; }

    real get_min_scale() const
    {
        if (m_is_uniform_scale)
            return m_scale.x;
        return min(m_scale.x, min(m_scale.y, m_scale.z));
    }

    real get_max_scale() const
    {
        if (m_is_uniform_scale)
            return m_scale.x;
        return max(m_scale.x, max(m_scale.y, m_scale.z));
    }

    bool is_uniform_scale() const { return m_is_uniform_scale; }

    void set_uniform_scale(real scale)
    {
        m_scale            = vec3<real>(scale);
        m_is_uniform_scale = true;
        update_identity_flag();
    }

    void make_identity()
    {
        m_rotation         = quat<real>();
        m_translation      = vec3<real>();
        m_scale            = vec3<real>(1);
        m_is_uniform_scale = true;
        m_is_identity      = true;
    }

    bool is_identity() const { return m_is_identity; }

    void make_unit_scale()
    {
        m_scale            = vec3<real>(1);
        m_is_uniform_scale = true;
    }

    vec3<real> apply_forward(const vec3<real> &point) const
    {
        if (m_is_identity)
            return point;

        vec3<real> result = point;
        result            = result * m_scale;
        result            = m_rotation * result;
        result            = result + m_translation;
        return result;
    }

    void apply_forward(size_t count, vec3<real> *points) const
    {
        for (size_t i = 0; i < count; ++i)
            points[i] = apply_forward(points[i]);
    }

    plane3<real> apply_forward(const plane3<real> &plane) const
    {
        if (m_is_identity)
            return plane;

        vec3<real> p       = plane.normal * -plane.d;
        vec3<real> p_prime = apply_forward(p);

        vec3<real> n_prime = m_rotation * (plane.normal / m_scale);
        n_prime            = normalize(n_prime);

        // Recompute d
        // dot(n', p') + d' = 0  =>  d' = -dot(n', p')
        real d_prime = -dot(n_prime, p_prime);

        return plane3<real>(n_prime, d_prime);
    }

    vec3<real> apply_backward(const vec3<real> &point) const
    {
        if (m_is_identity)
            return point;

        vec3<real> result = point;
        result            = result - m_translation;
        result            = result * m_rotation;
        result            = result / m_scale;
        return result;
    }

    void apply_backward(size_t count, vec3<real> *points) const
    {
        if (m_is_identity)
            return;

        for (size_t i = 0; i < count; ++i)
            points[i] = apply_backward(points[i]);
    }

    void product(const transformation &a, const transformation &b)
    {
        if (a.m_is_identity)
        {
            *this = b;
            return;
        }

        if (b.m_is_identity)
        {
            *this = a;
            return;
        }

        if (a.m_is_uniform_scale)
        {
            m_scale    = a.m_scale * b.m_scale;
            m_rotation = a.m_rotation * b.m_rotation;
            m_translation =
                a.m_translation + a.m_rotation * (a.m_scale * b.m_translation);
            m_is_uniform_scale = b.m_is_uniform_scale;
            m_is_identity      = false;
        }
        else
        {
            mat4<real> ma = mat4_translation(a.m_translation) *
                            mat4_from_quat(a.m_rotation) *
                            mat4_scaling(a.m_scale);
            mat4<real> mb = mat4_translation(b.m_translation) *
                            mat4_from_quat(b.m_rotation) *
                            mat4_scaling(b.m_scale);
            mat4<real> m = ma * mb;
            mat4_decompose(m, m_translation, m_scale, m_rotation);
            m_is_uniform_scale = abs(m_scale.x - m_scale.y) < real::epsilon() &&
                                 abs(m_scale.x - m_scale.z) < real::epsilon();
            m_is_identity = false;
        }
    }

    void inverse(transformation &inv) const
    {
        if (m_is_identity)
        {
            inv.make_identity();
            return;
        }

        if (m_is_uniform_scale)
        {
            inv.m_scale    = vec3<real>(real(1) / m_scale.x);
            inv.m_rotation = zabato::inverse(m_rotation);
            inv.m_translation =
                inv.m_rotation * (-m_translation * inv.m_scale.x);
            inv.m_is_uniform_scale = true;
            inv.m_is_identity      = false;
        }
        else
        {
            mat4<real> m = mat4_translation(m_translation) *
                           mat4_from_quat(m_rotation) * mat4_scaling(m_scale);
            mat4<real> m_inv;
            if (zabato::inverse(m, m_inv))
            {
                mat4_decompose(
                    m_inv, inv.m_translation, inv.m_scale, inv.m_rotation);
                inv.m_is_uniform_scale =
                    abs(inv.m_scale.x - inv.m_scale.y) < real::epsilon() &&
                    abs(inv.m_scale.x - inv.m_scale.z) < real::epsilon();
                inv.m_is_identity = false;
            }
            else
            {
                inv.make_identity();
            }
        }
    }

    void update_identity_flag()
    {
        m_is_identity = (m_translation == vec3<real>(0)) &&
                        (m_scale == vec3<real>(1)) &&
                        (m_rotation == quat<real>());
    }

private:
    quat<real> m_rotation;
    vec3<real> m_translation;
    vec3<real> m_scale;
    bool m_is_identity;
    bool m_is_uniform_scale;
};
} // namespace zabato