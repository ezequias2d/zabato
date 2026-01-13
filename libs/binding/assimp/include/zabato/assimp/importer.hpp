#pragma once

#include <zabato/importer.hpp>

namespace zabato::assimp
{

class assimp_importer : public importer
{
public:
    string_view name() const override { return "assimp"; }
    bool supports(const string &extension) const override;
    result<shared_ptr<resource>>
    import(fs::file_system &fs,
           const string &path,
           const tinyxml2::XMLElement *settings = nullptr) override;

    bool is_resource_type(const zabato::rtti &type) const override;
};

// Function into initialize the module and register the importer
void register_importer();

} // namespace zabato::assimp
