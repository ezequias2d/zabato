#pragma once

#include <zabato/base_object.hpp>
#include <zabato/math.hpp>
#include <zabato/object.hpp>
#include <zabato/physics/character_controller.hpp>
#include <zabato/physics/physics_controller.hpp>
#include <zabato/physics/rigid_body_controller.hpp>
#include <zabato/physics/types.hpp>
// #include <zabato/physics/vehicle_controller.hpp>

namespace zabato::physics
{

class physics_world : public base_object
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    virtual ~physics_world() = default;

    virtual void update(real dt) = 0;

    virtual void set_gravity(const vec3<real> &g) = 0;
    virtual vec3<real> get_gravity() const        = 0;

    virtual void add_controller(physics_controller *controller)    = 0;
    virtual void remove_controller(physics_controller *controller) = 0;

    virtual bool raycast(const ray3<real> &ray,
                         physics_raycast_result &out) = 0;

    virtual pointer<character_controller>
    create_character(const character_creation_config &settings) = 0;

    // virtual pointer<vehicle_controller>
    // create_vehicle(const vehicle_creation_config &settings) = 0;

    virtual pointer<rigid_body_controller>
    create_rigid_body(const rigid_body_creation_config &settings) = 0;
};

} // namespace zabato::physics