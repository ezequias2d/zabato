#pragma once

#include <zabato/importer.hpp>

namespace zabato
{

class object_importer : public importer
{
public:
    object_importer(resource_manager *manager) : m_manager(manager) {}

    string_view name() const override { return "Object Importer"; }

    bool supports(const string &extension) const override
    {
        return extension == ".zfile";
    }

    result<shared_ptr<resource>>
    import(class resource_manager &manager,
           const string &path,
           const tinyxml2::XMLElement *settings = nullptr) override;

    bool is_resource_type(const zabato::rtti &type) const override;

private:
    resource_manager *m_manager;
};

void register_object_importer(resource_manager *manager);

} // namespace zabato
