#pragma once

#include <zabato/gpu.hpp>

namespace zabato
{

/**
 * @brief Creates a normalization cube map.
 *
 * This cube map serves as a lookup table to normalize vectors.
 * Each texel contains the normalized vector from the center of the cube
 * to that texel, encoded as a color (0..1 range representing -1..1).
 *
 * @param gpu_ctx The GPU context to create the texture with.
 * @param size The resolution of each face (usually small, e.g., 32 or 64).
 * @return A new texture instance (caller owns it).
 */
texture *create_normalization_cubemap(gpu *gpu_ctx, uint16_t size = 32);

} // namespace zabato
