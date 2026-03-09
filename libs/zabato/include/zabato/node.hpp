#pragma once

#include "object.hpp"
#include "spatial.hpp"

namespace zabato
{
class xml_serializer;

class node : public spatial
{
public:
    static const rtti TYPE;
    static void reflect(reflection &r);

    const rtti &type() const override { return TYPE; }

    node();
    virtual ~node();

    virtual void save_xml(xml_serializer &serializer,
                          tinyxml2::XMLElement &element) const override;
    virtual void load_xml(xml_serializer &serializer,
                          tinyxml2::XMLElement &element) override;
    virtual void link(xml_serializer &serializer,
                      tinyxml2::XMLElement &element) override;

    virtual bool register_object(serializer &stream) const override;
    virtual void save(serializer &serializer) const override;
    virtual void load(serializer &serializer, serializer_link *link) override;
    virtual void link(serializer &serializer, serializer_link *link) override;

    virtual void on_transform_changed() override;

    virtual object *get_object_by_name(const symbol_ref &name) const override;

    int quantity() const { return m_children.size(); }
    int attach_child(spatial *child);
    int detach_child(spatial *child);
    pointer<spatial> detach_child_at(int index);
    pointer<spatial> child_at(int index) { return m_children[index]; }
    pointer<spatial> set_child_at(int index, spatial *child);

protected:
    vector<pointer<spatial>> m_children;
};

} // namespace zabato