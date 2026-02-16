#include <zabato/error.hpp>
#include <zabato/gpu.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/stb/importer.hpp>

// STB Image implementation
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace zabato::stb
{

void register_importer()
{
    importer_registry::register_importer(make_shared<stb_importer>());
}

bool stb_importer::supports(const string &extension) const
{
    static const vector<string> supported_exts = {".png",
                                                  ".jpg",
                                                  ".jpeg",
                                                  ".bmp",
                                                  ".tga",
                                                  ".psd",
                                                  ".gif",
                                                  ".hdr",
                                                  ".pic"};

    for (const auto &ext : supported_exts)
    {
        if (extension == ext)
            return true;
    }
    return false;
}

bool stb_importer::is_resource_type(const zabato::rtti &type) const
{
    return type.is_exactly(zabato::texture::TYPE);
}

result<shared_ptr<resource>>
stb_importer::import(class resource_manager &manager,
                     const string &path,
                     const tinyxml2::XMLElement *settings)
{
    report(report_type::info, "Importing texture: %s", path.c_str());

    auto gpu_ctx = manager.get_gpu();
    if (!gpu_ctx)
        return report_error(error_code::value,
                            "GPU context required for texture import");

    auto fs = manager.get_file_system();
    if (!fs)
        return report_error(error_code::value,
                            "File system not set in resource_manager");

    if (!fs->exists(path))
        return report_error(error_code::file_not_found, path.c_str());

    vector<uint8_t> buf = fs->read_all_bytes(path);
    if (buf.empty())
        return report_error(error_code::unable_to_read, path.c_str());

    int width, height, channels;
    // Force 4 channels (RGBA)
    stbi_uc *pixels = stbi_load_from_memory(
        buf.data(), (int)buf.size(), &width, &height, &channels, 4);

    if (!pixels)
        return report_error(error_code::unable_to_read, stbi_failure_reason());

    // Create GPU Texture
    zabato::texture *tex = gpu_ctx->create_texture(
        (uint16_t)width, (uint16_t)height, color_format::rgba8888);

    if (!tex)
    {
        stbi_image_free(pixels);
        return report_error(error_code::value, "Failed to create GPU texture");
    }

    tex->load((uint16_t)width,
              (uint16_t)height,
              color_format::rgba8888,
              width * height * 4,
              pixels);

    stbi_image_free(pixels);

    return result<shared_ptr<resource>>(shared_ptr<resource>(tex));
}

} // namespace zabato::stb
