#pragma once

#include <zabato/error.hpp>
#include <zabato/gpu.hpp>
#include <zabato/hash.hpp>
#include <zabato/ice.hpp>

namespace zabato
{
template <typename T> result<void> serialize(ice_writer &writer, const T &obj);
template <typename T> result<void> deserialize(ice_reader &reader, T &obj);

/** @brief Header for a texture chunk in an ICE file. */
struct ICE_TEXTURE_HEADER
{
    ice_uint8_t pearson_hash;
    ice_uint8_t color_format_raw;
    ice_uint16_t width;
    ice_uint16_t height;

    color_format get_format() const
    {
        return static_cast<color_format>((uint8_t)color_format_raw);
    }
};

template <> inline result<void> serialize(ice_writer &writer, const texture &t)
{
    TRACE_FUNCTION;

    uint16_t width = 0, height = 0;
    color_format format    = color_format::undefined;
    size_t pixel_data_size = 0;

    // First call to get the required size
    t.copy(&width, &height, &format, nullptr, &pixel_data_size);
    if (pixel_data_size == 0 || width == 0 || height == 0)
        return report_error(error_code::value);

    vector<uint8_t> pixel_data;
    pixel_data.resize(pixel_data_size);

    // Second call to get the actual data
    t.copy(nullptr, nullptr, nullptr, pixel_data.data(), &pixel_data_size);

    // Calculate Pearson hash of pixel data
    uint8_t hash = pearson_hash_array(0, pixel_data.data(), pixel_data_size);
    printf("hash: %d\n", (int)hash);

    ICE_TEXTURE_HEADER header = {
        hash,
        static_cast<uint8_t>(format),
        width,
        height,
    };

    auto chunk_res = writer.write_chunk_header(
        texture::CHUNK_ID, sizeof(header) + pixel_data_size);
    if (chunk_res.has_error())
        return chunk_res.error;

    auto header_res = writer.write(&header, sizeof(header));
    if (header_res.has_error())
        return header_res.error;

    auto data_res = writer.write(pixel_data.data(), pixel_data_size);
    if (data_res.has_error())
        return data_res.error;

    return error_code::ok;
}

/**
 * @brief Finds and loads a texture from an ICE stream.
 * @param reader The ice_reader to use.
 * @param gpu_api The GPU instance to create the texture with.
 * @param tex_to_reuse An optional existing texture to reload data into. If
 * null, a new texture is created.
 * @return A pointer to the loaded texture, or nullptr on failure.
 */
template <> inline result<void> deserialize(ice_reader &reader, texture &t)
{
    auto [error, chunk] = reader.find_chunk(texture::CHUNK_ID);
    if (error)
    {
        printf("Error finding texture chunk: %d\n", (int)error);
        return error;
    }

    ICE_TEXTURE_HEADER tex_header;
    reader.read(&tex_header, sizeof(tex_header));

    printf("tex_header.format: %d\n", (int)tex_header.get_format());
    printf("chunk.id: %s\n", chunk.id.to_string().c_str());
    printf("chunk.size: %d\n", (int)chunk.size);

    printf("tex_header.pearson_hash: %d\n", (int)tex_header.pearson_hash);
    printf("tex_header.width: %d\n", (int)tex_header.width);
    printf("tex_header.height: %d\n", (int)tex_header.height);

    vector<uint8_t> pixel_data(chunk.size - sizeof(tex_header));
    printf("pixel_data.size(): %d\n", (int)pixel_data.size());

    reader.read(pixel_data.data(), pixel_data.size());
    uint8_t computed_hash =
        pearson_hash_array(0, pixel_data.data(), pixel_data.size());

    printf("computed_hash: %d\n", (int)computed_hash);

    if (computed_hash != tex_header.pearson_hash)
        return report_error(error_code::chunk_broken,
                            texture::CHUNK_ID.to_string().c_str(),
                            (uint32_t)texture::CHUNK_ID);

    t.load(tex_header.width,
           tex_header.height,
           tex_header.get_format(),
           pixel_data.size(),
           pixel_data.data());
    return error_code::ok;
}
} // namespace zabato