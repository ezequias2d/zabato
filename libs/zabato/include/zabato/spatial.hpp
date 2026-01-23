#pragma once

#include "object.hpp"
#include "transformation.hpp"

namespace zabato
{
class world;
class xml_serializer;

class spatial : public object
{
public:
    static const rtti TYPE;
    static void reflect(reflection &r);
    static void get_global_bounds(spatial *root,
                                  vec3<real> &min_pt,
                                  vec3<real> &max_pt,
                                  bool include_fallback_radius = false);

    const rtti &type() const override { return TYPE; }

    virtual ~spatial() {}
    spatial *parent() const { return m_parent; }

    virtual void save_xml(xml_serializer &serializer,
                          tinyxml2::XMLElement &element) const override;
    virtual void load_xml(xml_serializer &serializer,
                          tinyxml2::XMLElement &element) override;
    virtual void link(xml_serializer &serializer,
                      tinyxml2::XMLElement &element) override;

    virtual world *get_world() const override final
    {
        auto p = parent();
        if (p != nullptr)
            return p->get_world();
        return nullptr;
    }

    transformation &get_local() { return local; }
    transformation &get_world_transform();
    const transformation &get_world_transform() const
    {
        return const_cast<spatial *>(this)->get_world_transform();
    }

    void set_local(const transformation &local);

    void set_world(const transformation &world);

    virtual void on_transform_changed() {}

    void force_dirty()
    {
        if (!is_world_dirty)
        {
            is_world_dirty = true;
            on_transform_changed();
        }
    }

protected:
    transformation local;
    transformation world_transform;
    bool is_world_dirty;

    spatial() : m_parent(nullptr), is_world_dirty(false) {}
    spatial *m_parent;

    void set_parent(spatial *parent)
    {
        m_parent       = parent;
        is_world_dirty = true;
        on_transform_changed();
    }

    friend class node;
};
} // namespace zabato