#include <zabato/base_object.hpp>

namespace zabato
{
const rtti
    base_object::TYPE("zabato.base_object", nullptr, base_object::reflect);

void base_object::reflect(reflection &ref) {}

base_object::base_object() : m_uiRefCount(0) {}

base_object::~base_object() {}
} // namespace zabato
