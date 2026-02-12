#pragma once

#include <zabato/mesh.hpp>
#include <zabato/shared_ptr.hpp>

namespace zabato
{

namespace primitives
{

shared_ptr<mesh> create_cube();
shared_ptr<mesh> create_sphere(real radius, int rings, int sectors);
shared_ptr<mesh> create_plane(real size, int divisions);
shared_ptr<mesh> create_quad();

} // namespace primitives

} // namespace zabato
