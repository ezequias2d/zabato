#include <zabato/object.hpp>
#include <zabato/value.hpp>

namespace zabato
{

const rtti ivalue::TYPE("zabato.ivalue", nullptr);
const rtti native_value::TYPE("zabato.native_value", &ivalue::TYPE);

value::value() : impl(make_shared<native_value>()) {}
value::value(bool v) : impl(make_shared<native_value>(v)) {}
value::value(double v) : impl(make_shared<native_value>(v)) {}
value::value(int v) : impl(make_shared<native_value>((int64_t)v)) {}
value::value(int64_t v) : impl(make_shared<native_value>(v)) {}
value::value(const char *v) : impl(make_shared<native_value>(string_view(v))) {}
value::value(string_view v) : impl(make_shared<native_value>(v)) {}
value::value(const string &v) : impl(make_shared<native_value>(string_view(v)))
{
}

value::value(const script_delegate &v) : impl(make_shared<native_value>(v)) {}

value::value(void (*v)(script_system *, script_instance *, script_args *))
    : impl(make_shared<native_value>(script_delegate::from_ptr(v)))
{
}

void value::call(script_system *sys,
                 script_instance *ctx,
                 script_args *args) const
{
    if (impl)
        impl->call(sys, ctx, args);
}

value value::make_map()
{
    auto nv = make_shared<native_value>();
    nv->init_map();
    return value(nv);
}

value value::make_list()
{
    auto nv = make_shared<native_value>();
    nv->init_list();
    return value(nv);
}

native_value::native_value(const pointer<object> &v)
    : m_type(value_type::OBJECT)
{
    o_val = v;
}

native_value::~native_value()
{
    switch (m_type)
    {
    case value_type::STRING:
        delete s_val;
        break;
    case value_type::MAP:
        delete t_val;
        break;
    case value_type::LIST:
        delete a_val;
        break;
    case value_type::OBJECT:
        delete o_val;
        break;
    default:
        break;
    }
}

void *native_value::as_pointer() const
{
    switch (m_type)
    {
    case value_type::POINTER:
        return p_val;
    case value_type::OBJECT:
        return o_val ? (void *)o_val : nullptr;
    default:
        return nullptr;
    }
}

pointer<object> native_value::as_object() const
{
    switch (m_type)
    {
    case value_type::OBJECT:
        return o_val;
    default:
        return pointer<object>(nullptr);
    }
}

bool native_value::operator==(const value &other) const
{
    if (m_type != other.type())
        return false;
    if (m_type == value_type::NIL)
        return true;
    if (m_type == value_type::BOOLEAN)
        return as_bool() == other.as_bool();
    if (m_type == value_type::NUMBER)
        return as_number() == other.as_number();
    if (m_type == value_type::INTEGER)
        return as_int() == other.as_int();
    if (m_type == value_type::STRING)
        return as_string() == other.as_string();
    if (m_type == value_type::MAP)
        return get_type_info().is_exactly(other.impl->get_type_info()) &&
               t_val == static_cast<native_value *>(other.impl.get())->t_val;
    if (m_type == value_type::LIST)
        return get_type_info().is_exactly(other.impl->get_type_info()) &&
               a_val == static_cast<native_value *>(other.impl.get())->a_val;
    if (m_type == value_type::POINTER)
        return p_val == other.as_pointer();
    if (m_type == value_type::OBJECT)
        return as_object() == other.as_object();
    if (m_type == value_type::FUNCTION)
        return as_function() == other.as_function();
    if (m_type == value_type::NATIVE_OBJECT)
        return get_type_info().is_exactly(other.impl->get_type_info());
    return false;
}

pointer<object> value::as_object() const
{
    if (impl)
        return impl->as_object();
    return pointer<object>();
}

} // namespace zabato
