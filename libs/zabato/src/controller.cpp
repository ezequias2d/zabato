#include <zabato/controller.hpp>
#include <zabato/reflection.hpp>

namespace zabato
{

const rtti
    controller::TYPE("zabato.controller", &object::TYPE, controller::reflect);

void controller::reflect(reflection &r) { object::reflect(r); }

controller::controller() : m_object(nullptr), m_prev(nullptr), m_next(nullptr)
{
}

controller::~controller() {}

void controller::set_object(object *obj) { m_object = obj; }

} // namespace zabato
