#pragma once

#include <zabato/color.hpp>
#include <zabato/transformation.hpp>

namespace zabato
{
class gpu;
}

namespace zabato::physics
{
struct shape_config;

void draw_shape_config(gpu &g,
                       const shape_config *shape,
                       const transformation &t,
                       const zabato::color &c);

} // namespace zabato::physics
