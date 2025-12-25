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
    transformation &get_world_transform()
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

    void set_local(const transformation &local)
    {
        this->local    = local;
        is_world_dirty = true;
    }

    void set_world(const transformation &world)
    {
        this->world_transform = world;
        is_world_dirty        = false;

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

protected:
    transformation local;
    transformation world_transform;
    bool is_world_dirty;

    spatial() : m_parent(nullptr), is_world_dirty(false) {}
    spatial *m_parent;

public:
    void set_parent(spatial *parent)
    {
        m_parent       = parent;
        is_world_dirty = true;
    }
};
} // namespace zabato