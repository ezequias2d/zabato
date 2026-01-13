#pragma once

#include "zabato/string.hpp"
#include <zabato/importer.hpp>
#include <zabato/rtti.hpp>

namespace zabato::stb
{

class stb_importer : public importer
{
public:
    virtual ~stb_importer() = default;

    string_view name() const override { return "stb"; }
    bool supports(const string &extension) const override;
    result<shared_ptr<resource>>
    import(fs::file_system &fs,
           class gpu *gpu_ctx,
           const string &path,
           const tinyxml2::XMLElement *settings = nullptr) override;

    bool is_resource_type(const zabato::rtti &type) const override;
};

void register_importer();

} // namespace zabato::stb
