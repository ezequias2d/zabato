#pragma once

#include <zabato/gpu.hpp>
#include <zabato/spatial.hpp>

namespace zabato
{

class light : public spatial
{
public:
    static const rtti TYPE;
    virtual const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    light();
    virtual ~light() = default;

    virtual void save_xml(xml_serializer &serializer,
                          tinyxml2::XMLElement &element) const override;
    virtual void load_xml(xml_serializer &serializer,
                          tinyxml2::XMLElement &element) override;

    virtual void save(serializer &serializer) const override;
    virtual void load(serializer &serializer, serializer_link *link) override;

    virtual void on_transform_changed() override;

    void set_data(const light_data &data);
    const light_data &get_data();

protected:
    bool m_dirty_transform;
    light_data m_data;

    void update_light_transform();
};

} // namespace zabato
