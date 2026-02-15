#include <zabato/base_object.hpp>
#include <zabato/gizmos.hpp>
#include <zabato/object.hpp>
#include <zabato/physics/character_controller.hpp>
#include <zabato/physics/types.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/serializer.hpp>
#include <zabato/spatial.hpp>
#include <zabato/world.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato::physics
{
#pragma region character_creation_config
const rtti character_creation_config::TYPE =
    rtti("zabato.physics.character_creation_config",
         &base_object::TYPE,
         &character_creation_config::reflect);

static void character_creation_config_get_shape(script_system *,
                                                script_instance *,
                                                script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(static_cast<pointer<base_object>>(s->shape)));
}

static void character_creation_config_set_shape(script_system *sys,
                                                script_instance *inst,
                                                script_args *args)
{
    if (args->count() < 1)
        return;

    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;

    auto value          = args->get_value(1);
    shape_config *shape = nullptr;
    if (value.is_string())
        shape = create_shape_config(value.as_string());
    else
    {
        shape = c_dynamic_cast<shape_config>(value.as_object().get());
    }

    if (shape == nullptr)
    {
        report(report_type::error,
               "Invalid shape pass to character_creation_config_set_shape");
        return;
    }

    s->shape = shape;
}

static void character_creation_config_get_inner_body_shape(script_system *,
                                                           script_instance *,
                                                           script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(
        value(static_cast<pointer<base_object>>(s->inner_body_shape)));
}

static void
character_creation_config_set_inner_body_shape(script_system *sys,
                                               script_instance *inst,
                                               script_args *args)
{
    if (args->count() < 1)
        return;

    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;

    auto value          = args->get_value(1);
    shape_config *shape = nullptr;
    if (value.is_string())
        shape = create_shape_config(value.as_string());
    else
    {
        shape = c_dynamic_cast<shape_config>(value.as_object().get());
    }

    if (shape == nullptr)
    {
        report(report_type::error,
               "Invalid shape pass to character_creation_config_set_inner_body_"
               "shape");
        return;
    }

    s->inner_body_shape = shape;
}

static void character_creation_config_get_mass(script_system *,
                                               script_instance *,
                                               script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->mass));
}

static void character_creation_config_set_mass(script_system *,
                                               script_instance *,
                                               script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->mass = args->get_value(1).as_number();
}

static void character_creation_config_get_max_slope_angle(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->max_slope_angle));
}

static void character_creation_config_set_max_slope_angle(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->max_slope_angle = args->get_value(1).as_number();
}

static void character_creation_config_get_gravity_factor(script_system *,
                                                         script_instance *,
                                                         script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->gravity_factor));
}

static void character_creation_config_set_gravity_factor(script_system *,
                                                         script_instance *,
                                                         script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->gravity_factor = args->get_value(1).as_number();
}

static void character_creation_config_get_max_strength(script_system *,
                                                       script_instance *,
                                                       script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->max_strength));
}

static void character_creation_config_set_max_strength(script_system *,
                                                       script_instance *,
                                                       script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->max_strength = args->get_value(1).as_number();
}

static void character_creation_config_get_character_padding(script_system *,
                                                            script_instance *,
                                                            script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->character_padding));
}

static void character_creation_config_set_character_padding(script_system *,
                                                            script_instance *,
                                                            script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->character_padding = args->get_value(1).as_number();
}

static void
character_creation_config_get_penetration_recovery_speed(script_system *,
                                                         script_instance *,
                                                         script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->penetration_recovery_speed));
}

static void
character_creation_config_set_penetration_recovery_speed(script_system *,
                                                         script_instance *,
                                                         script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->penetration_recovery_speed = args->get_value(1).as_number();
}

static void
character_creation_config_get_predictive_contact_distance(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->predictive_contact_distance));
}

static void
character_creation_config_set_predictive_contact_distance(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->predictive_contact_distance = args->get_value(1).as_number();
}

static void character_creation_config_get_shape_offset(script_system *,
                                                       script_instance *,
                                                       script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->shape_offset));
}

static void character_creation_config_set_shape_offset(script_system *,
                                                       script_instance *,
                                                       script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->shape_offset = args->get_value(1).as_vec3();
}

static void character_creation_config_get_up(script_system *,
                                             script_instance *,
                                             script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->up));
}

static void character_creation_config_set_up(script_system *,
                                             script_instance *,
                                             script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->up = args->get_value(1).as_vec3();
}

static void
character_creation_config_get_enhanced_internal_edge_removal(script_system *,
                                                             script_instance *,
                                                             script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->enhanced_internal_edge_removal));
}

static void
character_creation_config_set_enhanced_internal_edge_removal(script_system *,
                                                             script_instance *,
                                                             script_args *args)
{
    if (args->count() < 1)
        return;
    character_creation_config *s = c_dynamic_cast<character_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->enhanced_internal_edge_removal = args->get_value(1).as_bool();
}

void character_creation_config::reflect(reflection &r)
{
    base_object::reflect(r);
    auto shape_enum = value::make_list();
    for (auto it = &ALL_SHAPE_CONFIG_TYPES[0]; it->_0 != nullptr; it++)
    {
        shape_enum.push(it->_0->name());
    }

    value shape_attrs = value::make_map();
    shape_attrs.set_field("types", shape_enum);

    r.add_property("shape",
                   character_creation_config_get_shape,
                   character_creation_config_set_shape,
                   shape_attrs);

    r.add_property("inner_body_shape",
                   character_creation_config_get_inner_body_shape,
                   character_creation_config_set_inner_body_shape,
                   shape_attrs);

    r.add_property("mass",
                   character_creation_config_get_mass,
                   character_creation_config_set_mass);

    r.add_property("max_slope_angle",
                   character_creation_config_get_max_slope_angle,
                   character_creation_config_set_max_slope_angle);

    r.add_property("gravity_factor",
                   character_creation_config_get_gravity_factor,
                   character_creation_config_set_gravity_factor);

    r.add_property("max_strength",
                   character_creation_config_get_max_strength,
                   character_creation_config_set_max_strength);

    r.add_property("character_padding",
                   character_creation_config_get_character_padding,
                   character_creation_config_set_character_padding);

    r.add_property("penetration_recovery_speed",
                   character_creation_config_get_penetration_recovery_speed,
                   character_creation_config_set_penetration_recovery_speed);

    r.add_property("predictive_contact_distance",
                   character_creation_config_get_predictive_contact_distance,
                   character_creation_config_set_predictive_contact_distance);

    r.add_property("shape_offset",
                   character_creation_config_get_shape_offset,
                   character_creation_config_set_shape_offset);

    r.add_property("up",
                   character_creation_config_get_up,
                   character_creation_config_set_up);

    r.add_property(
        "enhanced_internal_edge_removal",
        character_creation_config_get_enhanced_internal_edge_removal,
        character_creation_config_set_enhanced_internal_edge_removal);
}

void character_creation_config::save(serializer &stream) const
{
    if (shape)
    {
        stream.write(string(shape->type().name()));
        shape->save(stream);
    }
    else
    {
        stream.write(string(""));
    }

    if (inner_body_shape)
    {
        stream.write(string(inner_body_shape->type().name()));
        inner_body_shape->save(stream);
    }
    else
    {
        stream.write(string(""));
    }

    stream.write(mass);
    stream.write(max_slope_angle);
    stream.write(gravity_factor);
    stream.write(max_strength);
    stream.write(character_padding);
    stream.write(penetration_recovery_speed);
    stream.write(predictive_contact_distance);
    stream.write(shape_offset);
    stream.write(up);
    stream.write(enhanced_internal_edge_removal);
    stream.write(supporting_volume.normal);
    stream.write(supporting_volume.d);
    stream.write((int32_t)back_face);
    stream.write(max_collision_iterations);
    stream.write(max_constraint_iterations);
    stream.write(min_time_remaining);
    stream.write(collision_tolerance);
    stream.write(max_num_hits);
    stream.write(hit_reduction_cos_max_angle);
}

void character_creation_config::load(serializer &stream, serializer_link *link)
{
    string type_name;
    stream.read(type_name);

    if (!type_name.empty())
    {
        auto ptr = create_shape_config(type_name);
        if (ptr)
        {
            ptr->load(stream, link);
            shape = ptr;
        }
    }

    string inner_type_name;
    stream.read(inner_type_name);

    if (!inner_type_name.empty())
    {
        auto ptr = create_shape_config(inner_type_name);
        if (ptr)
        {
            ptr->load(stream, link);
            inner_body_shape = ptr;
        }
    }

    stream.read(mass);
    stream.read(max_slope_angle);
    stream.read(gravity_factor);
    stream.read(max_strength);
    stream.read(character_padding);
    stream.read(penetration_recovery_speed);
    stream.read(predictive_contact_distance);
    stream.read(shape_offset);
    stream.read(up);
    stream.read(enhanced_internal_edge_removal);
    stream.read(supporting_volume.normal);
    stream.read(supporting_volume.d);
    int32_t back_face_val;
    stream.read(back_face_val);
    back_face = (back_face_mode)back_face_val;
    stream.read(max_collision_iterations);
    stream.read(max_constraint_iterations);
    stream.read(min_time_remaining);
    stream.read(collision_tolerance);
    stream.read(max_num_hits);
    stream.read(hit_reduction_cos_max_angle);
}

void character_creation_config::save_xml(xml_serializer &stream,
                                         tinyxml2::XMLElement &element) const
{
    if (shape)
    {
        auto *shape_el = element.GetDocument()->NewElement("shape");
        shape_el->SetAttribute("type_name", shape->type().name());
        shape->save_xml(stream, *shape_el);
        element.InsertEndChild(shape_el);
    }

    if (inner_body_shape)
    {
        auto *inner_shape_el =
            element.GetDocument()->NewElement("inner_body_shape");
        inner_shape_el->SetAttribute("type_name",
                                     inner_body_shape->type().name());
        inner_body_shape->save_xml(stream, *inner_shape_el);
        element.InsertEndChild(inner_shape_el);
    }

    element.SetAttribute("mass", (double)mass);
    element.SetAttribute("max_slope_angle", (double)max_slope_angle);
    element.SetAttribute("gravity_factor", (double)gravity_factor);
    element.SetAttribute("max_strength", (double)max_strength);
    element.SetAttribute("character_padding", (double)character_padding);
    element.SetAttribute("penetration_recovery_speed",
                         (double)penetration_recovery_speed);
    element.SetAttribute("predictive_contact_distance",
                         (double)predictive_contact_distance);
    xml_serializer::write_vec3(*element.InsertNewChildElement("shape_offset"),
                               shape_offset);
    xml_serializer::write_vec3(*element.InsertNewChildElement("up"), up);
    element.SetAttribute("enhanced_internal_edge_removal",
                         enhanced_internal_edge_removal);

    auto *sv_el = element.InsertNewChildElement("supporting_volume");
    xml_serializer::write_vec3(*sv_el, supporting_volume.normal);
    sv_el->SetAttribute("d", (double)supporting_volume.d);

    element.SetAttribute("back_face", (int)back_face);
    element.SetAttribute("max_collision_iterations",
                         (int)max_collision_iterations);
    element.SetAttribute("max_constraint_iterations",
                         (int)max_constraint_iterations);
    element.SetAttribute("min_time_remaining", (double)min_time_remaining);
    element.SetAttribute("collision_tolerance", (double)collision_tolerance);
    element.SetAttribute("max_num_hits", (int)max_num_hits);
    element.SetAttribute("hit_reduction_cos_max_angle",
                         (double)hit_reduction_cos_max_angle);
}

void character_creation_config::load_xml(xml_serializer &stream,
                                         tinyxml2::XMLElement &element)
{
    auto *shape_el = element.FirstChildElement("shape");
    if (shape_el)
    {
        string type_name = shape_el->Attribute("type_name");
        auto ptr         = create_shape_config(type_name);
        if (ptr)
        {
            ptr->load_xml(stream, *shape_el);
            shape = ptr;
        }
    }

    auto *inner_shape_el = element.FirstChildElement("inner_body_shape");
    if (inner_shape_el)
    {
        string type_name = inner_shape_el->Attribute("type_name");
        auto ptr         = create_shape_config(type_name);
        if (ptr)
        {
            ptr->load_xml(stream, *inner_shape_el);
            inner_body_shape = ptr;
        }
    }

    mass              = element.DoubleAttribute("mass");
    max_slope_angle   = element.DoubleAttribute("max_slope_angle");
    gravity_factor    = element.DoubleAttribute("gravity_factor", 1.0);
    max_strength      = element.DoubleAttribute("max_strength", 100.0);
    character_padding = element.DoubleAttribute("character_padding", 0.02);
    penetration_recovery_speed =
        element.DoubleAttribute("penetration_recovery_speed", 1.0);
    predictive_contact_distance =
        element.DoubleAttribute("predictive_contact_distance", 0.1);

    auto *shape_offset_el = element.FirstChildElement("shape_offset");
    if (shape_offset_el)
        shape_offset = xml_serializer::read_vec3(*shape_offset_el);

    auto *up_el = element.FirstChildElement("up");
    if (up_el)
        up = xml_serializer::read_vec3(*up_el);

    enhanced_internal_edge_removal =
        element.BoolAttribute("enhanced_internal_edge_removal");

    auto *sv_el = element.FirstChildElement("supporting_volume");
    if (sv_el)
    {
        supporting_volume.normal = xml_serializer::read_vec3(*sv_el);
        supporting_volume.d      = sv_el->DoubleAttribute("d");
    }

    back_face = (back_face_mode)element.IntAttribute("back_face");
    max_collision_iterations =
        element.IntAttribute("max_collision_iterations", 5);
    max_constraint_iterations =
        element.IntAttribute("max_constraint_iterations", 15);
    min_time_remaining = element.DoubleAttribute("min_time_remaining", 1.0e-4);
    collision_tolerance =
        element.DoubleAttribute("collision_tolerance", 1.0e-3);
    max_num_hits = element.IntAttribute("max_num_hits", 256);
    hit_reduction_cos_max_angle =
        element.DoubleAttribute("hit_reduction_cos_max_angle", 0.999);
}

#pragma endregion character_creation_config

const rtti character_controller::TYPE("zabato.character_controller",
                                      &physics_controller::TYPE,
                                      character_controller::reflect);

static void
w_is_on_ground(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 1)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(character_controller::TYPE))
        return;

    auto *ctrl = static_cast<character_controller *>(self.get());
    bool res   = ctrl->is_on_ground();
    args->push_return(value(res));
}

static void w_get_config(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    auto self = c_dynamic_cast<character_controller>(
        args->get_value(0).as_object().get());
    if (!self)
        return;

    auto *c = new character_creation_config();
    *c      = self->get_config();
    args->push_return(value(c));
}

static void w_set_config(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    auto self = c_dynamic_cast<character_controller>(
        args->get_value(0).as_object().get());
    if (!self)
        return;

    auto v = c_dynamic_cast<character_creation_config>(
        args->get_value(1).as_object().get());
    if (!v)
    {
        args->error("Invalid value. The value provided is not a "
                    "character_creation_config.");
        return;
    }

    self->set_config(*v);
}

void character_controller::reflect(reflection &r)
{
    physics_controller::reflect(r);

    r.add_property("config", w_get_config, w_set_config);
    r.add_property("is_on_ground", w_is_on_ground);
}

character_controller::character_controller() {}
character_controller::~character_controller() {}

void character_controller::initialize(const controller::context &ctx)
{
    controller::initialize(ctx);

    if (!m_object)
    {
        m_console->log_warning("character_controller::initialize: No object");
        return;
    }

    // Set initial position from spatial
    spatial *s       = static_cast<spatial *>(m_object);
    transformation t = s->get_world_transform();
    set_position(t.translate());
    set_rotation(t.rotate());

    start_sync();
}

void character_controller::start() { start_sync(); }

void character_controller::update(real dt) { update_sync(); }

void character_controller::save(serializer &stream) const
{
    controller::save(stream);
    auto config = get_config();
    config.save(stream);
}

void character_controller::load(serializer &stream, serializer_link *link)
{
    controller::load(stream, link);
    character_creation_config config;
    config.load(stream, link);
    set_config(config);
}

void character_controller::save_xml(xml_serializer &stream,
                                    tinyxml2::XMLElement &element) const
{
    controller::save_xml(stream, element);
    auto config = get_config();
    config.save_xml(stream, element);
}

void character_controller::load_xml(xml_serializer &stream,
                                    tinyxml2::XMLElement &element)
{
    controller::load_xml(stream, element);
    character_creation_config config;
    config.load_xml(stream, element);
    set_config(config);
}

} // namespace zabato::physics
