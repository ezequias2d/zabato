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

} // namespace zabato
