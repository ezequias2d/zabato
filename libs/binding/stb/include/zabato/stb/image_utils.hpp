#pragma once

#include <zabato/span.hpp>
#include <zabato/vector.hpp>

namespace zabato::stb
{
enum class comp
{
    y    = 1, //< grayscale
    ya   = 2, //< grayscale + alpha
    rgb  = 3, //< rgb
    rgba = 4  //< rgba
};

/**
 * @brief Writes pixels (RGBA, etc defined by comp) to JPEG in memory.
 *
 * @param pixels pixels to write
 * @param w width of the image
 * @param h height of the image
 * @param comp number of components per pixel
 * @param out_buffer output buffer
 * @param quality quality of the image
 * @return true if successful
 * @return false if failed
 */
bool write_jpg_to_memory(const span<uint8_t> &pixels,
                         int w,
                         int h,
                         comp comp,
                         vector<uint8_t> &out_buffer,
                         int quality = 90);

/**
 * @brief Loads image from memory buffer.
 *
 * @param buffer buffer to load
 * @param w width of the image
 * @param h height of the image
 * @param comp number of components per pixel
 * @param req_comp requested components (0 for auto)
 * @return pixel data
 */
vector<uint8_t> load_image_from_memory(const span<uint8_t> &buffer,
                                       int &w,
                                       int &h,
                                       comp &comp,
                                       int req_comp = 0);

} // namespace zabato::stb
