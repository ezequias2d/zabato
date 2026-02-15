#pragma once

#include <zabato/controller.hpp>
#include <zabato/event.hpp>
#include <zabato/physics/physics_controller.hpp>
#include <zabato/physics/types.hpp>
#include <zabato/resource.hpp>
#include <zabato/string.hpp>
#include <zabato/transformation.hpp>
#include <zabato/unique_ptr.hpp>

namespace zabato::physics
{

class rigid_body_controller : public physics_controller
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    virtual void set_angular_velocity(const vec3<real> &velocity) = 0;
    virtual vec3<real> get_angular_velocity() const               = 0;

    virtual void add_force(const vec3<real> &force)             = 0;
    virtual void add_force(const vec3<real> &force,
                           const vec3<real> &point)             = 0;
    virtual void add_torque(const vec3<real> &torque)           = 0;
    virtual void add_impulse(const vec3<real> &impulse)         = 0;
    virtual void add_impulse(const vec3<real> &impulse,
                             const vec3<real> &point)           = 0;
    virtual void add_angular_impulse(const vec3<real> &impulse) = 0;

    rigid_body_controller();
    virtual ~rigid_body_controller();

    virtual void initialize(const controller::context &ctx) override;
    virtual void start() override;
    virtual void update(real dt) override;

    virtual void save(serializer &stream) const override;
    virtual void load(serializer &stream, serializer_link *link) override;
    virtual void save_xml(xml_serializer &stream,
                          tinyxml2::XMLElement &element) const override;
    virtual void load_xml(xml_serializer &stream,
                          tinyxml2::XMLElement &element) override;

    virtual void set_config(const rigid_body_creation_config &config) = 0;
    virtual rigid_body_creation_config get_config() const             = 0;
};
} // namespace zabato::physics
