#pragma once

#include <tinyxml2.h>
#include <zabato/math.hpp>
#include <zabato/object.hpp>
#include <zabato/resource.hpp>
#include <zabato/string.hpp>
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

    bool save(fs::file_system &fs, const char *path, object *root);
    object *load(fs::file_system &fs, const char *path);

    tinyxml2::XMLDocument &doc() { return m_doc; }

    static void write_vec2(tinyxml2::XMLElement &el, const vec2<real> &v);
    static void write_vec3(tinyxml2::XMLElement &el, const vec3<real> &v);
    static void write_vec4(tinyxml2::XMLElement &el, const vec4<real> &v);
    static void write_quat(tinyxml2::XMLElement &el, const quat<real> &v);
    static vec2<real> read_vec2(tinyxml2::XMLElement &el);
    static vec3<real> read_vec3(tinyxml2::XMLElement &el);
    static vec4<real> read_vec4(tinyxml2::XMLElement &el);
    static quat<real> read_quat(tinyxml2::XMLElement &el);
    void write_object(tinyxml2::XMLElement &el, object *obj);
    pointer<object> read_object(tinyxml2::XMLElement &el);

    static void write_transform(tinyxml2::XMLElement &el,
                                const transformation &t);
    static transformation read_transform(tinyxml2::XMLElement &el);
    static void read_transform_into(tinyxml2::XMLElement &el,
                                    transformation &t);

    static void write_resource_ref(tinyxml2::XMLElement &el,
                                   const resource_ref &res,
                                   const char *attr = "src");
    static void read_resource_ref(tinyxml2::XMLElement &el,
                                  resource_ref &res,
                                  const char *attr = "src");

    pointer<object> get_object(uuid id);
    void add_object(uuid id, object *obj);

    void set_manager(resource_manager *mgr) { m_manager = mgr; }
    resource_manager *get_manager() const { return m_manager; }

    void set_remap_ids(bool remap) { m_remap_ids = remap; }
    bool should_remap_ids() const { return m_remap_ids; }
    uuid remap(const uuid &id);

private:
    tinyxml2::XMLDocument m_doc;
    hash_map<uuid, pointer<object>> m_links;
    hash_map<uuid, uuid> m_id_map;
    resource_manager *m_manager = nullptr;
    bool m_remap_ids            = false;
};

} // namespace zabato
