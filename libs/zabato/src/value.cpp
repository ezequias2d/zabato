#include <zabato/script.hpp>
#include <zabato/value.hpp>

namespace zabato
{
const rtti ivalue::TYPE("zabato.ivalue", nullptr);
const rtti native_value::TYPE("zabato.native_value", &ivalue::TYPE);

value::value() {}
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
    : impl(make_shared<native_value>(
          delegate<void(script_system *, script_instance *, script_args *)>::
              from_ptr(v)))
{
}
value::value(object *v)
{
    if (v)
        impl = make_shared<native_value>(v);
}
value::value(const pointer<object> &v)
{
    if (v)
        impl = make_shared<native_value>(v);
}
value::value(const vec2<real> &v) : impl(make_shared<native_value>(v)) {}
value::value(const vec3<real> &v) : impl(make_shared<native_value>(v)) {}
value::value(const vec4<real> &v) : impl(make_shared<native_value>(v)) {}
value::value(const quat<real> &v) : impl(make_shared<native_value>(v)) {}
value::value(const color &v) : impl(make_shared<native_value>(v)) {}
value::value(const mat3<real> &v) : impl(make_shared<native_value>(v)) {}
value::value(const mat4<real> &v) : impl(make_shared<native_value>(v)) {}

value value::make_map()
{
    auto v = make_shared<native_value>();
    v->init_map();
    return value(v);
}
value value::make_list()
{
    auto v = make_shared<native_value>();
    v->init_list();
    return value(v);
}

void value::call(script_system *sys,
                 script_instance *ctx,
                 script_args *args) const
{
    if (impl)
        impl->call(sys, ctx, args);
}

pointer<object> value::as_object() const
{
    return impl ? impl->as_object() : nullptr;
}

native_value::native_value(const pointer<object> &v)
    : m_type(value_type::OBJECT)
{
    new (&o_val) pointer<object>(v);
}

native_value::native_value(const mat3<real> &v) : m_type(value_type::MAT3)
{
    m3_val = new mat3<real>(v);
}

native_value::native_value(const mat4<real> &v) : m_type(value_type::MAT4)
{
    m4_val = new mat4<real>(v);
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
        o_val.~pointer<object>();
        break;
    case value_type::MAT3:
        delete m3_val;
        break;
    case value_type::MAT4:
        delete m4_val;
        break;
    default:
        break;
    }
}

void *native_value::as_pointer() const
{
    return (m_type == value_type::POINTER) ? p_val : nullptr;
}

pointer<object> native_value::as_object() const
{
    return (m_type == value_type::OBJECT) ? o_val : nullptr;
}

// Implicit Conversion Helpers
static bool is_list_of_numbers(const native_value *val, size_t count)
{
    if (val->m_type != value_type::LIST)
        return false;
    if (val->a_val->size() != count)
        return false;
    for (size_t i = 0; i < count; ++i)
    {
        if (!(*val->a_val)[i].is_number())
            return false;
    }
    return true;
}

bool native_value::is_vec2() const
{
    if (m_type == value_type::VEC2)
        return true;
    return is_list_of_numbers(this, 2);
}

bool native_value::is_vec3() const
{
    if (m_type == value_type::VEC3)
        return true;
    return is_list_of_numbers(this, 3);
}

bool native_value::is_vec4() const
{
    if (m_type == value_type::VEC4)
        return true;
    return is_list_of_numbers(this, 4);
}

bool native_value::is_quat() const
{
    if (m_type == value_type::QUAT)
        return true;
    // Quaternions are also just 4 numbers
    return is_list_of_numbers(this, 4);
}

bool native_value::is_color() const
{
    if (m_type == value_type::COLOR)
        return true;
    // Colors are also just 4 numbers
    return is_list_of_numbers(this, 4);
}

bool native_value::is_mat3() const
{
    if (m_type == value_type::MAT3)
        return true;
    return is_list_of_numbers(this, 9);
}

bool native_value::is_mat4() const
{
    if (m_type == value_type::MAT4)
        return true;
    return is_list_of_numbers(this, 16);
}

vec2<real> native_value::as_vec2() const
{
    if (m_type == value_type::VEC2)
        return v2_val;
    if (m_type == value_type::LIST && a_val->size() >= 2)
    {
        return vec2<real>((real)(*a_val)[0].as_number(),
                          (real)(*a_val)[1].as_number());
    }
    return vec2<real>();
}

vec3<real> native_value::as_vec3() const
{
    if (m_type == value_type::VEC3)
        return v3_val;
    if (m_type == value_type::LIST && a_val->size() >= 3)
    {
        return vec3<real>((real)(*a_val)[0].as_number(),
                          (real)(*a_val)[1].as_number(),
                          (real)(*a_val)[2].as_number());
    }
    return vec3<real>();
}

vec4<real> native_value::as_vec4() const
{
    if (m_type == value_type::VEC4)
        return v4_val;
    if (m_type == value_type::LIST && a_val->size() >= 4)
    {
        return vec4<real>((real)(*a_val)[0].as_number(),
                          (real)(*a_val)[1].as_number(),
                          (real)(*a_val)[2].as_number(),
                          (real)(*a_val)[3].as_number());
    }
    return vec4<real>();
}

quat<real> native_value::as_quat() const
{
    if (m_type == value_type::QUAT)
        return q_val;
    if (m_type == value_type::LIST && a_val->size() >= 4)
    {
        return quat<real>((real)(*a_val)[0].as_number(),
                          (real)(*a_val)[1].as_number(),
                          (real)(*a_val)[2].as_number(),
                          (real)(*a_val)[3].as_number());
    }
    return quat<real>();
}

color native_value::as_color() const
{
    if (m_type == value_type::COLOR)
        return c_val;
    if (m_type == value_type::LIST && a_val->size() >= 4)
    {
        return color((real)(*a_val)[0].as_number(),
                     (real)(*a_val)[1].as_number(),
                     (real)(*a_val)[2].as_number(),
                     (real)(*a_val)[3].as_number());
    }
    return color();
}

mat3<real> native_value::as_mat3() const
{
    if (m_type == value_type::MAT3)
        return *m3_val;
    // TODO: List conversion for matrices not prioritized for now, but simple to
    // add if needed
    return mat3<real>();
}

mat4<real> native_value::as_mat4() const
{
    if (m_type == value_type::MAT4)
        return *m4_val;
    return mat4<real>();
}

bool native_value::operator==(const value &other) const
{
    if (m_type != other.type())
        return false;

    switch (m_type)
    {
    case value_type::NIL:
        return true;
    case value_type::BOOLEAN:
        return b_val == other.as_bool();
    case value_type::NUMBER:
    {
        // Simple comparison for now, maybe epsilon later
        return n_val == other.as_number();
    }
    case value_type::INTEGER:
        return i_val == other.as_int();
    case value_type::STRING:
        return *s_val == other.as_string();
    case value_type::POINTER:
        return p_val == other.as_pointer();
    case value_type::OBJECT:
        return o_val == other.as_object();
    case value_type::FUNCTION:
        // Function comparison is tricky, usually pointer comparison
        return func == other.as_function();
    case value_type::VEC2:
        return v2_val == other.as_vec2();
    case value_type::VEC3:
        return v3_val == other.as_vec3();
    case value_type::VEC4:
        return v4_val == other.as_vec4();
    case value_type::QUAT:
        return q_val == other.as_quat();
    case value_type::COLOR:
        return c_val == other.as_color();
    case value_type::MAT3:
        return *m3_val == other.as_mat3();
    case value_type::MAT4:
        return *m4_val == other.as_mat4();

    case value_type::MAP:
    case value_type::LIST:
        // Deep comparison? By default no, pointer comparison usually.
        // But for completeness let's just return false for now or use pointer
        // identity if we had logic for it.
        // The original code didn't implement robust container comparison.
        // For 'value' wrappers, they compare impl pointers.
        // But here we are comparing *this (native_value) to another value.
        // This suggests we are comparing values.
        // Since maps/lists are reference types in this engine, check if it's
        // same object. But 'other' is value struct.
        // Let's assume false for complex types for now unless we do deep
        // compare
        return false;

    default:
        return false;
    }
}

} // namespace zabato
