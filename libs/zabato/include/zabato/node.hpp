#pragma once

#include "zabato/object.hpp"
#include "spatial.hpp"

namespace zabato
{
class node : public spatial
{
public:
    node();
    virtual ~node();

    int quantity() const { return m_children.size(); }
    int attach_child(spatial *child)
    {
        pointer<spatial> ptr(child);
        m_children.push_back(ptr);
        return m_children.size();
    }

    int detach_child(spatial *child)
    {
        pointer<spatial> ptr(child);
        m_children.remove(ptr);
        return m_children.size();
    }

    pointer<spatial> detach_child_at(int index)
    {
        pointer<spatial> child = m_children[index];
        m_children.remove_at(index);
        return child;
    }

    pointer<spatial> child_at(int index) { return m_children[index]; }

    pointer<spatial> set_child_at(int index, spatial *child)
    {
        pointer<spatial> old_child = m_children[index];
        m_children[index]          = child;
        return old_child;
    }

protected:
    vector<pointer<spatial>> m_children;
};

} // namespace zabato