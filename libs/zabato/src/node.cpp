#include <zabato/node.hpp>
#include <zabato/world.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{
class world;

const rtti node::TYPE("zabato.node", &spatial::TYPE);

node::node() {}

node::~node() {}

int node::attach_child(spatial *child)
{
    pointer<spatial> ptr(child);
    m_children.push_back(ptr);

    if (child)
    {
        child->set_parent(this);
        auto w = get_world();
        if (w)
            w->register_controllers_recursive(child);
    }
    return m_children.size();
}

int node::detach_child(spatial *child)
{
    auto w = get_world();
    if (w && child)
        w->unregister_controllers_recursive(child);

    pointer<spatial> ptr(child);
    bool found = m_children.remove(ptr);
    if (found && child)
    {
        child->set_parent(nullptr);
    }
    return m_children.size();
}

pointer<spatial> node::detach_child_at(int index)
{
    if (index >= 0 && index < m_children.size())
    {
        pointer<spatial> child = m_children[index];
        auto w                 = get_world();
        if (w && child)
            w->unregister_controllers_recursive(child);
        m_children.remove_at(index);
        if (child)
            child->set_parent(nullptr);
        return child;
    }
    return nullptr;
}

pointer<spatial> node::set_child_at(int index, spatial *child)
{
    if (index >= 0 && index < m_children.size())
    {
        pointer<spatial> old_child = m_children[index];
        auto w                     = get_world();
        if (w && old_child)
            w->unregister_controllers_recursive(old_child);

        m_children[index] = child;
        if (child)
        {
            child->set_parent(this);
            if (w)
            {
                w->register_controllers_recursive(child);
            }
        }

        if (old_child)
            old_child->set_parent(nullptr);

        return old_child;
    }
    return nullptr;
}

void node::save_xml(xml_serializer &serializer, tinyxml2::XMLElement &el) const
{
    spatial::save_xml(serializer, el);

    for (const auto &child : m_children)
    {
        if (child)
        {
            tinyxml2::XMLElement *childEl =
                el.InsertNewChildElement(child->type().name());
            child->save_xml(serializer, *childEl);
        }
    }
}

void node::load_xml(xml_serializer &serializer, tinyxml2::XMLElement &el)
{
    spatial::load_xml(serializer, el);

    tinyxml2::XMLElement *childEl = el.FirstChildElement();
    for (; childEl; childEl = childEl->NextSiblingElement())
    {
        string name = childEl->Name();
        if (name == "transform" || name == "controllers" || name == "ref")
            continue;

        object *obj           = object::factory(serializer, *childEl);
        spatial *childSpatial = c_dynamic_cast<spatial>(obj);
        if (childSpatial)
            attach_child(childSpatial);
        else if (obj)
            delete obj;
    }
}

void node::link(xml_serializer &serializer, tinyxml2::XMLElement &el)
{
    spatial::link(serializer, el);

    tinyxml2::XMLElement *childEl = el.FirstChildElement();

    size_t index = 0;
    for (; childEl; childEl = childEl->NextSiblingElement())
    {
        string name = childEl->Name();
        if (name == "transform" || name == "controllers")
            continue;

        if (name != "ref")
        {
            pointer<spatial> child = m_children[index++];
            if (child)
                child->link(serializer, *childEl);
        }
        else
        {
            const char *id = childEl->Attribute("id");
            if (!id)
                continue;

            uuid uuid;
            bool result = uuid::try_parse({id, strlen(id)}, uuid);
            assert(result && "Invalid UUID");

            object *obj           = serializer.get_object(uuid);
            spatial *childSpatial = c_dynamic_cast<spatial>(obj);
            if (childSpatial)
                attach_child(childSpatial);
            else if (obj)
                delete obj;
        }
    }
}

} // namespace zabato