#pragma once

#include <zabato/object.hpp>
#include <zabato/resource.hpp>

namespace zabato::editor
{
class object_resource : public resource
{
public:
    object_resource(pointer<object> object) : m_object(object) {}
    ~object_resource() override {}

    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }

    pointer<object> get_object() const { return m_object; }

private:
    pointer<object> m_object = nullptr;
};

} // namespace zabato::editor