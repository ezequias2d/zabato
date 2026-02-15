#pragma once

#include <zabato/controller.hpp>
#include <zabato/event.hpp>
#include <zabato/math.hpp>
#include <zabato/spatial.hpp>
#include <zabato/transformation.hpp>

namespace zabato::physics
{
class physics_controller : public controller
{
protected:
    transformation m_last_transform;
    event<>::scoped_connection m_transform_connection;
    bool m_updating_from_physics = false;

public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    virtual void on_draw_gizmos(gpu &g, bool selected) override = 0;

    virtual void activate()        = 0;
    virtual void deactivate()      = 0;
    virtual bool is_active() const = 0;

    virtual void set_position(const vec3<real> &pos) = 0;
    virtual vec3<real> get_position() const          = 0;

    virtual void set_rotation(const quat<real> &rot) = 0;
    virtual quat<real> get_rotation() const          = 0;

    virtual void set_linear_velocity(const vec3<real> &velocity) = 0;
    virtual vec3<real> get_linear_velocity() const               = 0;

    void start_sync()
    {
        if (!m_object && !m_object->is_derived(spatial::TYPE))
            return;

        spatial *s = static_cast<spatial *>(m_object);

        m_transform_connection = s->on_dirty.connect(
            [this, s]()
            {
                if (m_updating_from_physics)
                    return;

                transformation current_t = s->get_world_transform();

                real dist_sq = length_sq(current_t.translate() -
                                         m_last_transform.translate());
                real dot_val =
                    abs(dot(current_t.rotate(), m_last_transform.rotate()));
                bool rot_changed = dot_val < 0.9999f;

                if (dist_sq > 0.0001f || rot_changed)
                {
                    set_position(current_t.translate());
                    set_rotation(current_t.rotate());
                    activate();

                    m_last_transform = current_t;
                }
            });

        m_last_transform = s->get_world_transform();
    }

    void update_sync()
    {
        if (!(m_object && m_object->is_derived(spatial::TYPE)))
            return;

        spatial *s = static_cast<spatial *>(m_object);

        vec3<real> pos = get_position();
        quat<real> rot = get_rotation();

        vec3<real> scale = s->get_world_transform().scale();
        transformation world_t;
        world_t.set_translate(pos);
        world_t.set_rotate(rot);
        world_t.set_scale(scale);

        m_last_transform = world_t;

        m_updating_from_physics = true;
        s->set_world(world_t);
        m_updating_from_physics = false;
    }
};

} // namespace zabato::physics
