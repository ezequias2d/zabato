#include <zabato/controller.hpp>

namespace zabato
{

const rtti controller::TYPE("zabato.controller", &object::TYPE);

controller::controller() : m_object(nullptr), m_next(nullptr), m_prev(nullptr)
{
}

controller::~controller() {}

void controller::set_object(object *obj) { m_object = obj; }

} // namespace zabato
