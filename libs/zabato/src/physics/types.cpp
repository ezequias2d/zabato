#include <stdio.h>
#include <tinyxml2.h>

#include <zabato/base_object.hpp>
#include <zabato/error.hpp>
#include <zabato/object.hpp>
#include <zabato/physics/types.hpp>
#include <zabato/reflection.hpp>
#include <zabato/rtti.hpp>
#include <zabato/script.hpp>
#include <zabato/serializer.hpp>
#include <zabato/tuple.hpp>
#include <zabato/unique_ptr.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato::physics
{

const rtti shape_config::TYPE("zabato.physics.shape_config",
                              &base_object::TYPE,
                              shape_config::reflect);
const rtti convex_shape_config::TYPE("zabato.physics.convex_shape_config",
                                     &shape_config::TYPE,
                                     convex_shape_config::reflect);
const rtti box_shape_config::TYPE("zabato.physics.box_shape_config",
                                  &convex_shape_config::TYPE,
                                  box_shape_config::reflect);
const rtti capsule_shape_config::TYPE("zabato.physics.capsule_shape_config",
                                      &convex_shape_config::TYPE,
                                      capsule_shape_config::reflect);
const rtti
    convex_hull_shape_config::TYPE("zabato.physics.convex_hull_shape_config",
                                   &convex_shape_config::TYPE,
                                   convex_hull_shape_config::reflect);
const rtti cylinder_shape_config::TYPE("zabato.physics.cylinder_shape_config",
                                       &convex_shape_config::TYPE,
                                       cylinder_shape_config::reflect);
const rtti sphere_shape_config::TYPE("zabato.physics.sphere_shape_config",
                                     &convex_shape_config::TYPE,
                                     sphere_shape_config::reflect);
const rtti tapered_capsule_shape_config::TYPE(
    "zabato.physics.tapered_capsule_shape_config",
    &convex_shape_config::TYPE,
    tapered_capsule_shape_config::reflect);
const rtti tapered_cylinder_shape_config::TYPE(
    "zabato.physics.tapered_cylinder_shape_config",
    &convex_shape_config::TYPE,
    tapered_cylinder_shape_config::reflect);
const rtti triangle_shape_config::TYPE("zabato.physics.triangle_shape_config",
                                       &convex_shape_config::TYPE,
                                       triangle_shape_config::reflect);
const rtti empty_shape_config::TYPE("zabato.physics.empty_shape_config",
                                    &shape_config::TYPE,
                                    empty_shape_config::reflect);

shape_config *create_shape_config(string_view type_name)
{
    for (auto it = &ALL_SHAPE_CONFIG_TYPES[0]; it->_0 != nullptr; it++)
        if (it->_0->name() == type_name)
            return it->_1();
    return nullptr;
}

#pragma region shape_config
void shape_config::save(serializer &stream) const {}

void shape_config::load(serializer &stream, serializer_link *link) {}

void shape_config::save_xml(xml_serializer &stream,
                            tinyxml2::XMLElement &element) const
{
}

void shape_config::load_xml(xml_serializer &stream,
                            tinyxml2::XMLElement &element)
{
}

void shape_config::reflect(reflection &r) { base_object::reflect(r); }

#pragma endregion shape_config

#pragma region convex_shape_config
void convex_shape_config::save(serializer &stream) const
{
    shape_config::save(stream);
    stream.write(density);
}

void convex_shape_config::load(serializer &stream, serializer_link *link)
{
    shape_config::load(stream, link);
    stream.read(density);
}

void convex_shape_config::save_xml(xml_serializer &stream,
                                   tinyxml2::XMLElement &element) const
{
    shape_config::save_xml(stream, element);
    element.SetAttribute("density", (double)density);
}

void convex_shape_config::load_xml(xml_serializer &stream,
                                   tinyxml2::XMLElement &element)
{
    shape_config::load_xml(stream, element);
    density = element.DoubleAttribute("density");
}

static void convex_shape_config_get_density(script_system *,
                                            script_instance *,
                                            script_args *args)
{
    if (args->count() < 1)
        return;
    convex_shape_config *c = c_dynamic_cast<convex_shape_config>(
        args->get_value(0).as_object().get());
    if (!c)
        return;
    args->push_return(value(c->density));
}

static void convex_shape_config_set_density(script_system *,
                                            script_instance *,
                                            script_args *args)
{
    if (args->count() < 1)
        return;
    convex_shape_config *c = c_dynamic_cast<convex_shape_config>(
        args->get_value(0).as_object().get());
    if (!c)
        return;
    c->density = args->get_value(1).as_number();
}

void convex_shape_config::reflect(reflection &r)
{
    shape_config::reflect(r);
    r.add_property("density",
                   convex_shape_config_get_density,
                   convex_shape_config_set_density);
}
#pragma endregion convex_shape_config

#pragma region box_shape_config
void box_shape_config::save(serializer &stream) const
{
    convex_shape_config::save(stream);
    stream.write(half_extent);
    stream.write(convex_radius);
}

void box_shape_config::load(serializer &stream, serializer_link *link)
{
    convex_shape_config::load(stream, link);
    stream.read(half_extent);
    stream.read(convex_radius);
}

void box_shape_config::save_xml(xml_serializer &stream,
                                tinyxml2::XMLElement &element) const
{
    convex_shape_config::save_xml(stream, element);

    auto *child = element.GetDocument()->NewElement("half_extent");
    xml_serializer::write_vec3(*child, half_extent);
    element.InsertEndChild(child);

    element.SetAttribute("convex_radius", (double)convex_radius);
}

void box_shape_config::load_xml(xml_serializer &stream,
                                tinyxml2::XMLElement &element)
{
    convex_shape_config::load_xml(stream, element);

    auto *child = element.FirstChildElement("half_extent");
    if (child)
        half_extent = xml_serializer::read_vec3(*child);

    convex_radius = element.DoubleAttribute("convex_radius");
}

static void box_shape_config_get_half_extent(script_system *,
                                             script_instance *,
                                             script_args *args)
{
    if (args->count() < 1)
        return;
    box_shape_config *b =
        c_dynamic_cast<box_shape_config>(args->get_value(0).as_object().get());
    if (!b)
        return;
    args->push_return(value(b->half_extent));
}

static void box_shape_config_set_half_extent(script_system *,
                                             script_instance *,
                                             script_args *args)
{
    if (args->count() < 1)
        return;
    box_shape_config *b =
        c_dynamic_cast<box_shape_config>(args->get_value(0).as_object().get());
    if (!b)
        return;
    b->half_extent = args->get_value(1).as_vec3();
}

static void box_shape_config_get_convex_radius(script_system *,
                                               script_instance *,
                                               script_args *args)
{
    if (args->count() < 1)
        return;
    box_shape_config *b =
        c_dynamic_cast<box_shape_config>(args->get_value(0).as_object().get());
    if (!b)
        return;
    args->push_return(value(b->convex_radius));
}

static void box_shape_config_set_convex_radius(script_system *,
                                               script_instance *,
                                               script_args *args)
{
    if (args->count() < 1)
        return;
    box_shape_config *b =
        c_dynamic_cast<box_shape_config>(args->get_value(0).as_object().get());
    if (!b)
        return;
    b->convex_radius = args->get_value(1).as_number();
}

void box_shape_config::reflect(reflection &r)
{
    convex_shape_config::reflect(r);
    r.add_property("half_extent",
                   box_shape_config_get_half_extent,
                   box_shape_config_set_half_extent);
    r.add_property("convex_radius",
                   box_shape_config_get_convex_radius,
                   box_shape_config_set_convex_radius);
}

#pragma endregion box_shape_config

#pragma region capsule_shape_config
void capsule_shape_config::save(serializer &stream) const
{
    convex_shape_config::save(stream);
    stream.write(radius);
    stream.write(half_height);
}

void capsule_shape_config::load(serializer &stream, serializer_link *link)
{
    convex_shape_config::load(stream, link);
    stream.read(radius);
    stream.read(half_height);
}

void capsule_shape_config::save_xml(xml_serializer &stream,
                                    tinyxml2::XMLElement &element) const
{
    convex_shape_config::save_xml(stream, element);
    element.SetAttribute("radius", (double)radius);
    element.SetAttribute("half_height", (double)half_height);
}

void capsule_shape_config::load_xml(xml_serializer &stream,
                                    tinyxml2::XMLElement &element)
{
    convex_shape_config::load_xml(stream, element);
    radius      = element.DoubleAttribute("radius");
    half_height = element.DoubleAttribute("half_height");
}

static void capsule_shape_config_get_half_height(script_system *,
                                                 script_instance *,
                                                 script_args *args)
{
    if (args->count() < 1)
        return;
    capsule_shape_config *c = c_dynamic_cast<capsule_shape_config>(
        args->get_value(0).as_object().get());
    if (!c)
        return;
    args->push_return(value(c->half_height));
}

static void capsule_shape_config_set_half_height(script_system *,
                                                 script_instance *,
                                                 script_args *args)
{
    if (args->count() < 1)
        return;
    capsule_shape_config *c = c_dynamic_cast<capsule_shape_config>(
        args->get_value(0).as_object().get());
    if (!c)
        return;
    c->half_height = args->get_value(1).as_number();
}

static void capsule_shape_config_get_radius(script_system *,
                                            script_instance *,
                                            script_args *args)
{
    if (args->count() < 1)
        return;
    capsule_shape_config *c = c_dynamic_cast<capsule_shape_config>(
        args->get_value(0).as_object().get());
    if (!c)
        return;
    args->push_return(value(c->radius));
}

static void capsule_shape_config_set_radius(script_system *,
                                            script_instance *,
                                            script_args *args)
{
    if (args->count() < 1)
        return;
    capsule_shape_config *c = c_dynamic_cast<capsule_shape_config>(
        args->get_value(0).as_object().get());
    if (!c)
        return;
    c->radius = args->get_value(1).as_number();
}

void capsule_shape_config::reflect(reflection &r)
{
    convex_shape_config::reflect(r);
    r.add_property("half_height",
                   capsule_shape_config_get_half_height,
                   capsule_shape_config_set_half_height);
    r.add_property("radius",
                   capsule_shape_config_get_radius,
                   capsule_shape_config_set_radius);
}
#pragma endregion capsule_shape_config

#pragma region convex_hull_shape_config
void convex_hull_shape_config::save(serializer &stream) const
{
    convex_shape_config::save(stream);
    stream.write(mesh);
    stream.write(max_convex_radius);
    stream.write(max_error_convex_radius);
    stream.write(hull_tolerance);
}

void convex_hull_shape_config::load(serializer &stream, serializer_link *link)
{
    convex_shape_config::load(stream, link);
    uint32_t size;
    stream.read(size);
    stream.read(mesh);
    stream.read(max_convex_radius);
    stream.read(max_error_convex_radius);
    stream.read(hull_tolerance);
}

void convex_hull_shape_config::save_xml(xml_serializer &stream,
                                        tinyxml2::XMLElement &element) const
{
    convex_shape_config::save_xml(stream, element);

    auto *mesh_el = element.InsertNewChildElement("mesh");
    xml_serializer::write_resource_ref(*mesh_el, mesh);
    element.SetAttribute("max_convex_radius", (double)max_convex_radius);
    element.SetAttribute("max_error_convex_radius",
                         (double)max_error_convex_radius);
    element.SetAttribute("hull_tolerance", (double)hull_tolerance);
}

void convex_hull_shape_config::load_xml(xml_serializer &stream,
                                        tinyxml2::XMLElement &element)
{
    convex_shape_config::load_xml(stream, element);

    auto *mesh_el = element.FirstChildElement("mesh");
    if (mesh_el)
    {
        xml_serializer::read_resource_ref(*mesh_el, mesh);
    }

    max_convex_radius = element.DoubleAttribute("max_convex_radius");
    max_error_convex_radius =
        element.DoubleAttribute("max_error_convex_radius");
    hull_tolerance = element.DoubleAttribute("hull_tolerance");
}

static void convex_hull_shape_config_get_mesh(script_system *,
                                              script_instance *,
                                              script_args *args)
{
    if (args->count() < 1)
        return;
    convex_hull_shape_config *h = c_dynamic_cast<convex_hull_shape_config>(
        args->get_value(0).as_object().get());
    if (!h)
        return;
    args->push_return(value(h->mesh.path()));
}

static void convex_hull_shape_config_set_mesh(script_system *,
                                              script_instance *,
                                              script_args *args)
{
    if (args->count() < 1)
        return;
    convex_hull_shape_config *h = c_dynamic_cast<convex_hull_shape_config>(
        args->get_value(0).as_object().get());
    if (!h)
        return;
    h->mesh = args->get_value(1).as_string();
}

static void convex_hull_shape_config_get_max_convex_radius(script_system *,
                                                           script_instance *,
                                                           script_args *args)
{
    if (args->count() < 1)
        return;
    convex_hull_shape_config *h = c_dynamic_cast<convex_hull_shape_config>(
        args->get_value(0).as_object().get());
    if (!h)
        return;
    args->push_return(value(h->max_convex_radius));
}

static void convex_hull_shape_config_set_max_convex_radius(script_system *,
                                                           script_instance *,
                                                           script_args *args)
{
    if (args->count() < 1)
        return;
    convex_hull_shape_config *h = c_dynamic_cast<convex_hull_shape_config>(
        args->get_value(0).as_object().get());
    if (!h)
        return;
    h->max_convex_radius = args->get_value(1).as_number();
}

static void
convex_hull_shape_config_get_max_error_convex_radius(script_system *,
                                                     script_instance *,
                                                     script_args *args)
{
    if (args->count() < 1)
        return;
    convex_hull_shape_config *h = c_dynamic_cast<convex_hull_shape_config>(
        args->get_value(0).as_object().get());
    if (!h)
        return;
    args->push_return(value(h->max_error_convex_radius));
}

static void
convex_hull_shape_config_set_max_error_convex_radius(script_system *,
                                                     script_instance *,
                                                     script_args *args)
{
    if (args->count() < 1)
        return;
    convex_hull_shape_config *h = c_dynamic_cast<convex_hull_shape_config>(
        args->get_value(0).as_object().get());
    if (!h)
        return;
    h->max_error_convex_radius = args->get_value(1).as_number();
}

static void convex_hull_shape_config_get_hull_tolerance(script_system *,
                                                        script_instance *,
                                                        script_args *args)
{
    if (args->count() < 1)
        return;
    convex_hull_shape_config *h = c_dynamic_cast<convex_hull_shape_config>(
        args->get_value(0).as_object().get());
    if (!h)
        return;
    args->push_return(value(h->hull_tolerance));
}

static void convex_hull_shape_config_set_hull_tolerance(script_system *,
                                                        script_instance *,
                                                        script_args *args)
{
    if (args->count() < 1)
        return;
    convex_hull_shape_config *h = c_dynamic_cast<convex_hull_shape_config>(
        args->get_value(0).as_object().get());
    if (!h)
        return;
    h->hull_tolerance = args->get_value(1).as_number();
}

void convex_hull_shape_config::reflect(reflection &r)
{
    convex_shape_config::reflect(r);
    r.add_property("mesh",
                   convex_hull_shape_config_get_mesh,
                   convex_hull_shape_config_set_mesh);
    r.add_property("max_convex_radius",
                   convex_hull_shape_config_get_max_convex_radius,
                   convex_hull_shape_config_set_max_convex_radius);
    r.add_property("max_error_convex_radius",
                   convex_hull_shape_config_get_max_error_convex_radius,
                   convex_hull_shape_config_set_max_error_convex_radius);
    r.add_property("hull_tolerance",
                   convex_hull_shape_config_get_hull_tolerance,
                   convex_hull_shape_config_set_hull_tolerance);
}
#pragma endregion convex_hull_shape_config

#pragma region cylinder_shape_config
void cylinder_shape_config::save(serializer &stream) const
{
    convex_shape_config::save(stream);
    stream.write(radius);
    stream.write(half_height);
    stream.write(convex_radius);
}

void cylinder_shape_config::load(serializer &stream, serializer_link *link)
{
    convex_shape_config::load(stream, link);
    stream.read(radius);
    stream.read(half_height);
    stream.read(convex_radius);
}

void cylinder_shape_config::save_xml(xml_serializer &stream,
                                     tinyxml2::XMLElement &element) const
{
    convex_shape_config::save_xml(stream, element);
    element.SetAttribute("radius", (double)radius);
    element.SetAttribute("half_height", (double)half_height);
    element.SetAttribute("convex_radius", (double)convex_radius);
}

void cylinder_shape_config::load_xml(xml_serializer &stream,
                                     tinyxml2::XMLElement &element)
{
    convex_shape_config::load_xml(stream, element);
    radius        = element.DoubleAttribute("radius");
    half_height   = element.DoubleAttribute("half_height");
    convex_radius = element.DoubleAttribute("convex_radius");
}

static void cylinder_shape_config_get_radius(script_system *,
                                             script_instance *,
                                             script_args *args)
{
    if (args->count() < 1)
        return;
    cylinder_shape_config *c = c_dynamic_cast<cylinder_shape_config>(
        args->get_value(0).as_object().get());
    if (!c)
        return;
    args->push_return(value(c->radius));
}

static void cylinder_shape_config_set_radius(script_system *,
                                             script_instance *,
                                             script_args *args)
{
    if (args->count() < 1)
        return;
    cylinder_shape_config *c = c_dynamic_cast<cylinder_shape_config>(
        args->get_value(0).as_object().get());
    if (!c)
        return;
    c->radius = args->get_value(1).as_number();
}

static void cylinder_shape_config_get_half_height(script_system *,
                                                  script_instance *,
                                                  script_args *args)
{
    if (args->count() < 1)
        return;
    cylinder_shape_config *c = c_dynamic_cast<cylinder_shape_config>(
        args->get_value(0).as_object().get());
    if (!c)
        return;
    args->push_return(value(c->half_height));
}

static void cylinder_shape_config_set_half_height(script_system *,
                                                  script_instance *,
                                                  script_args *args)
{
    if (args->count() < 1)
        return;
    cylinder_shape_config *c = c_dynamic_cast<cylinder_shape_config>(
        args->get_value(0).as_object().get());
    if (!c)
        return;
    c->half_height = args->get_value(1).as_number();
}

static void cylinder_shape_config_get_convex_radius(script_system *,
                                                    script_instance *,
                                                    script_args *args)
{
    if (args->count() < 1)
        return;
    cylinder_shape_config *c = c_dynamic_cast<cylinder_shape_config>(
        args->get_value(0).as_object().get());
    if (!c)
        return;
    args->push_return(value(c->convex_radius));
}

static void cylinder_shape_config_set_convex_radius(script_system *,
                                                    script_instance *,
                                                    script_args *args)
{
    if (args->count() < 1)
        return;
    cylinder_shape_config *c = c_dynamic_cast<cylinder_shape_config>(
        args->get_value(0).as_object().get());
    if (!c)
        return;
    c->convex_radius = args->get_value(1).as_number();
}

void cylinder_shape_config::reflect(reflection &r)
{
    convex_shape_config::reflect(r);
    r.add_property("radius",
                   cylinder_shape_config_get_radius,
                   cylinder_shape_config_set_radius);
    r.add_property("half_height",
                   cylinder_shape_config_get_half_height,
                   cylinder_shape_config_set_half_height);
    r.add_property("convex_radius",
                   cylinder_shape_config_get_convex_radius,
                   cylinder_shape_config_set_convex_radius);
}

#pragma endregion cylinder_shape_config

#pragma region sphere_shape_config
void sphere_shape_config::save(serializer &stream) const
{
    convex_shape_config::save(stream);
    stream.write(radius);
}

void sphere_shape_config::load(serializer &stream, serializer_link *link)
{
    convex_shape_config::load(stream, link);
    stream.read(radius);
}

void sphere_shape_config::save_xml(xml_serializer &stream,
                                   tinyxml2::XMLElement &element) const
{
    convex_shape_config::save_xml(stream, element);
    element.SetAttribute("radius", (double)radius);
}

void sphere_shape_config::load_xml(xml_serializer &stream,
                                   tinyxml2::XMLElement &element)
{
    convex_shape_config::load_xml(stream, element);
    radius = element.DoubleAttribute("radius");
}

static void sphere_shape_config_get_radius(script_system *,
                                           script_instance *,
                                           script_args *args)
{
    if (args->count() < 1)
        return;
    sphere_shape_config *s = c_dynamic_cast<sphere_shape_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->radius));
}

static void sphere_shape_config_set_radius(script_system *,
                                           script_instance *,
                                           script_args *args)
{
    if (args->count() < 1)
        return;
    sphere_shape_config *s = c_dynamic_cast<sphere_shape_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->radius = args->get_value(1).as_number();
}

void sphere_shape_config::reflect(reflection &r)
{
    convex_shape_config::reflect(r);
    r.add_property("radius",
                   sphere_shape_config_get_radius,
                   sphere_shape_config_set_radius);
}
#pragma endregion sphere_shape_config

#pragma region tapered_capsule_shape_config
void tapered_capsule_shape_config::save(serializer &stream) const
{
    convex_shape_config::save(stream);
    stream.write(top_radius);
    stream.write(bottom_radius);
    stream.write(half_height);
}

void tapered_capsule_shape_config::load(serializer &stream,
                                        serializer_link *link)
{
    convex_shape_config::load(stream, link);
    stream.read(top_radius);
    stream.read(bottom_radius);
    stream.read(half_height);
}

void tapered_capsule_shape_config::save_xml(xml_serializer &stream,
                                            tinyxml2::XMLElement &element) const
{
    convex_shape_config::save_xml(stream, element);
    element.SetAttribute("top_radius", (double)top_radius);
    element.SetAttribute("bottom_radius", (double)bottom_radius);
    element.SetAttribute("half_height", (double)half_height);
}

void tapered_capsule_shape_config::load_xml(xml_serializer &stream,
                                            tinyxml2::XMLElement &element)
{
    convex_shape_config::load_xml(stream, element);
    top_radius    = element.DoubleAttribute("top_radius");
    bottom_radius = element.DoubleAttribute("bottom_radius");
    half_height   = element.DoubleAttribute("half_height");
}

static void tapered_capsule_shape_config_get_top_radius(script_system *,
                                                        script_instance *,
                                                        script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_capsule_shape_config *s =
        c_dynamic_cast<tapered_capsule_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->top_radius));
}

static void tapered_capsule_shape_config_set_top_radius(script_system *,
                                                        script_instance *,
                                                        script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_capsule_shape_config *s =
        c_dynamic_cast<tapered_capsule_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    s->top_radius = args->get_value(1).as_number();
}

static void tapered_capsule_shape_config_get_bottom_radius(script_system *,
                                                           script_instance *,
                                                           script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_capsule_shape_config *s =
        c_dynamic_cast<tapered_capsule_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->bottom_radius));
}

static void tapered_capsule_shape_config_set_bottom_radius(script_system *,
                                                           script_instance *,
                                                           script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_capsule_shape_config *s =
        c_dynamic_cast<tapered_capsule_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    s->bottom_radius = args->get_value(1).as_number();
}

static void tapered_capsule_shape_config_get_half_height(script_system *,
                                                         script_instance *,
                                                         script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_capsule_shape_config *s =
        c_dynamic_cast<tapered_capsule_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->half_height));
}

static void tapered_capsule_shape_config_set_half_height(script_system *,
                                                         script_instance *,
                                                         script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_capsule_shape_config *s =
        c_dynamic_cast<tapered_capsule_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    s->half_height = args->get_value(1).as_number();
}

void tapered_capsule_shape_config::reflect(reflection &r)
{
    convex_shape_config::reflect(r);
    r.add_property("top_radius",
                   tapered_capsule_shape_config_get_top_radius,
                   tapered_capsule_shape_config_set_top_radius);
    r.add_property("bottom_radius",
                   tapered_capsule_shape_config_get_bottom_radius,
                   tapered_capsule_shape_config_set_bottom_radius);
    r.add_property("half_height",
                   tapered_capsule_shape_config_get_half_height,
                   tapered_capsule_shape_config_set_half_height);
}
#pragma endregion tapered_capsule_shape_config

#pragma region tapered_cylinder_shape_config
void tapered_cylinder_shape_config::save(serializer &stream) const
{
    convex_shape_config::save(stream);
    stream.write(top_radius);
    stream.write(bottom_radius);
    stream.write(half_height);
    stream.write(convex_radius);
}

void tapered_cylinder_shape_config::load(serializer &stream,
                                         serializer_link *link)
{
    convex_shape_config::load(stream, link);
    stream.read(top_radius);
    stream.read(bottom_radius);
    stream.read(half_height);
    stream.read(convex_radius);
}

void tapered_cylinder_shape_config::save_xml(
    xml_serializer &stream,
    tinyxml2::XMLElement &element) const
{
    convex_shape_config::save_xml(stream, element);
    element.SetAttribute("top_radius", (double)top_radius);
    element.SetAttribute("bottom_radius", (double)bottom_radius);
    element.SetAttribute("half_height", (double)half_height);
    element.SetAttribute("convex_radius", (double)convex_radius);
}

void tapered_cylinder_shape_config::load_xml(xml_serializer &stream,
                                             tinyxml2::XMLElement &element)
{
    convex_shape_config::load_xml(stream, element);
    top_radius    = element.DoubleAttribute("top_radius");
    bottom_radius = element.DoubleAttribute("bottom_radius");
    half_height   = element.DoubleAttribute("half_height");
    convex_radius = element.DoubleAttribute("convex_radius");
}

static void tapered_cylinder_shape_config_get_top_radius(script_system *,
                                                         script_instance *,
                                                         script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_cylinder_shape_config *s =
        c_dynamic_cast<tapered_cylinder_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->top_radius));
}

static void tapered_cylinder_shape_config_set_top_radius(script_system *,
                                                         script_instance *,
                                                         script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_cylinder_shape_config *s =
        c_dynamic_cast<tapered_cylinder_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    s->top_radius = args->get_value(1).as_number();
}

static void tapered_cylinder_shape_config_get_bottom_radius(script_system *,
                                                            script_instance *,
                                                            script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_cylinder_shape_config *s =
        c_dynamic_cast<tapered_cylinder_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->bottom_radius));
}

static void tapered_cylinder_shape_config_set_bottom_radius(script_system *,
                                                            script_instance *,
                                                            script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_cylinder_shape_config *s =
        c_dynamic_cast<tapered_cylinder_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    s->bottom_radius = args->get_value(1).as_number();
}

static void tapered_cylinder_shape_config_get_half_height(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_cylinder_shape_config *s =
        c_dynamic_cast<tapered_cylinder_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->half_height));
}

static void tapered_cylinder_shape_config_set_half_height(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_cylinder_shape_config *s =
        c_dynamic_cast<tapered_cylinder_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    s->half_height = args->get_value(1).as_number();
}

static void tapered_cylinder_shape_config_get_convex_radius(script_system *,
                                                            script_instance *,
                                                            script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_cylinder_shape_config *s =
        c_dynamic_cast<tapered_cylinder_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->convex_radius));
}

static void tapered_cylinder_shape_config_set_convex_radius(script_system *,
                                                            script_instance *,
                                                            script_args *args)
{
    if (args->count() < 1)
        return;
    tapered_cylinder_shape_config *s =
        c_dynamic_cast<tapered_cylinder_shape_config>(
            args->get_value(0).as_object().get());
    if (!s)
        return;
    s->convex_radius = args->get_value(1).as_number();
}

void tapered_cylinder_shape_config::reflect(reflection &r)
{
    convex_shape_config::reflect(r);
    r.add_property("top_radius",
                   tapered_cylinder_shape_config_get_top_radius,
                   tapered_cylinder_shape_config_set_top_radius);
    r.add_property("bottom_radius",
                   tapered_cylinder_shape_config_get_bottom_radius,
                   tapered_cylinder_shape_config_set_bottom_radius);
    r.add_property("half_height",
                   tapered_cylinder_shape_config_get_half_height,
                   tapered_cylinder_shape_config_set_half_height);
    r.add_property("convex_radius",
                   tapered_cylinder_shape_config_get_convex_radius,
                   tapered_cylinder_shape_config_set_convex_radius);
}

#pragma endregion tapered_cylinder_shape_config

#pragma region triangle_shape_config
void triangle_shape_config::save(serializer &stream) const
{
    convex_shape_config::save(stream);
    for (int i = 0; i < 3; ++i)
        stream.write(points[i]);
}

void triangle_shape_config::load(serializer &stream, serializer_link *link)
{
    convex_shape_config::load(stream, link);
    for (int i = 0; i < 3; ++i)
        stream.read(points[i]);
}

void triangle_shape_config::save_xml(xml_serializer &stream,
                                     tinyxml2::XMLElement &element) const
{
    convex_shape_config::save_xml(stream, element);
    for (int i = 0; i < 3; ++i)
    {
        auto *el = element.GetDocument()->NewElement("point");
        xml_serializer::write_vec3(*el, points[i]);
        element.InsertEndChild(el);
    }
}

void triangle_shape_config::load_xml(xml_serializer &stream,
                                     tinyxml2::XMLElement &element)
{
    convex_shape_config::load_xml(stream, element);
    auto *child = element.FirstChildElement("point");
    for (int i = 0; i < 3 && child; ++i)
    {
        points[i] = xml_serializer::read_vec3(*child);
        child     = child->NextSiblingElement("point");
    }
}

static void triangle_shape_config_get_points(script_system *,
                                             script_instance *,
                                             script_args *args)
{
    if (args->count() < 1)
        return;
    triangle_shape_config *s = c_dynamic_cast<triangle_shape_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;

    auto points = value::make_list();
    for (const auto &p : s->points)
        points.push(value(p));
    args->push_return(points);
}

static void triangle_shape_config_set_points(script_system *,
                                             script_instance *,
                                             script_args *args)
{
    if (args->count() < 1)
        return;
    triangle_shape_config *s = c_dynamic_cast<triangle_shape_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;

    for (size_t i = 0; i < args->count() && i <= 3; ++i)
        s->points[i] = args->get_value(i).as_vec3();
}

void triangle_shape_config::reflect(reflection &r)
{
    convex_shape_config::reflect(r);
    r.add_property("points",
                   triangle_shape_config_get_points,
                   triangle_shape_config_set_points);
}
#pragma endregion triangle_shape_config

#pragma region empty_shape_config
void empty_shape_config::save(serializer &stream) const
{
    shape_config::save(stream);
    stream.write(center_of_mass);
}

void empty_shape_config::load(serializer &stream, serializer_link *link)
{
    shape_config::load(stream, link);
    stream.read(center_of_mass);
}

void empty_shape_config::save_xml(xml_serializer &stream,
                                  tinyxml2::XMLElement &element) const
{
    shape_config::save_xml(stream, element);
    auto *child = element.GetDocument()->NewElement("center_of_mass");
    xml_serializer::write_vec3(*child, center_of_mass);
    element.InsertEndChild(child);
}

void empty_shape_config::load_xml(xml_serializer &stream,
                                  tinyxml2::XMLElement &element)
{
    shape_config::load_xml(stream, element);
    auto *child = element.FirstChildElement("center_of_mass");
    if (child)
        center_of_mass = xml_serializer::read_vec3(*child);
}

static void empty_shape_config_get_center_of_mass(script_system *,
                                                  script_instance *,
                                                  script_args *args)
{
    if (args->count() < 1)
        return;
    empty_shape_config *s = c_dynamic_cast<empty_shape_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;

    args->push_return(value(s->center_of_mass));
}

static void empty_shape_config_set_center_of_mass(script_system *,
                                                  script_instance *,
                                                  script_args *args)
{
    if (args->count() < 1)
        return;
    empty_shape_config *s = c_dynamic_cast<empty_shape_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;

    s->center_of_mass = args->get_value(1).as_vec3();
}

void empty_shape_config::reflect(reflection &r)
{
    shape_config::reflect(r);
    r.add_property("center_of_mass",
                   empty_shape_config_get_center_of_mass,
                   empty_shape_config_set_center_of_mass);
}

#pragma endregion empty_shape_config

#pragma region rigid_body_creation_config
const rtti rigid_body_creation_config::TYPE(
    "zabato.physics.rigid_body_creation_config",
    &base_object::TYPE,
    rigid_body_creation_config::reflect);

static void rigid_body_creation_config_get_shape(script_system *,
                                                 script_instance *,
                                                 script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(static_cast<pointer<base_object>>(s->shape)));
}

static void rigid_body_creation_config_set_shape(script_system *,
                                                 script_instance *,
                                                 script_args *args)
{
    if (args->count() < 1)
        return;

    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
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
               "Invalid shape pass to rigid_body_creation_config");
        return;
    }

    s->shape = shape;
}

static void rigid_body_creation_config_get_position(script_system *,
                                                    script_instance *,
                                                    script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->position));
}

static void rigid_body_creation_config_set_position(script_system *,
                                                    script_instance *,
                                                    script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->position = args->get_value(1).as_vec3();
}

static void rigid_body_creation_config_get_rotation(script_system *,
                                                    script_instance *,
                                                    script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->rotation));
}

static void rigid_body_creation_config_set_rotation(script_system *,
                                                    script_instance *,
                                                    script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->rotation = args->get_value(1).as_quat();
}

static void rigid_body_creation_config_get_motion_type(script_system *,
                                                       script_instance *,
                                                       script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value((int64_t)s->motion_type));
}

static void rigid_body_creation_config_set_motion_type(script_system *,
                                                       script_instance *,
                                                       script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->motion_type = (physics_motion_type)args->get_value(1).as_int();
}

static void rigid_body_creation_config_get_allowed_dofs(script_system *,
                                                        script_instance *,
                                                        script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value((int64_t)s->allowed_dofs));
}

static void rigid_body_creation_config_set_allowed_dofs(script_system *,
                                                        script_instance *,
                                                        script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->allowed_dofs = (physics_allowed_dofs)args->get_value(1).as_int();
}

static void
rigid_body_creation_config_get_allow_dynamic_or_kinematic(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->allow_dynamic_or_kinematic));
}

static void
rigid_body_creation_config_set_allow_dynamic_or_kinematic(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->allow_dynamic_or_kinematic = args->get_value(1).as_bool();
}

static void rigid_body_creation_config_get_is_sensor(script_system *,
                                                     script_instance *,
                                                     script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->is_sensor));
}

static void rigid_body_creation_config_set_is_sensor(script_system *,
                                                     script_instance *,
                                                     script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->is_sensor = args->get_value(1).as_bool();
}

static void rigid_body_creation_config_get_collide_kinematic_vs_non_dynamic(
    script_system *,
    script_instance *,
    script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->collide_kinematic_vs_non_dynamic));
}

static void rigid_body_creation_config_set_collide_kinematic_vs_non_dynamic(
    script_system *,
    script_instance *,
    script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->collide_kinematic_vs_non_dynamic = args->get_value(1).as_bool();
}

static void
rigid_body_creation_config_get_use_manifold_reduction(script_system *,
                                                      script_instance *,
                                                      script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->use_manifold_reduction));
}

static void
rigid_body_creation_config_set_use_manifold_reduction(script_system *,
                                                      script_instance *,
                                                      script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->use_manifold_reduction = args->get_value(1).as_bool();
}

static void
rigid_body_creation_config_get_apply_gyroscopic_force(script_system *,
                                                      script_instance *,
                                                      script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->apply_gyroscopic_force));
}

static void
rigid_body_creation_config_set_apply_gyroscopic_force(script_system *,
                                                      script_instance *,
                                                      script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->apply_gyroscopic_force = args->get_value(1).as_bool();
}

static void rigid_body_creation_config_get_allow_sleeping(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->allow_sleeping));
}

static void rigid_body_creation_config_set_allow_sleeping(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->allow_sleeping = args->get_value(1).as_bool();
}

static void rigid_body_creation_config_get_friction(script_system *,
                                                    script_instance *,
                                                    script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->friction));
}

static void rigid_body_creation_config_set_friction(script_system *,
                                                    script_instance *,
                                                    script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->friction = args->get_value(1).as_number();
}

static void rigid_body_creation_config_get_restitution(script_system *,
                                                       script_instance *,
                                                       script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->restitution));
}

static void rigid_body_creation_config_set_restitution(script_system *,
                                                       script_instance *,
                                                       script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->restitution = args->get_value(1).as_number();
}

static void rigid_body_creation_config_get_linear_damping(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->linear_damping));
}

static void rigid_body_creation_config_set_linear_damping(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->linear_damping = args->get_value(1).as_number();
}

static void rigid_body_creation_config_get_angular_damping(script_system *,
                                                           script_instance *,
                                                           script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->angular_damping));
}

static void rigid_body_creation_config_set_angular_damping(script_system *,
                                                           script_instance *,
                                                           script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->angular_damping = args->get_value(1).as_number();
}

static void
rigid_body_creation_config_get_max_linear_velocity(script_system *,
                                                   script_instance *,
                                                   script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->max_linear_velocity));
}

static void
rigid_body_creation_config_set_max_linear_velocity(script_system *,
                                                   script_instance *,
                                                   script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->max_linear_velocity = args->get_value(1).as_number();
}

static void
rigid_body_creation_config_get_max_angular_velocity(script_system *,
                                                    script_instance *,
                                                    script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->max_angular_velocity));
}

static void
rigid_body_creation_config_set_max_angular_velocity(script_system *,
                                                    script_instance *,
                                                    script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->max_angular_velocity = args->get_value(1).as_number();
}

static void rigid_body_creation_config_get_gravity_factor(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->gravity_factor));
}

static void rigid_body_creation_config_set_gravity_factor(script_system *,
                                                          script_instance *,
                                                          script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->gravity_factor = args->get_value(1).as_number();
}

static void
rigid_body_creation_config_get_num_velocity_steps_override(script_system *,
                                                           script_instance *,
                                                           script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value((int64_t)s->num_velocity_steps_override));
}

static void
rigid_body_creation_config_set_num_velocity_steps_override(script_system *,
                                                           script_instance *,
                                                           script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->num_velocity_steps_override = args->get_value(1).as_int();
}

static void
rigid_body_creation_config_get_num_position_steps_override(script_system *,
                                                           script_instance *,
                                                           script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value((int64_t)s->num_position_steps_override));
}

static void
rigid_body_creation_config_set_num_position_steps_override(script_system *,
                                                           script_instance *,
                                                           script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->num_position_steps_override = args->get_value(1).as_int();
}

static void
rigid_body_creation_config_get_override_mass_properties(script_system *,
                                                        script_instance *,
                                                        script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value((int64_t)s->override_mass_properties));
}

static void
rigid_body_creation_config_set_override_mass_properties(script_system *,
                                                        script_instance *,
                                                        script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->override_mass_properties =
        (physics_override_mass_properties)args->get_value(1).as_int();
}

static void rigid_body_creation_config_get_inertia_multiplier(script_system *,
                                                              script_instance *,
                                                              script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->inertia_multiplier));
}

static void rigid_body_creation_config_set_inertia_multiplier(script_system *,
                                                              script_instance *,
                                                              script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->inertia_multiplier = args->get_value(1).as_number();
}

static void rigid_body_creation_config_get_mass_override(script_system *,
                                                         script_instance *,
                                                         script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value(s->mass_override));
}

static void rigid_body_creation_config_set_mass_override(script_system *,
                                                         script_instance *,
                                                         script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->mass_override = args->get_value(1).as_number();
}

static void rigid_body_creation_config_get_quality(script_system *,
                                                   script_instance *,
                                                   script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value((int64_t)s->quality));
}

static void rigid_body_creation_config_set_quality(script_system *,
                                                   script_instance *,
                                                   script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->quality = (physics_motion_quality)args->get_value(1).as_int();
}

static void rigid_body_creation_config_get_layer(script_system *,
                                                 script_instance *,
                                                 script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    args->push_return(value((int64_t)s->layer));
}

static void rigid_body_creation_config_set_layer(script_system *,
                                                 script_instance *,
                                                 script_args *args)
{
    if (args->count() < 1)
        return;
    rigid_body_creation_config *s = c_dynamic_cast<rigid_body_creation_config>(
        args->get_value(0).as_object().get());
    if (!s)
        return;
    s->layer = (uint16_t)args->get_value(1).as_int();
}

void rigid_body_creation_config::reflect(reflection &r)
{
    base_object::reflect(r);

    auto shape_enum = value::make_list();
    for (auto it = &ALL_SHAPE_CONFIG_TYPES[0]; it->_0 != nullptr; it++)
        shape_enum.push(it->_0->name());

    value shape_attrs = value::make_map();
    shape_attrs.set_field("types", shape_enum);

    r.add_property("shape",
                   rigid_body_creation_config_get_shape,
                   rigid_body_creation_config_set_shape,
                   shape_attrs);

    r.add_property("position",
                   rigid_body_creation_config_get_position,
                   rigid_body_creation_config_set_position);

    r.add_property("rotation",
                   rigid_body_creation_config_get_rotation,
                   rigid_body_creation_config_set_rotation);

    auto motion_type_enum = value::make_list();
    motion_type_enum.push("static");
    motion_type_enum.push("kinematic");
    motion_type_enum.push("dynamic");

    auto motion_type_attr = value::make_map();
    motion_type_attr.set_field("enum", motion_type_enum);
    r.add_property("motion_type",
                   rigid_body_creation_config_get_motion_type,
                   rigid_body_creation_config_set_motion_type,
                   motion_type_attr);

    auto allowed_dofs_enum = value::make_list();
    allowed_dofs_enum.push("all");
    allowed_dofs_enum.push("plane_2d");
    allowed_dofs_enum.push("rotation_only");
    allowed_dofs_enum.push("translation_only");

    auto allowed_dofs_attr = value::make_map();
    allowed_dofs_attr.set_field("enum", allowed_dofs_enum);
    r.add_property("allowed_dofs",
                   rigid_body_creation_config_get_allowed_dofs,
                   rigid_body_creation_config_set_allowed_dofs,
                   allowed_dofs_attr);

    r.add_property("allow_dynamic_or_kinematic",
                   rigid_body_creation_config_get_allow_dynamic_or_kinematic,
                   rigid_body_creation_config_set_allow_dynamic_or_kinematic);

    r.add_property("is_sensor",
                   rigid_body_creation_config_get_is_sensor,
                   rigid_body_creation_config_set_is_sensor);

    r.add_property(
        "collide_kinematic_vs_non_dynamic",
        rigid_body_creation_config_get_collide_kinematic_vs_non_dynamic,
        rigid_body_creation_config_set_collide_kinematic_vs_non_dynamic);

    r.add_property("use_manifold_reduction",
                   rigid_body_creation_config_get_use_manifold_reduction,
                   rigid_body_creation_config_set_use_manifold_reduction);

    r.add_property("apply_gyroscopic_force",
                   rigid_body_creation_config_get_apply_gyroscopic_force,
                   rigid_body_creation_config_set_apply_gyroscopic_force);

    r.add_property("allow_sleeping",
                   rigid_body_creation_config_get_allow_sleeping,
                   rigid_body_creation_config_set_allow_sleeping);

    r.add_property("friction",
                   rigid_body_creation_config_get_friction,
                   rigid_body_creation_config_set_friction);

    r.add_property("restitution",
                   rigid_body_creation_config_get_restitution,
                   rigid_body_creation_config_set_restitution);

    r.add_property("linear_damping",
                   rigid_body_creation_config_get_linear_damping,
                   rigid_body_creation_config_set_linear_damping);

    r.add_property("angular_damping",
                   rigid_body_creation_config_get_angular_damping,
                   rigid_body_creation_config_set_angular_damping);

    r.add_property("max_linear_velocity",
                   rigid_body_creation_config_get_max_linear_velocity,
                   rigid_body_creation_config_set_max_linear_velocity);

    r.add_property("max_angular_velocity",
                   rigid_body_creation_config_get_max_angular_velocity,
                   rigid_body_creation_config_set_max_angular_velocity);

    r.add_property("gravity_factor",
                   rigid_body_creation_config_get_gravity_factor,
                   rigid_body_creation_config_set_gravity_factor);

    r.add_property("num_velocity_steps_override",
                   rigid_body_creation_config_get_num_velocity_steps_override,
                   rigid_body_creation_config_set_num_velocity_steps_override);

    r.add_property("num_position_steps_override",
                   rigid_body_creation_config_get_num_position_steps_override,
                   rigid_body_creation_config_set_num_position_steps_override);

    auto override_mass_properties_enum = value::make_list();
    override_mass_properties_enum.push("calculate_mass_and_inertia");
    override_mass_properties_enum.push("calculate_inertia");
    override_mass_properties_enum.push("mass_and_inertia_provided");

    auto override_mass_properties_attr = value::make_map();
    override_mass_properties_attr.set_field("enum",
                                            override_mass_properties_enum);
    r.add_property("override_mass_properties",
                   rigid_body_creation_config_get_override_mass_properties,
                   rigid_body_creation_config_set_override_mass_properties,
                   override_mass_properties_attr);

    r.add_property("inertia_multiplier",
                   rigid_body_creation_config_get_inertia_multiplier,
                   rigid_body_creation_config_set_inertia_multiplier);

    r.add_property("mass_override",
                   rigid_body_creation_config_get_mass_override,
                   rigid_body_creation_config_set_mass_override);

    auto motion_quality_enum = value::make_list();
    motion_quality_enum.push("discrete");
    motion_quality_enum.push("continuous");

    auto motion_quality_attr = value::make_map();
    motion_quality_attr.set_field("enum", motion_quality_enum);
    r.add_property("quality",
                   rigid_body_creation_config_get_quality,
                   rigid_body_creation_config_set_quality,
                   motion_quality_attr);

    r.add_property("layer",
                   rigid_body_creation_config_get_layer,
                   rigid_body_creation_config_set_layer);
}

void rigid_body_creation_config::save(serializer &stream) const
{
    if (shape)
    {
        string type_name = shape->type().name();
        stream.write(type_name);
        shape->save(stream);
    }
    else
    {
        string empty;
        stream.write(empty);
    }

    stream.write(position);
    stream.write(rotation);

    stream.write((int32_t)motion_type);
    stream.write((int32_t)allowed_dofs);
    stream.write(allow_dynamic_or_kinematic);
    stream.write(is_sensor);
    stream.write(collide_kinematic_vs_non_dynamic);
    stream.write(use_manifold_reduction);
    stream.write(apply_gyroscopic_force);
    stream.write(allow_sleeping);

    stream.write(friction);
    stream.write(restitution);
    stream.write(linear_damping);
    stream.write(angular_damping);
    stream.write(max_linear_velocity);
    stream.write(max_angular_velocity);
    stream.write(gravity_factor);

    stream.write(num_velocity_steps_override);
    stream.write(num_position_steps_override);

    stream.write((int32_t)override_mass_properties);
    stream.write(inertia_multiplier);
    stream.write(mass_override);

    stream.write((int8_t)quality);
    stream.write(layer);
}

void rigid_body_creation_config::load(serializer &stream, serializer_link *link)
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

    stream.read(position);
    stream.read(rotation);

    int32_t mt;
    stream.read(mt);
    motion_type = (physics_motion_type)mt;
    int32_t ad;
    stream.read(ad);
    allowed_dofs = (physics_allowed_dofs)ad;
    stream.read(allow_dynamic_or_kinematic);
    stream.read(is_sensor);
    stream.read(collide_kinematic_vs_non_dynamic);
    stream.read(use_manifold_reduction);
    stream.read(apply_gyroscopic_force);
    stream.read(allow_sleeping);

    stream.read(friction);
    stream.read(restitution);
    stream.read(linear_damping);
    stream.read(angular_damping);
    stream.read(max_linear_velocity);
    stream.read(max_angular_velocity);
    stream.read(gravity_factor);

    stream.read(num_velocity_steps_override);
    stream.read(num_position_steps_override);

    int32_t omp;
    stream.read(omp);
    override_mass_properties = (physics_override_mass_properties)omp;
    stream.read(inertia_multiplier);
    stream.read(mass_override);

    int8_t q;
    stream.read(q);
    quality = (physics_motion_quality)q;
    stream.read(layer);
}

void rigid_body_creation_config::save_xml(xml_serializer &stream,
                                          tinyxml2::XMLElement &element) const
{
    auto *shape_el = element.GetDocument()->NewElement("shape");
    if (shape)
    {
        shape_el->SetAttribute("type_name", shape->type().name());
        shape->save_xml(stream, *shape_el);
    }
    element.InsertEndChild(shape_el);

    auto *pos_el = element.GetDocument()->NewElement("position");
    xml_serializer::write_vec3(*pos_el, position);
    element.InsertEndChild(pos_el);

    auto *rot_el = element.GetDocument()->NewElement("rotation");
    xml_serializer::write_quat(*rot_el, rotation);
    element.InsertEndChild(rot_el);

    // Simulation
    element.SetAttribute("motion_type", (int)motion_type);
    element.SetAttribute("allowed_dofs", (int)allowed_dofs);
    element.SetAttribute("allow_dynamic_or_kinematic",
                         allow_dynamic_or_kinematic);
    element.SetAttribute("is_sensor", is_sensor);
    element.SetAttribute("collide_kinematic_vs_non_dynamic",
                         collide_kinematic_vs_non_dynamic);
    element.SetAttribute("use_manifold_reduction", use_manifold_reduction);
    element.SetAttribute("apply_gyroscopic_force", apply_gyroscopic_force);
    element.SetAttribute("allow_sleeping", allow_sleeping);

    element.SetAttribute("friction", (float)friction);
    element.SetAttribute("restitution", (float)restitution);
    element.SetAttribute("linear_damping", (float)linear_damping);
    element.SetAttribute("angular_damping", (float)angular_damping);
    element.SetAttribute("max_linear_velocity", (float)max_linear_velocity);
    element.SetAttribute("max_angular_velocity", (float)max_angular_velocity);
    element.SetAttribute("gravity_factor", (float)gravity_factor);

    element.SetAttribute("num_velocity_steps_override",
                         num_velocity_steps_override);
    element.SetAttribute("num_position_steps_override",
                         num_position_steps_override);

    // Mass
    element.SetAttribute("override_mass_properties",
                         (int)override_mass_properties);
    element.SetAttribute("inertia_multiplier", (float)inertia_multiplier);
    element.SetAttribute("mass_override", (float)mass_override);

    element.SetAttribute("quality", (int)quality);
    element.SetAttribute("layer", (int)layer);
}

void rigid_body_creation_config::load_xml(xml_serializer &stream,
                                          tinyxml2::XMLElement &element)
{
    auto *shape_el = element.FirstChildElement("shape");
    if (shape_el)
    {
        if (shape_el->Attribute("type_name"))
        {
            string type_name = shape_el->Attribute("type_name");
            auto ptr         = create_shape_config(type_name);
            if (ptr)
            {
                ptr->load_xml(stream, *shape_el);
                shape = ptr;
            }
        }
    }

    auto *pos_el = element.FirstChildElement("position");
    if (pos_el)
        position = xml_serializer::read_vec3(*pos_el);

    auto *rot_el = element.FirstChildElement("rotation");
    if (rot_el)
        rotation = xml_serializer::read_quat(*rot_el);

    // Simulation
    motion_type  = (physics_motion_type)element.IntAttribute("motion_type",
                                                            (int)motion_type);
    allowed_dofs = (physics_allowed_dofs)element.IntAttribute(
        "allowed_dofs", (int)allowed_dofs);
    allow_dynamic_or_kinematic = element.BoolAttribute(
        "allow_dynamic_or_kinematic", allow_dynamic_or_kinematic);
    is_sensor = element.BoolAttribute("is_sensor", is_sensor);
    collide_kinematic_vs_non_dynamic = element.BoolAttribute(
        "collide_kinematic_vs_non_dynamic", collide_kinematic_vs_non_dynamic);
    use_manifold_reduction =
        element.BoolAttribute("use_manifold_reduction", use_manifold_reduction);
    apply_gyroscopic_force =
        element.BoolAttribute("apply_gyroscopic_force", apply_gyroscopic_force);
    allow_sleeping = element.BoolAttribute("allow_sleeping", allow_sleeping);

    friction    = element.DoubleAttribute("friction", (double)friction);
    restitution = element.DoubleAttribute("restitution", (double)restitution);
    linear_damping =
        element.DoubleAttribute("linear_damping", (double)linear_damping);
    angular_damping =
        element.DoubleAttribute("angular_damping", (double)angular_damping);
    max_linear_velocity  = element.DoubleAttribute("max_linear_velocity",
                                                  (double)max_linear_velocity);
    max_angular_velocity = element.DoubleAttribute(
        "max_angular_velocity", (double)max_angular_velocity);
    gravity_factor =
        element.DoubleAttribute("gravity_factor", (double)gravity_factor);

    num_velocity_steps_override = element.UnsignedAttribute(
        "num_velocity_steps_override", num_velocity_steps_override);
    num_position_steps_override = element.UnsignedAttribute(
        "num_position_steps_override", num_position_steps_override);

    // Mass
    override_mass_properties =
        (physics_override_mass_properties)element.IntAttribute(
            "override_mass_properties", (int)override_mass_properties);
    inertia_multiplier = element.DoubleAttribute("inertia_multiplier",
                                                 (double)inertia_multiplier);
    mass_override =
        element.DoubleAttribute("mass_override", (double)mass_override);

    quality =
        (physics_motion_quality)element.IntAttribute("quality", (int)quality);
    layer = (uint16_t)element.IntAttribute("layer", layer);
}
#pragma endregion rigid_body_creation_config
} // namespace zabato::physics