#include <zabato/physics/physics_world.hpp>
#include <zabato/reflection.hpp>

namespace zabato::physics
{

const rtti physics_world::TYPE("zabato.physics.physics_world",
                               &base_object::TYPE,
                               physics_world::reflect);

void physics_world::reflect(reflection &r) { base_object::reflect(r); }

} // namespace zabato::physics
