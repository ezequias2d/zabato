#include <string.h>
#include <tinyxml2.h>

#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/spatial.hpp>
#include <zabato/string.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti spatial::TYPE("zabato::spatial", &object::TYPE, spatial::reflect);

static void spatial_translate_getter(script_system *sys,
                                     script_instance *ctx,
                                     script_args *args)
{
    if (args->count() < 1)
        return;
    value v    = args->get_value(0);
    object *o  = v.as_object();
    spatial *s = c_dynamic_cast<spatial>(o);
    if (s)
    {
        vec3<real> t = s->get_local().translate();
        value v      = t;
        args->push_return(v);
    }
}

static void spatial_translate_setter(script_system *sys,
                                     script_instance *ctx,
                                     script_args *args)
{
    if (args->count() < 2)
        return;
    value v1   = args->get_value(0);
    object *o  = v1.as_object();
    spatial *s = c_dynamic_cast<spatial>(o);
    value v2   = args->get_value(1);
    if (s && v2.is_vec3())
    {
        vec3<real> t         = v2.as_vec3();
        transformation trans = s->get_local();
        trans.set_translate(t);
        s->set_local(trans);
    }
}

static void spatial_scale_getter(script_system *sys,
                                 script_instance *ctx,
                                 script_args *args)
{
    if (args->count() < 1)
        return;
    value v    = args->get_value(0);
    object *o  = v.as_object();
    spatial *s = c_dynamic_cast<spatial>(o);
    if (s)
    {
        vec3<real> sc = s->get_local().scale();
        value v       = sc;
        args->push_return(v);
    }
}

static void spatial_scale_setter(script_system *sys,
                                 script_instance *ctx,
                                 script_args *args)
{
    if (args->count() < 2)
        return;
    value v1   = args->get_value(0);
    object *o  = v1.as_object();
    spatial *s = c_dynamic_cast<spatial>(o);
    value v2   = args->get_value(1);
    if (s && v2.is_vec3())
    {
        vec3<real> sc        = v2.as_vec3();
        transformation trans = s->get_local();
        trans.set_scale(sc);
        s->set_local(trans);
    }
}

void spatial::reflect(reflection &r)
{
    object::reflect(r);
    r.properties.add("translation",
                     {spatial_translate_getter, spatial_translate_setter});
    r.properties.add("scale", {spatial_scale_getter, spatial_scale_setter});
}

void spatial::save_xml(xml_serializer &serializer,
                       tinyxml2::XMLElement &element) const
{
    object::save_xml(serializer, element);
    xml_serializer::write_transform(element, local);
}

void spatial::load_xml(xml_serializer &serializer,
                       tinyxml2::XMLElement &element)
{
    object::load_xml(serializer, element);

    if (element.FirstChildElement("transform"))
    {
        transformation t = get_local();
        xml_serializer::read_transform_into(element, t);
        set_local(t);
    }
}

void spatial::link(xml_serializer &serializer, tinyxml2::XMLElement &element)
{
    object::link(serializer, element);

    tinyxml2::XMLNode *parent = element.Parent();
    if (parent)
    {
        tinyxml2::XMLElement *parentEl = parent->ToElement();
        if (parentEl)
        {
            const char *id = parentEl->Attribute("id");
            if (id)
            {
                uuid parentId;
                bool result = uuid::try_parse({id, strlen(id)}, parentId);
                assert(result && "Invalid parent id");
                object *parentObj = serializer.get_object(parentId);
                spatial *parent   = c_dynamic_cast<spatial>(parentObj);
                assert(parent && "Invalid parent");
                set_parent(parent);
            }
        }
    }
}

transformation &spatial::get_world_transform()
{
    if (is_world_dirty)
    {
        spatial *p = parent();
        if (p)
            world_transform.product(p->get_world_transform(), local);
        else
            world_transform = local;
        is_world_dirty = false;
    }
    return world_transform;
}

void spatial::set_local(const transformation &local)
{
    this->local = local;
    force_dirty();
}

void spatial::set_world(const transformation &world)
{
    this->world_transform = world;
    force_dirty();

    spatial *p = parent();
    if (p)
    {
        transformation inv_parent;
        p->get_world_transform().inverse(inv_parent);
        local.product(inv_parent, world);
    }
    else
    {
        local = world;
    }
}

} // namespace zabato