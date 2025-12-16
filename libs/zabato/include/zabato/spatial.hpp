#pragma once

#include "object.hpp"
#include "transformation.hpp"

namespace zabato
{
class spatial : public object
{
public:
    virtual ~spatial();
    spatial *parent();

    transformation &get_local() { return local; }
    transformation &get_world()
    {
        if (is_world_dirty)
        {
            spatial *p = parent();
            if (p)
                world.product(p->get_world(), local);
            else
                world = local;
            is_world_dirty = false;
        }
        return world;
    }

    void set_local(const transformation &local)
    {
        this->local    = local;
        is_world_dirty = true;
    }

    void set_world(const transformation &world)
    {
        this->world    = world;
        is_world_dirty = false;

        spatial *p = parent();
        if (p)
        {
            transformation inv_parent;
            p->get_world().inverse(inv_parent);
            local.product(inv_parent, world);
        }
        else
        {
            local = world;
        }
    }

protected:
    transformation local;
    transformation world;
    bool is_world_dirty;

    spatial();
    spatial *m_parent;

public:
    void set_parent(spatial *parent);
};
} // namespace zabato