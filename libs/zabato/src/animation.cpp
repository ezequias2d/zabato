#include <zabato/animation.hpp>
#include <zabato/script.hpp>

namespace zabato
{

const rtti
    animation::TYPE("zabato.animation", &resource::TYPE, animation::reflect);

void animation::reflect(reflection &ref) {}
} // namespace zabato
