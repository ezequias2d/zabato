
#include <tinyxml2.h>
#include <zabato/controller.hpp>
#include <zabato/gizmos.hpp>
#include <zabato/mesh.hpp>
#include <zabato/physics/rigid_body_controller.hpp>
#include <zabato/physics/types.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/serializer.hpp>
#include <zabato/spatial.hpp>
#include <zabato/world.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato::physics
{

const rtti rigid_body_controller::TYPE("zabato.rigid_body_controller",
                                       &physics_controller::TYPE,
                                       rigid_body_controller::reflect);

static void w_get_config(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    auto self = c_dynamic_cast<rigid_body_controller>(
        args->get_value(0).as_object().get());
    if (!self)
        return;

    auto *c = new rigid_body_creation_config();
    *c      = self->get_config();
    args->push_return(value(c));
}

static void w_set_config(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    auto self = c_dynamic_cast<rigid_body_controller>(
        args->get_value(0).as_object().get());
    if (!self)
        return;

    auto v = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(1).as_object().get());
    if (!v)
    {
        args->error("Invalid value. The value provided is not a "
                    "rigid_body_creation_config.");
        return;
    }

    self->set_config(*v);
}

static void w_get_angular_velocity(script_system *sys,
                                   script_instance *inst,
                                   script_args *args)
{
    if (args->count() < 1)
        return;
    auto self = c_dynamic_cast<rigid_body_controller>(
        args->get_value(0).as_object().get());
    if (!self)
        return;

    vec3<real> v = self->get_angular_velocity();
    args->push_return(value(v));
}

static void w_set_angular_velocity(script_system *sys,
                                   script_instance *inst,
                                   script_args *args)
{
    if (args->count() < 2)
        return;
    auto self = c_dynamic_cast<rigid_body_controller>(
        args->get_value(0).as_object().get());
    if (!self)
        return;

    self->set_angular_velocity(args->get_value(1).as_vec3());
}

static void
add_force_method(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    auto self = c_dynamic_cast<rigid_body_controller>(
        args->get_value(0).as_object().get());
    if (!self)
        return;

    vec3<real> force = args->get_value(1).as_vec3();
    if (args->count() >= 3)
        self->add_force(force, args->get_value(2).as_vec3());
    else
        self->add_force(force);
}

static void
add_torque_method(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    auto self = c_dynamic_cast<rigid_body_controller>(
        args->get_value(0).as_object().get());
    if (!self)
        return;

    self->add_torque(args->get_value(1).as_vec3());
}

static void
add_impulse_method(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    auto self = c_dynamic_cast<rigid_body_controller>(
        args->get_value(0).as_object().get());
    if (!self)
        return;

    vec3<real> impulse = args->get_value(1).as_vec3();
    if (args->count() >= 3)
        self->add_impulse(impulse, args->get_value(2).as_vec3());
    else
        self->add_impulse(impulse);
}

static void add_angular_impulse_method(script_system *,
                                       script_instance *,
                                       script_args *args)
{
    if (args->count() < 2)
        return;
    auto self = c_dynamic_cast<rigid_body_controller>(
        args->get_value(0).as_object().get());
    if (!self)
        return;

    self->add_angular_impulse(args->get_value(1).as_vec3());
}

void rigid_body_controller::reflect(reflection &r)
{
    physics_controller::reflect(r);

    r.add_property("config", w_get_config, w_set_config);

    r.add_property(
        "angular_velocity", w_get_angular_velocity, w_set_angular_velocity);

    r.add_method("add_force", add_force_method);
    r.add_method("add_torque", add_torque_method);
    r.add_method("add_impulse", add_impulse_method);
    r.add_method("add_angular_impulse", add_angular_impulse_method);
}

rigid_body_controller::rigid_body_controller() {}

rigid_body_controller::~rigid_body_controller() {}

void rigid_body_controller::initialize(const controller::context &ctx)
{
    controller::initialize(ctx);

    if (!m_object)
    {
        m_console->log_warning("rigid_body_controller::initialize: No object");
        return;
    }

    // Set initial position from spatial
    spatial *s       = static_cast<spatial *>(m_object);
    transformation t = s->get_world_transform();
    set_position(t.translate());
    set_rotation(t.rotate());

    start_sync();
}

void rigid_body_controller::start() { start_sync(); }

void rigid_body_controller::update(real dt) { update_sync(); }

void rigid_body_controller::save(serializer &stream) const
{
    controller::save(stream);
    auto config = get_config();
    config.save(stream);
}

void rigid_body_controller::load(serializer &stream, serializer_link *link)
{
    controller::load(stream, link);
    rigid_body_creation_config config;
    config.load(stream, link);
    set_config(config);
}

void rigid_body_controller::save_xml(xml_serializer &stream,
                                     tinyxml2::XMLElement &element) const
{
    controller::save_xml(stream, element);
    auto config = get_config();
    config.save_xml(stream, element);
}

void rigid_body_controller::load_xml(xml_serializer &stream,
                                     tinyxml2::XMLElement &element)
{
    controller::load_xml(stream, element);
    rigid_body_creation_config config;
    config.load_xml(stream, element);
    set_config(config);
}

} // namespace zabato::physics
