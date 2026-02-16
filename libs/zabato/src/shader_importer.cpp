#include <zabato/shader_asset.hpp>
#include <zabato/shader_importer.hpp>

namespace zabato
{

bool shader_importer::supports(const string &extension) const
{
    return extension == ".zshader";
}

bool shader_importer::is_resource_type(const zabato::rtti &type) const
{
    return shader_asset::TYPE.is_derived(type);
}

result<shared_ptr<resource>>
shader_importer::import(class resource_manager &manager,
                        const string &path,
                        const tinyxml2::XMLElement *settings)
{
    auto fs = manager.get_file_system();
    if (!fs)
        return report_error(error_code::value,
                            "File system not set in resource_manager");

    auto gpu = manager.get_gpu();
    if (!gpu)
        return report_error(error_code::value,
                            "GPU not set in resource_manager");

    auto shader_txt = fs->read_all_text(path);
    if (shader_txt.empty())
        return report_error(error_code::unable_to_match,
                            "Unable to read shader file");

    auto asset = make_shared<shader_asset>();
    asset->set_source(shader_txt);
    return static_pointer_cast<resource>(asset);
}

void shader_importer::register_importer()
{
    importer_registry::register_importer(make_shared<shader_importer>());
}

} // namespace zabato
