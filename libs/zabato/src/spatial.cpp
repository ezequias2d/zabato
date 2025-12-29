#include <string.h>
#include <tinyxml2.h>

#include <zabato/spatial.hpp>
#include <zabato/string.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti spatial::TYPE("zabato::spatial", &object::TYPE);

void spatial::save_xml(xml_serializer &serializer,
                       tinyxml2::XMLElement &element) const
{
    object::save_xml(serializer, element);
    xml_serializer::write_transform(element, local);
}

void spatial::load_xml(xml_serializer &serializer,
                       tinyxml2::XMLElement &element)
{
    object::load_xml(serializer, element);
    local = xml_serializer::read_transform(element);
}

void spatial::link(xml_serializer &serializer, tinyxml2::XMLElement &element)
{
    object::link(serializer, element);

    tinyxml2::XMLNode *parent = element.Parent();
    if (parent)
    {
        tinyxml2::XMLElement *parentEl = parent->ToElement();
        if (parentEl)
        {
            const char *id = parentEl->Attribute("id");
            if (id)
            {
                uuid parentId;
                bool result = uuid::try_parse({id, strlen(id)}, parentId);
                assert(result && "Invalid parent id");

                object *parentObj = serializer.get_object(parentId);
                spatial *parent   = c_dynamic_cast<spatial>(parentObj);
                assert(parent && "Invalid parent");
                set_parent(parent);
            }
        }
    }
}

transformation &spatial::get_world_transform()
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

void spatial::set_local(const transformation &local)
{
    this->local = local;
    force_dirty();
}

void spatial::set_world(const transformation &world)
{
    this->world_transform = world;
    force_dirty();

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

} // namespace zabato