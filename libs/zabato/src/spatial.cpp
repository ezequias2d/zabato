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

} // namespace zabato