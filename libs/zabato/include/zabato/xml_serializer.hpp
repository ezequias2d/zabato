#pragma once

#include <tinyxml2.h>
#include <zabato/math.hpp>
#include <zabato/object.hpp>
#include <zabato/resource.hpp>
#include <zabato/transformation.hpp>
#include <zabato/uuid.hpp>
#include <zabato/vector.hpp>

namespace zabato
{
class serializer_link;
class object;

class xml_serializer
{
public:
    xml_serializer();
    ~xml_serializer();

    bool save(const char *path, object *root);
    object *load(const char *path);

    tinyxml2::XMLDocument &doc() { return m_doc; }

    static void write_vec3(tinyxml2::XMLElement &el, const vec3<real> &v);
    static void write_vec4(tinyxml2::XMLElement &el, const vec4<real> &v);
    static void write_quat(tinyxml2::XMLElement &el, const quat<real> &v);
    static vec3<real> read_vec3(tinyxml2::XMLElement &el);
    static vec4<real> read_vec4(tinyxml2::XMLElement &el);
    static quat<real> read_quat(tinyxml2::XMLElement &el);
    void write_object(tinyxml2::XMLElement &el, object *obj);
    object *read_object(tinyxml2::XMLElement &el);

    static void write_transform(tinyxml2::XMLElement &el,
                                const transformation &t);
    static transformation read_transform(tinyxml2::XMLElement &el);

    static void write_resource_ref(tinyxml2::XMLElement &el,
                                   const resource_ref &res);
    static void read_resource_ref(tinyxml2::XMLElement &el, resource_ref &res);

    object *get_object(uuid id)
    {
        object *result = nullptr;
        if (m_links.try_get_value(id, result))
            return result;
        return nullptr;
    }

private:
    tinyxml2::XMLDocument m_doc;
    hash_map<uuid, object *> m_links;
};

} // namespace zabato
