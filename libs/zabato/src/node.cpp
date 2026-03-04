#include <zabato/node.hpp>
#include <zabato/serializer.hpp>
#include <zabato/world.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{
class world;

const rtti node::TYPE("zabato.node", &spatial::TYPE, node::reflect);

void node::reflect(reflection &r) { spatial::reflect(r); }

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
            serializer.write_object(el, child);
        }
    }
}

bool node::register_object(serializer &serializer) const
{
    if (!object::register_object(serializer))
        return false;

    for (const auto &child : m_children)
        if (child && !child->register_object(serializer))
            return false;

    return true;
}

void node::save(serializer &serializer) const
{
    spatial::save(serializer);

    ice_int32_t child_count = (ice_int32_t)m_children.size();
    serializer.write(child_count);

    for (const auto &child : m_children)
        serializer.write((const object *)child);
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

        object *obj = object::factory(name);
        if (obj)
        {
            obj->load_xml(serializer, *childEl);
            spatial *childSpatial = c_dynamic_cast<spatial>(obj);
            if (childSpatial)
                attach_child(childSpatial);
            else
                delete obj;
        }
    }
}

void node::load(serializer &serializer, serializer_link *link)
{
    spatial::load(serializer, link);

    ice_int32_t child_count = 0;
    size_t readed           = serializer.read(child_count);
    assert(readed == sizeof(child_count));

    const int32_t count = child_count;
    m_children.clear();
    m_children.resize(count);

    for (int32_t i = 0; i < count; i++)
    {
        object *old_child = nullptr;
        size_t readed     = serializer.read(old_child);
        assert(readed == sizeof(void *));

        link->add_child_id(old_child);
    }
}

void node::link(xml_serializer &serializer, tinyxml2::XMLElement &el)
{
    spatial::link(serializer, el);

    tinyxml2::XMLElement *childEl = el.FirstChildElement();
    size_t index                  = 0;
    for (; childEl; childEl = childEl->NextSiblingElement())
    {
        string name = childEl->Name();
        if (name == "transform" || name == "controllers")
            continue;

        if (name != "ref")
        {
            if (index < m_children.size())
            {
                pointer<spatial> child = m_children[index++];
                if (child)
                    child->link(serializer, *childEl);
            }
        }
        else
        {
            const char *id = childEl->Attribute("id");
            if (id)
            {
                uuid uuid_val;
                if (uuid::try_parse({id, strlen(id)}, uuid_val))
                {
                    object *obj           = serializer.get_object(uuid_val);
                    spatial *childSpatial = c_dynamic_cast<spatial>(obj);
                    if (childSpatial)
                        attach_child(childSpatial);
                }
            }
        }
    }
}

void node::link(serializer &serializer, serializer_link *link)
{
    spatial::link(serializer, link);

    for (size_t i = 0; i < m_children.size(); i++)
    {
        object *old_child = link->get_next_child_id();
        m_children[i] =
            c_dynamic_cast<spatial>(serializer.get_from_map(old_child));
    }
}

void node::on_transform_changed()
{
    spatial::on_transform_changed();
    for (auto &child : m_children)
    {
        if (child)
            child->force_dirty();
    }
}

} // namespace zabato