#include <cstdint>
#include <zabato/stb/image_utils.hpp>

// Implementation for Write
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Declaration for Load (Implementation is in importer.cpp)
#include "stb_image.h"

namespace zabato::stb
{

void write_func_vector(void *context, void *data, int size)
{
    vector<uint8_t> *vec = (vector<uint8_t> *)context;
    uint8_t *ptr         = (uint8_t *)data;
    vec->insert(vec->end(), ptr, ptr + size);
}

bool write_jpg_to_memory(const span<uint8_t> &pixels,
                         int w,
                         int h,
                         comp comp,
                         vector<uint8_t> &out_buffer,
                         int quality)
{
    assert(comp >= comp::rgba && comp <= comp::rgba);
    out_buffer.clear();
    // 0 on failure, non-0 on success
    int res = stbi_write_jpg_to_func(write_func_vector,
                                     &out_buffer,
                                     w,
                                     h,
                                     (int)comp,
                                     pixels.data(),
                                     quality);
    return res != 0;
}

vector<uint8_t> load_image_from_memory(const span<uint8_t> &buffer,
                                       int &w,
                                       int &h,
                                       comp &c,
                                       int req_comp)
{
    w = 0;
    h = 0;
    c = (comp)0;
    if (buffer.empty())
        return vector<uint8_t>{};

    int x, y, n;
    unsigned char *data = stbi_load_from_memory(
        buffer.data(), (int)buffer.size(), &x, &y, &n, req_comp);

    if (!data)
        return vector<uint8_t>{};

    w = x;
    h = y;
    c = (comp)n;
    assert(c >= comp::y && c <= comp::rgba);

    int channels = (req_comp > 0) ? req_comp : n;
    vector<uint8_t> res;
    res.assign(data, data + (w * h * channels));

    stbi_image_free(data);
    return move(res);
}

} // namespace zabato::stb
