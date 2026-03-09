#include <tinyxml2.h>
#include <zabato/camera.hpp>
#include <zabato/fs.hpp>
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

bool xml_serializer::save(fs::file_system &fs, const char *path, object *root)
{
    m_doc.Clear();
    m_links.clear();

    if (!root)
        return false;

    tinyxml2::XMLElement *rootEl = m_doc.NewElement(root->type().name());
    m_doc.InsertEndChild(rootEl);

    root->save_xml(*this, *rootEl);

    tinyxml2::XMLPrinter printer;
    m_doc.Accept(&printer);
    string_view xml = printer.CStr();

    auto file = fs.open(path, fs::open_mode::write | fs::open_mode::truncate);
    size_t writted = file->write({(const uint8_t *)xml.data(), xml.size()});
    file->close();
    delete file;

    return writted == xml.size();
}

object *xml_serializer::load(fs::file_system &fs, const char *path)
{
    m_links.clear();

    auto xml = fs.read_all_text(path);
    if (xml.empty())
        return nullptr;

    if (m_doc.Parse(xml.c_str(), xml.size()) != tinyxml2::XML_SUCCESS)
        return nullptr;

    tinyxml2::XMLElement *rootEl = m_doc.RootElement();
    if (!rootEl)
        return nullptr;

    object *obj = object::factory(rootEl->Name());
    obj->load_xml(*this, *rootEl);

    if (!obj)
        return nullptr;

    obj->link(*this, *rootEl);

    return obj;
}

void xml_serializer::write_vec2(tinyxml2::XMLElement &el, const vec2<real> &v)
{
    el.SetAttribute("x", (float)v.x);
    el.SetAttribute("y", (float)v.y);
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

vec2<real> xml_serializer::read_vec2(tinyxml2::XMLElement &el)
{
    vec2<real> v;
    v.x = el.FloatAttribute("x");
    v.y = el.FloatAttribute("y");
    return v;
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
    read_transform_into(el, t);
    return t;
}

void xml_serializer::read_transform_into(tinyxml2::XMLElement &el,
                                         transformation &t)
{
    tinyxml2::XMLElement *transEl = el.FirstChildElement("transform");
    if (transEl)
    {
        tinyxml2::XMLElement *posEl = transEl->FirstChildElement("position");
        if (posEl)
        {
            vec3<real> p = t.translate();
            float f;
            if (posEl->QueryFloatAttribute("x", &f) == tinyxml2::XML_SUCCESS)
                p.x = f;
            if (posEl->QueryFloatAttribute("y", &f) == tinyxml2::XML_SUCCESS)
                p.y = f;
            if (posEl->QueryFloatAttribute("z", &f) == tinyxml2::XML_SUCCESS)
                p.z = f;
            t.set_translate(p);
        }

        tinyxml2::XMLElement *rotEl = transEl->FirstChildElement("rotation");
        if (rotEl)
        {
            quat<real> q = t.rotate();
            vec4<real> v = q.as_vec4;
            float f;
            if (rotEl->QueryFloatAttribute("x", &f) == tinyxml2::XML_SUCCESS)
                v.x = f;
            if (rotEl->QueryFloatAttribute("y", &f) == tinyxml2::XML_SUCCESS)
                v.y = f;
            if (rotEl->QueryFloatAttribute("z", &f) == tinyxml2::XML_SUCCESS)
                v.z = f;
            if (rotEl->QueryFloatAttribute("w", &f) == tinyxml2::XML_SUCCESS)
                v.w = f;
            q.as_vec4 = v;
            t.set_rotate(q);
        }

        tinyxml2::XMLElement *scaleEl = transEl->FirstChildElement("scale");
        if (scaleEl)
        {
            vec3<real> s = t.scale();
            float f;
            if (scaleEl->QueryFloatAttribute("x", &f) == tinyxml2::XML_SUCCESS)
                s.x = f;
            if (scaleEl->QueryFloatAttribute("y", &f) == tinyxml2::XML_SUCCESS)
                s.y = f;
            if (scaleEl->QueryFloatAttribute("z", &f) == tinyxml2::XML_SUCCESS)
                s.z = f;
            t.set_scale(s);
        }
    }
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
        const rtti &type           = obj->type();
        const char *typeName       = type.name();
        tinyxml2::XMLElement *idEl = el.InsertNewChildElement(typeName);
        string sid                 = id.to_string(); // Heap allocation
        idEl->SetAttribute("id", sid.c_str());
        obj->save_xml(*this, *idEl);
    }
}

pointer<object> xml_serializer::read_object(tinyxml2::XMLElement &el)
{
    string type = el.Name();
    if (type == "ref")
    {
        const char *id = el.Attribute("id");
        assert(id);
        if (!id)
            return nullptr;

        uuid uuid;
        if (uuid::try_parse(id, uuid))
            return get_object(uuid);

        return nullptr;
    }
    else
    {
        pointer<object> obj = object::factory(type);
        if (!obj)
            return nullptr;

        obj->load_xml(*this, el);
        return obj;
    }
}

void xml_serializer::write_resource_ref(tinyxml2::XMLElement &el,
                                        const resource_ref &res,
                                        const char *attr)
{
    if (!res.path().empty())
        el.SetAttribute(attr, res.c_path());
}

void xml_serializer::read_resource_ref(tinyxml2::XMLElement &el,
                                       resource_ref &res,
                                       const char *attr)
{
    const char *path = el.Attribute(attr);
    if (path)
        res.set_path(path);
    else
        res.set_path("");
}

pointer<object> xml_serializer::get_object(uuid id)
{
    pointer<object> result = nullptr;
    if (m_links.try_get_value(id, result))
        return result;

    return nullptr;
}

void xml_serializer::add_object(uuid id, object *obj) { m_links.add(id, obj); }

uuid xml_serializer::remap(const uuid &id)
{
    if (id == uuid::null())
        return uuid::null();

    if (!m_remap_ids)
        return id;

    uuid new_id;
    if (m_id_map.try_get_value(id, new_id))
        return new_id;

    new_id = uuid::generate();
    m_id_map.add(id, new_id);
    return new_id;
}

} // namespace zabato
