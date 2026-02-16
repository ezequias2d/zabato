#pragma once

#include <zabato/camera.hpp>
#include <zabato/math.hpp>
#include <zabato/model.hpp>
#include <zabato/shape.hpp>
#include <zabato/tuple.hpp>
#include <zabato/world.hpp>

namespace zabato
{

/**
 * @brief Generates a ray from screen coordinates.
 * @param cam The camera to unproject from.
 * @param screen_pos The mouse position in screen coordinates (pixels).
 * @param screen_size The size of the viewport/screen (pixels).
 * @return A ray in world space.
 */
ray3<real> get_screen_ray(camera &cam,
                          const vec2<real> &screen_pos,
                          const vec2<real> &screen_size);

/**
 * @brief Picks a model in the world given a ray.
 * @param world The world containing models.
 * @param r The picking ray.
 * @return Pointer to the closest intersected model, or nullptr if none.
 */
tuple<model *, real> pick_object(const world &world, const ray3<real> &r);

} // namespace zabato
