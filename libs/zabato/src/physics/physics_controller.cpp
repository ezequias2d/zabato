#include <zabato/physics/physics_controller.hpp>
#include <zabato/physics/types.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>

namespace zabato::physics
{
const rtti physics_controller::TYPE("zabato.physics.physics_controller",
                                    &controller::TYPE,
                                    physics_controller::reflect);

static void
w_is_active(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 1)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(physics_controller::TYPE))
        return;
    auto *ctrl = static_cast<physics_controller *>(self.get());
    args->push_return(value(ctrl->is_active()));
}

static void
w_activate(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 1)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(physics_controller::TYPE))
        return;
    auto *ctrl = static_cast<physics_controller *>(self.get());
    ctrl->activate();
}

static void
w_deactivate(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 1)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(physics_controller::TYPE))
        return;
    auto *ctrl = static_cast<physics_controller *>(self.get());
    ctrl->deactivate();
}

static void
w_get_position(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 1)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(physics_controller::TYPE))
        return;
    auto *ctrl     = static_cast<physics_controller *>(self.get());
    vec3<real> pos = ctrl->get_position();
    args->push_return(value(pos));
}

static void
w_set_position(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 2)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(physics_controller::TYPE))
        return;
    auto *ctrl = static_cast<physics_controller *>(self.get());
    ctrl->set_position(args->get_value(1).as_vec3());
}

static void
w_get_rotation(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 1)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(physics_controller::TYPE))
        return;
    auto *ctrl     = static_cast<physics_controller *>(self.get());
    quat<real> rot = ctrl->get_rotation();
    args->push_return(value(rot));
}

static void
w_set_rotation(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 2)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(physics_controller::TYPE))
        return;
    auto *ctrl = static_cast<physics_controller *>(self.get());
    ctrl->set_rotation(args->get_value(1).as_quat());
}

static void w_get_linear_velocity(script_system *sys,
                                  script_instance *inst,
                                  script_args *args)
{
    if (args->count() < 1)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(physics_controller::TYPE))
        return;

    auto *ctrl   = static_cast<physics_controller *>(self.get());
    vec3<real> v = ctrl->get_linear_velocity();
    args->push_return(value(v));
}

static void w_set_linear_velocity(script_system *sys,
                                  script_instance *inst,
                                  script_args *args)
{
    if (args->count() < 2)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(physics_controller::TYPE))
        return;

    auto *ctrl = static_cast<physics_controller *>(self.get());
    ctrl->set_linear_velocity(args->get_value(1).as_vec3());
}

void physics_controller::reflect(reflection &r)
{
    controller::reflect(r);

    r.add_method("activate", w_activate);
    r.add_method("deactivate", w_deactivate);
    r.add_property("is_active", w_is_active);

    r.add_property("position", w_get_position, w_set_position);
    r.add_property("rotation", w_get_rotation, w_set_rotation);

    r.add_property(
        "linear_velocity", w_get_linear_velocity, w_set_linear_velocity);
}

} // namespace zabato::physics
