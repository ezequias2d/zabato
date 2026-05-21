#include <zabato/color.hpp>
#include <zabato/light.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/serializer.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti light::TYPE("zabato.light", &spatial::TYPE, light::reflect);

static void
light_type_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    light *l       = c_dynamic_cast<light>(o);
    if (l)
        args->push_return((int64_t)l->get_data().type);
}

static void
light_type_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    light *l       = c_dynamic_cast<light>(o);
    value v2       = args->get_value(1);
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
    if (args->count() < 1)
        return;
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    light *l       = c_dynamic_cast<light>(o);
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
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    light *l       = c_dynamic_cast<light>(o);
    value v2       = args->get_value(1);
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
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    light *l       = c_dynamic_cast<light>(o);
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
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    light *l       = c_dynamic_cast<light>(o);
    value v2       = args->get_value(1);
    if (l && v2.is_color())
    {
        light_data d = l->get_data();
        d.diffuse    = color5551(v2.as_color());
        l->set_data(d);
    }
}

static void is_spot(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
    {
        args->push_return(false);
        return;
    }
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    light *l       = c_dynamic_cast<light>(o);
    if (l && l->get_data().type == light_type::spot)
        args->push_return(true);
    else
        args->push_return(false);
}

static void
light_specular_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    light *l       = c_dynamic_cast<light>(o);
    if (l)
    {
        color5551 c = l->get_data().specular;
        value v     = color(c);
        args->push_return(v);
    }
}

static void
light_specular_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    light *l       = c_dynamic_cast<light>(o);
    value v2       = args->get_value(1);
    if (l && v2.is_color())
    {
        light_data d = l->get_data();
        d.specular   = color5551(v2.as_color());
        l->set_data(d);
    }
}

static void
light_cutoff_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    light *l       = c_dynamic_cast<light>(o);
    if (l)
        args->push_return((double)l->get_data().spot_cutoff);
}

static void
light_cutoff_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    light *l       = c_dynamic_cast<light>(o);
    value v2       = args->get_value(1);
    if (l && v2.is_number())
    {
        light_data d = l->get_data();
        float val    = (float)v2.as_number();
        if (val < 0.0f)
            val = 0.0f;
        if (val > 90.0f)
            val = 90.0f;
        d.spot_cutoff = val;
        l->set_data(d);
    }
}

static void
light_exponent_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    light *l       = c_dynamic_cast<light>(o);
    if (l)
        args->push_return((double)l->get_data().spot_exponent);
}

static void
light_exponent_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    light *l       = c_dynamic_cast<light>(o);
    value v2       = args->get_value(1);
    if (l && v2.is_number())
    {
        light_data d    = l->get_data();
        d.spot_exponent = (float)v2.as_number();
        l->set_data(d);
    }
}

static void is_positional(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
    {
        args->push_return(false);
        return;
    }
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    light *l       = c_dynamic_cast<light>(o);
    if (l && l->get_data().type != light_type::directional)
        args->push_return(true);
    else
        args->push_return(false);
}

static void
light_constant_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    light *l       = c_dynamic_cast<light>(o);
    if (l)
        args->push_return((double)l->get_data().constant_attenuation);
}

static void
light_constant_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    light *l       = c_dynamic_cast<light>(o);
    value v2       = args->get_value(1);
    if (l && v2.is_number())
    {
        light_data d           = l->get_data();
        d.constant_attenuation = (float)v2.as_number();
        l->set_data(d);
    }
}

static void
light_linear_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    light *l       = c_dynamic_cast<light>(o);
    if (l)
        args->push_return((double)l->get_data().linear_attenuation);
}

static void
light_linear_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    light *l       = c_dynamic_cast<light>(o);
    value v2       = args->get_value(1);
    if (l && v2.is_number())
    {
        light_data d         = l->get_data();
        d.linear_attenuation = (float)v2.as_number();
        l->set_data(d);
    }
}

static void
light_quadratic_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    light *l       = c_dynamic_cast<light>(o);
    if (l)
        args->push_return((double)l->get_data().quadratic_attenuation);
}

static void
light_quadratic_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    light *l       = c_dynamic_cast<light>(o);
    value v2       = args->get_value(1);
    if (l && v2.is_number())
    {
        light_data d            = l->get_data();
        d.quadratic_attenuation = (float)v2.as_number();
        l->set_data(d);
    }
}

void light::reflect(reflection &r)
{
    spatial::reflect(r);

    value enum_list = value::make_list();
    enum_list.push("Directional");
    enum_list.push("Point");
    enum_list.push("Spot");
    value type_attrs = value::make_map();
    type_attrs.set_field("enum", enum_list);

    r.add_property("type", light_type_getter, light_type_setter, type_attrs);

    r.add_property("ambient", light_color_getter, light_ambient_setter);
    r.add_property("diffuse", light_diffuse_getter, light_diffuse_setter);
    r.add_property("specular", light_specular_getter, light_specular_setter);

    value spot_attrs = value::make_map();
    spot_attrs.set_field("visible_if", value(is_spot));

    r.add_property(
        "spot_cutoff", light_cutoff_getter, light_cutoff_setter, spot_attrs);
    r.add_property("spot_exponent",
                   light_exponent_getter,
                   light_exponent_setter,
                   spot_attrs);

    value pos_attrs = value::make_map();
    pos_attrs.set_field("visible_if", value(is_positional));

    r.add_property("constant_attenuation",
                   light_constant_getter,
                   light_constant_setter,
                   pos_attrs);
    r.add_property("linear_attenuation",
                   light_linear_getter,
                   light_linear_setter,
                   pos_attrs);
    r.add_property("quadratic_attenuation",
                   light_quadratic_getter,
                   light_quadratic_setter,
                   pos_attrs);
}

light::light() : m_dirty_transform(true)
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

void light::save(serializer &serializer) const
{
    spatial::save(serializer);

    ice_uint8_t type = (int)m_data.type;

    ice_uint16_t ambient  = m_data.ambient.value;
    ice_uint16_t diffuse  = m_data.diffuse.value;
    ice_uint16_t specular = m_data.specular.value;

    ice_real cutoff   = m_data.spot_cutoff;
    ice_real exponent = m_data.spot_exponent;
    ice_real kC       = m_data.constant_attenuation;
    ice_real kL       = m_data.linear_attenuation;
    ice_real kQ       = m_data.quadratic_attenuation;

    serializer.write(type);
    serializer.write(ambient);
    serializer.write(diffuse);
    serializer.write(specular);
    serializer.write(cutoff);
    serializer.write(exponent);
    serializer.write(kC);
    serializer.write(kL);
    serializer.write(kQ);
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

void light::load(serializer &serializer, serializer_link *link)
{
    spatial::load(serializer, link);

    ice_uint8_t type;
    serializer.read(type);
    m_data.type = (light_type)(int)type;

    ice_uint16_t ambient;
    ice_uint16_t diffuse;
    ice_uint16_t specular;
    serializer.read(ambient);
    serializer.read(diffuse);
    serializer.read(specular);

    m_data.ambient.value  = ambient;
    m_data.diffuse.value  = diffuse;
    m_data.specular.value = specular;

    ice_real cutoff;
    ice_real exponent;
    ice_real kC;
    ice_real kL;
    ice_real kQ;
    serializer.read(cutoff);
    serializer.read(exponent);
    serializer.read(kC);
    serializer.read(kL);
    serializer.read(kQ);
    m_data.spot_cutoff           = cutoff;
    m_data.spot_exponent         = exponent;
    m_data.constant_attenuation  = kC;
    m_data.linear_attenuation    = kL;
    m_data.quadratic_attenuation = kQ;

    update_light_transform();
}

} // namespace zabato
