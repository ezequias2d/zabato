#include "zabato/color.hpp"
#include <zabato/light.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti light::TYPE("zabato::light", &spatial::TYPE, light::reflect);

static void
light_type_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v   = args->get_value(0);
    object *o = v.as_object();
    light *l  = c_dynamic_cast<light>(o);
    if (l)
        args->push_return((int64_t)l->get_data().type);
}

static void
light_type_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1  = args->get_value(0);
    object *o = v1.as_object();
    light *l  = c_dynamic_cast<light>(o);
    value v2  = args->get_value(1);
    if (l)
    {
        light_data d = l->get_data();
        d.type       = (light_type)v2.as_int();
        l->set_data(d);
    }
}

static void
light_color_getter(script_system *, script_instance *, script_args *args)
{
    // Reuse for ambient/diffuse/specular via closure or separate funcs?
    // For simplicity, separate funcs or copy-paste.
    // Let's do ambient.
    if (args->count() < 1)
        return;
    value v   = args->get_value(0);
    object *o = v.as_object();
    light *l  = c_dynamic_cast<light>(o);
    if (l)
    {
        color5551 c = l->get_data().ambient;
        value v     = color(c);
        args->push_return(v);
    }
}

static void
light_ambient_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1  = args->get_value(0);
    object *o = v1.as_object();
    light *l  = c_dynamic_cast<light>(o);
    value v2  = args->get_value(1);
    if (l && v2.is_color())
    {
        light_data d = l->get_data();
        d.ambient    = color5551(v2.as_color());
        l->set_data(d);
    }
}

static void
light_diffuse_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v   = args->get_value(0);
    object *o = v.as_object();
    light *l  = c_dynamic_cast<light>(o);
    if (l)
    {
        color5551 c = l->get_data().diffuse;
        value v     = color(c);
        args->push_return(v);
    }
}

static void
light_diffuse_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1  = args->get_value(0);
    object *o = v1.as_object();
    light *l  = c_dynamic_cast<light>(o);
    value v2  = args->get_value(1);
    if (l && v2.is_color())
    {
        light_data d = l->get_data();
        d.diffuse    = color5551(v2.as_color());
        l->set_data(d);
    }
}

void light::reflect(reflection &r)
{
    spatial::reflect(r);
    r.properties.add("type", {light_type_getter, light_type_setter});
    r.properties.add("ambient", {light_color_getter, light_ambient_setter});
    r.properties.add("diffuse", {light_diffuse_getter, light_diffuse_setter});
}

light::light()
{
    // Default light data
    m_data.type                  = light_type::point;
    m_data.ambient               = {0.2f, 0.2f, 0.2f, 1.0f};
    m_data.diffuse               = {0.8f, 0.8f, 0.8f, 1.0f};
    m_data.specular              = {1.0f, 1.0f, 1.0f, 1.0f};
    m_data.position              = {0, 0, 0};
    m_data.spot_direction        = {0, 0, -1};
    m_data.spot_cutoff           = 180.0f;
    m_data.spot_exponent         = 0.0f;
    m_data.constant_attenuation  = 1.0f;
    m_data.linear_attenuation    = 0.0f;
    m_data.quadratic_attenuation = 0.0f;
}

void light::on_transform_changed()
{
    spatial::on_transform_changed();
    m_dirty_transform = true;
}

void light::update_light_transform()
{
    transformation w      = get_world_transform();
    m_data.position       = w.translate();
    m_data.spot_direction = w.rotate() * vec3<real>(0, 0, -1);
}

void light::set_data(const light_data &data)
{
    m_data = data;
    update_light_transform();
}

const light_data &light::get_data()
{
    if (m_dirty_transform)
        update_light_transform();
    return m_data;
}

void light::save_xml(xml_serializer &serializer,
                     tinyxml2::XMLElement &element) const
{
    spatial::save_xml(serializer, element);

    element.SetAttribute("type", (int)m_data.type);

    // Helper to write color
    auto write_col = [&](const char *name, const color5551 &c)
    {
        auto *child = element.GetDocument()->NewElement(name);
        element.InsertEndChild(child);
        color color(c);
        serializer.write_vec4(*child, color.as_vec4());
    };

    write_col("ambient", m_data.ambient);
    write_col("diffuse", m_data.diffuse);
    write_col("specular", m_data.specular);

    element.SetAttribute("cutoff", (double)m_data.spot_cutoff);
    element.SetAttribute("exponent", (double)m_data.spot_exponent);
    element.SetAttribute("kC", (double)m_data.constant_attenuation);
    element.SetAttribute("kL", (double)m_data.linear_attenuation);
    element.SetAttribute("kQ", (double)m_data.quadratic_attenuation);
}

void light::load_xml(xml_serializer &serializer, tinyxml2::XMLElement &element)
{
    spatial::load_xml(serializer, element);

    int type_int = 0;
    if (element.QueryIntAttribute("type", &type_int) == tinyxml2::XML_SUCCESS)
        m_data.type = (light_type)type_int;

    auto read_col = [&](const char *name, color5551 &c)
    {
        auto *child = element.FirstChildElement(name);
        if (child)
        {
            vec4<real> v = serializer.read_vec4(*child);
            c            = {(float)v.x, (float)v.y, (float)v.z, (float)v.w};
        }
    };

    read_col("ambient", m_data.ambient);
    read_col("diffuse", m_data.diffuse);
    read_col("specular", m_data.specular);

    float val;
    if (element.QueryFloatAttribute("cutoff", &val) == tinyxml2::XML_SUCCESS)
        m_data.spot_cutoff = val;
    if (element.QueryFloatAttribute("exponent", &val) == tinyxml2::XML_SUCCESS)
        m_data.spot_exponent = val;
    if (element.QueryFloatAttribute("kC", &val) == tinyxml2::XML_SUCCESS)
        m_data.constant_attenuation = val;
    if (element.QueryFloatAttribute("kL", &val) == tinyxml2::XML_SUCCESS)
        m_data.linear_attenuation = val;
    if (element.QueryFloatAttribute("kQ", &val) == tinyxml2::XML_SUCCESS)
        m_data.quadratic_attenuation = val;

    update_light_transform();
}

} // namespace zabato
