#pragma once

#include <zabato/jolt_physics_world.hpp>
#include <zabato/physics/character_controller.hpp>
#include <zabato/physics/types.hpp>

namespace zabato::physics::jolt
{

class jolt_character_controller : public physics::character_controller
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    JPH::Ref<JPH::CharacterVirtual> m_character;
    class jolt_physics_world *m_world;
    physics::character_creation_config m_config;

    jolt_character_controller();
    jolt_character_controller(JPH::Ref<JPH::CharacterVirtual> character,
                              jolt_physics_world *w,
                              const physics::character_creation_config &cfg);
    virtual ~jolt_character_controller();

    void initialize_with_world(jolt_physics_world *w);
    void start() override final;

    void set_position(const vec3<real> &pos) override final;
    vec3<real> get_position() const override final;

    void set_rotation(const quat<real> &rot) override final;
    quat<real> get_rotation() const override final;

    void set_linear_velocity(const vec3<real> &vel) override final;
    vec3<real> get_linear_velocity() const override final;

    void activate() override final;
    void deactivate() override final;
    bool is_active() const override final;

    bool is_on_ground() const override final;

    physics::character_creation_config get_config() const override final;
    void
    set_config(const physics::character_creation_config &config) override final;

    void on_draw_gizmos(gpu &g, bool selected) override;

    void pre_physics_update(float dt);
};

} // namespace zabato::physics::jolt