#include <zabato/color.hpp>
#include <zabato/math.hpp>
#include <zabato/texture_utils.hpp>

namespace zabato
{

static void fill_face(uint16_t size, uint8_t face, vector<uint8_t> &data)
{
    // face: 0=+x, 1=-x, 2=+y, 3=-y, 4=+z, 5=-z
    data.resize(size * size * 4);

    real inv_size = real(1.0f) / real(size);

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            // Map x,y to range [-1, 1]
            // Center of texel logic: (x + 0.5) / size * 2 - 1
            real u =
                (real(x) + real(0.5f)) * inv_size * real(2.0f) - real(1.0f);
            real v =
                (real(y) + real(0.5f)) * inv_size * real(2.0f) - real(1.0f);

            vec3<real> dir;

            switch (face)
            {
            case 0: // +X
                dir = {real(1), -v, -u};
                break;
            case 1: // -X
                dir = {real(-1), -v, u};
                break;
            case 2: // +Y
                dir = {u, real(1), v};
                break;
            case 3: // -Y
                dir = {u, real(-1), -v};
                break;
            case 4: // +Z
                dir = {u, -v, real(1)};
                break;
            case 5: // -Z
                dir = {-u, -v, real(-1)};
                break;
            }

            if (length_sq(dir) > real(0))
                dir = normalize(dir);

            // Pack to 0..255
            // Range [-1, 1] -> [0, 1] -> [0, 255]
            uint8_t r = static_cast<uint8_t>((dir.x + real(1)) * real(0.5f) *
                                             real(255));
            uint8_t g = static_cast<uint8_t>((dir.y + real(1)) * real(0.5f) *
                                             real(255));
            uint8_t b = static_cast<uint8_t>((dir.z + real(1)) * real(0.5f) *
                                             real(255));
            uint8_t a = 255;

            size_t idx    = (y * size + x) * 4;
            data[idx + 0] = r;
            data[idx + 1] = g;
            data[idx + 2] = b;
            data[idx + 3] = a;
        }
    }
}

texture *create_normalization_cubemap(gpu *gpu_ctx, uint16_t size)
{
    if (!gpu_ctx)
        return nullptr;

    texture *tex = gpu_ctx->create_cubemap(size, color_format::rgba8888);

    if (!tex)
        return nullptr;

    vector<uint8_t> buffer;
    for (uint8_t f = 0; f < 6; ++f)
    {
        fill_face(size, f, buffer);
        tex->load(size,
                  size,
                  color_format::rgba8888,
                  buffer.size(),
                  buffer.data(),
                  f);
    }

    return tex;
}

} // namespace zabato
