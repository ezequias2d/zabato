#pragma once

#include <zabato/importer.hpp>

namespace zabato
{

class shader_importer : public importer
{
public:
    string_view name() const override { return "shader_importer"; }
    bool supports(const string &extension) const override;

    result<shared_ptr<resource>>
    import(class resource_manager &manager,
           const string &path,
           const tinyxml2::XMLElement *settings = nullptr) override;

    bool is_resource_type(const zabato::rtti &type) const override;

    static void register_importer();
};

} // namespace zabato
