#pragma once

#include <zabato/jolt_physics_world.hpp>
#include <zabato/physics/rigid_body_controller.hpp>
#include <zabato/physics/types.hpp>

namespace zabato::physics::jolt
{

class jolt_rigid_body_controller : public physics::rigid_body_controller
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    JPH::BodyID m_body_id;
    class jolt_physics_world *m_world;
    physics::rigid_body_creation_config m_config;

    jolt_rigid_body_controller();
    jolt_rigid_body_controller(JPH::BodyID id,
                               jolt_physics_world *w,
                               const physics::rigid_body_creation_config &cfg);
    virtual ~jolt_rigid_body_controller();

    void initialize_with_world(jolt_physics_world *w);
    void start() override;

    void set_position(const vec3<real> &pos) override;
    vec3<real> get_position() const override;
    void set_rotation(const quat<real> &rot) override;
    quat<real> get_rotation() const override;
    void set_linear_velocity(const vec3<real> &velocity) override;
    vec3<real> get_linear_velocity() const override;
    void set_angular_velocity(const vec3<real> &velocity) override;
    vec3<real> get_angular_velocity() const override;
    void activate() override;
    void deactivate() override;
    bool is_active() const override;

    void on_draw_gizmos(gpu &g, bool selected) override;
    void set_config(const physics::rigid_body_creation_config &config) override;
    physics::rigid_body_creation_config get_config() const override;

    void add_force(const vec3<real> &force) override;
    void add_force(const vec3<real> &force, const vec3<real> &point) override;
    void add_torque(const vec3<real> &torque) override;

    void add_impulse(const vec3<real> &impulse) override;
    void add_impulse(const vec3<real> &impulse,
                     const vec3<real> &point) override;
    void add_angular_impulse(const vec3<real> &impulse) override;

    void create_body();
    void destroy_body();

private:
};

} // namespace zabato::physics::jolt