#include <tinyxml2.h>
#include <zabato/camera.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/model.hpp>
#include <zabato/node.hpp>
#include <zabato/spatial.hpp>
#include <zabato/world.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

xml_serializer::xml_serializer() {}

xml_serializer::~xml_serializer() {}

bool xml_serializer::save(const char *path, object *root)
{
    m_doc.Clear();
    m_links.clear();

    if (!root)
        return false;

    tinyxml2::XMLElement *rootEl = m_doc.NewElement(root->type().name());
    m_doc.InsertEndChild(rootEl);

    root->save_xml(*this, *rootEl);

    return m_doc.SaveFile(path) == tinyxml2::XML_SUCCESS;
}

object *xml_serializer::load(const char *path)
{
    m_links.clear();

    if (m_doc.LoadFile(path) != tinyxml2::XML_SUCCESS)
    {
        return nullptr;
    }

    tinyxml2::XMLElement *rootEl = m_doc.RootElement();
    if (!rootEl)
        return nullptr;

    object *obj = object::factory(*this, *rootEl);
    if (!obj)
        return nullptr;

    obj->load_xml(*this, *rootEl);
    obj->link(*this, *rootEl);

    return obj;
}

void xml_serializer::write_vec3(tinyxml2::XMLElement &el, const vec3<real> &v)
{
    el.SetAttribute("x", (float)v.x);
    el.SetAttribute("y", (float)v.y);
    el.SetAttribute("z", (float)v.z);
}

void xml_serializer::write_vec4(tinyxml2::XMLElement &el, const vec4<real> &v)
{
    el.SetAttribute("x", (float)v.x);
    el.SetAttribute("y", (float)v.y);
    el.SetAttribute("z", (float)v.z);
    el.SetAttribute("w", (float)v.w);
}

void xml_serializer::write_quat(tinyxml2::XMLElement &el, const quat<real> &v)
{
    write_vec4(el, v.as_vec4);
}

vec3<real> xml_serializer::read_vec3(tinyxml2::XMLElement &el)
{
    vec3<real> v;
    v.x = el.FloatAttribute("x");
    v.y = el.FloatAttribute("y");
    v.z = el.FloatAttribute("z");
    return v;
}

vec4<real> xml_serializer::read_vec4(tinyxml2::XMLElement &el)
{
    vec4<real> v;
    v.x = el.FloatAttribute("x");
    v.y = el.FloatAttribute("y");
    v.z = el.FloatAttribute("z");
    v.w = el.FloatAttribute("w");
    return v;
}

quat<real> xml_serializer::read_quat(tinyxml2::XMLElement &el)
{
    quat<real> q;
    q.as_vec4 = read_vec4(el);
    return q;
}

void xml_serializer::write_transform(tinyxml2::XMLElement &el,
                                     const transformation &t)
{
    tinyxml2::XMLElement *transEl = el.InsertNewChildElement("transform");

    tinyxml2::XMLElement *posEl = transEl->InsertNewChildElement("position");
    write_vec3(*posEl, t.translate());

    tinyxml2::XMLElement *rotEl = transEl->InsertNewChildElement("rotation");
    write_quat(*rotEl, t.rotate());

    tinyxml2::XMLElement *scaleEl = transEl->InsertNewChildElement("scale");
    write_vec3(*scaleEl, t.scale());
}

transformation xml_serializer::read_transform(tinyxml2::XMLElement &el)
{
    transformation t;
    tinyxml2::XMLElement *transEl = el.FirstChildElement("transform");
    if (transEl)
    {
        tinyxml2::XMLElement *posEl = transEl->FirstChildElement("position");
        if (posEl)
            t.set_translate(read_vec3(*posEl));

        tinyxml2::XMLElement *rotEl = transEl->FirstChildElement("rotation");
        if (rotEl)
            t.set_rotate(read_quat(*rotEl));

        tinyxml2::XMLElement *scaleEl = transEl->FirstChildElement("scale");
        if (scaleEl)
            t.set_scale(read_vec3(*scaleEl));
    }
    return t;
}

void xml_serializer::write_object(tinyxml2::XMLElement &el, object *obj)
{
    uuid id = obj->id();
    if (m_links.contains_key(id))
    {
        tinyxml2::XMLElement *idEl = el.InsertNewChildElement("ref");
        char sid[37];
        id.to_chars(sid);
        idEl->SetAttribute("id", sid);
    }
    else
    {
        rtti type                  = obj->type();
        tinyxml2::XMLElement *idEl = el.InsertNewChildElement(type.name());
        char sid[37];
        id.to_chars(sid);
        idEl->SetAttribute("id", sid);

        obj->save_xml(*this, *idEl);
    }
}

object *xml_serializer::read_object(tinyxml2::XMLElement &el)
{
    return nullptr;
}

void xml_serializer::write_resource_ref(tinyxml2::XMLElement &el,
                                        const resource_ref &res)
{
    if (!res.path().empty())
        el.SetAttribute("src", res.c_path());
}

void xml_serializer::read_resource_ref(tinyxml2::XMLElement &el,
                                       resource_ref &res)
{
    const char *path = el.Attribute("src");
    if (path)
        res.set_path(path);
}

} // namespace zabato
