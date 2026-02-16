#pragma once

#include <zabato/importer.hpp>

namespace zabato
{

class material_importer : public importer
{
public:
    string_view name() const override { return "Material Importer"; }

    bool supports(const string &extension) const override
    {
        return extension == ".zmaterial";
    }

    bool is_resource_type(const zabato::rtti &type) const override;

    result<shared_ptr<resource>>
    import(class resource_manager &manager,
           const string &path,
           const tinyxml2::XMLElement *settings = nullptr) override;

    static void register_importer();
};

} // namespace zabato
