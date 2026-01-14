#pragma once

#include <zabato/color.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/math.hpp>
#include <zabato/object.hpp>
#include <zabato/real.hpp>
#include <zabato/rtti.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/string.hpp>
#include <zabato/vector.hpp>

namespace zabato
{
enum class value_type
{
    NIL,
    BOOLEAN,
    NUMBER,
    INTEGER,
    STRING,
    MAP,
    LIST,
    NATIVE_OBJECT,
    POINTER,
    FUNCTION,
    OBJECT,
    VEC2,
    VEC3,
    VEC4,
    QUAT,
    COLOR,
    MAT3,
    MAT4,
};

class object;
class script_system;
class script_instance;
class script_args;
template <class T> class pointer;
template <typename T> class delegate;

template <typename Ret, typename... Args> class delegate<Ret(Args...)>
{
public:
    typedef Ret (*stub_type)(void *object, Args...);

    delegate() : m_object(nullptr), m_stub(nullptr) {}

    template <class T, Ret (T::*TMethod)(Args...)>
    static delegate from_method(T *object)
    {
        delegate d;
        d.m_object = object;
        d.m_stub   = &method_stub<T, TMethod>;
        return d;
    }

    template <Ret (*TFunc)(Args...)> static delegate from_function()
    {
        delegate d;
        d.m_object = nullptr;
        d.m_stub   = &function_stub<TFunc>;
        return d;
    }

    static delegate from_ptr(Ret (*ptr)(Args...))
    {
        delegate d;
        d.m_object = reinterpret_cast<void *>(ptr);
        d.m_stub   = &ptr_stub;
        return d;
    }

    Ret operator()(Args... args) const { return m_stub(m_object, args...); }

    bool is_valid() const { return m_stub != nullptr; }

    bool operator==(const delegate &other) const
    {
        return m_object == other.m_object && m_stub == other.m_stub;
    }

    bool operator!=(const delegate &other) const { return !(*this == other); }

    void set_object(void *object) { m_object = object; }
    void *get_object() const { return m_object; }

private:
    void *m_object;
    stub_type m_stub;

    template <class T, Ret (T::*TMethod)(Args...)>
    static Ret method_stub(void *object, Args... args)
    {
        T *p = static_cast<T *>(object);
        return (p->*TMethod)(args...);
    }

    template <Ret (*TFunc)(Args...)>
    static Ret function_stub(void *object, Args... args)
    {
        return TFunc(args...);
    }

    static Ret ptr_stub(void *object, Args... args)
    {
        auto p = reinterpret_cast<Ret (*)(Args...)>(object);
        return p(args...);
    }
};

using script_delegate =
    delegate<void(script_system *, script_instance *, script_args *)>;

struct value;

struct iterator
{
    virtual ~iterator()             = default;
    virtual bool next()             = 0;
    virtual value get_key() const   = 0;
    virtual value get_value() const = 0;
};

struct ivalue
{
    static const rtti TYPE;
    virtual const rtti &get_type_info() const { return TYPE; }

    virtual ~ivalue() = default;

    virtual value_type type() const = 0;

    bool is_nil() const { return type() == value_type::NIL; }
    bool is_valid() const { return type() != value_type::NIL; }
    bool is_bool() const { return type() == value_type::BOOLEAN; }
    bool is_number() const { return type() == value_type::NUMBER; }
    bool is_int() const { return type() == value_type::INTEGER; }
    bool is_string() const { return type() == value_type::STRING; }
    bool is_map() const { return type() == value_type::MAP; }
    bool is_list() const { return type() == value_type::LIST; }
    bool is_function() const { return type() == value_type::FUNCTION; }
    bool is_pointer() const { return type() == value_type::POINTER; }
    bool is_native() const { return type() == value_type::NATIVE_OBJECT; }
    bool is_object() const { return type() == value_type::OBJECT; }

    virtual bool is_vec2() const  = 0;
    virtual bool is_vec3() const  = 0;
    virtual bool is_vec4() const  = 0;
    virtual bool is_quat() const  = 0;
    virtual bool is_color() const = 0;
    virtual bool is_mat3() const  = 0;
    virtual bool is_mat4() const  = 0;

    virtual void
    call(script_system *sys, script_instance *ctx, script_args *args) const = 0;

    virtual bool as_bool() const              = 0;
    virtual double as_number() const          = 0;
    virtual int64_t as_int() const            = 0;
    virtual string_view as_string() const     = 0;
    virtual void *as_pointer() const          = 0;
    virtual pointer<object> as_object() const = 0;

    virtual script_delegate as_function() const = 0;

    virtual vec2<real> as_vec2() const = 0;
    virtual vec3<real> as_vec3() const = 0;
    virtual vec4<real> as_vec4() const = 0;
    virtual quat<real> as_quat() const = 0;
    virtual color as_color() const     = 0;
    virtual mat3<real> as_mat3() const = 0;
    virtual mat4<real> as_mat4() const = 0;

#pragma region Map Access
    virtual void set(const zabato::value &key, const zabato::value &val) = 0;
    virtual zabato::value get(const zabato::value &key) const            = 0;
#pragma endregion Map Access

#pragma region String Key Convenience
    virtual void set_field(string_view key, const zabato::value &v) = 0;
    virtual zabato::value get_field(string_view key) const          = 0;
#pragma endregion String Key Convenience

#pragma region List Access
    virtual void push(const zabato::value &v)        = 0;
    virtual zabato::value get_at(size_t index) const = 0;
    virtual size_t length() const                    = 0;
#pragma endregion List Access

#pragma region Iteration
    virtual shared_ptr<iterator> get_iterator() const = 0;
#pragma endregion Iteration

    virtual bool operator==(const zabato::value &other) const = 0;
    virtual bool operator!=(const zabato::value &other) const = 0;
    virtual bool operator<(const zabato::value &other) const  = 0;
    virtual bool operator>(const zabato::value &other) const  = 0;
    virtual bool operator<=(const zabato::value &other) const = 0;
    virtual bool operator>=(const zabato::value &other) const = 0;
};

struct value
{
    shared_ptr<ivalue> impl;

    value();
    value(const shared_ptr<ivalue> &ptr) : impl(ptr) {}
    value(bool v);
    value(double v);
    value(int v);
    value(int64_t v);
    value(const char *v);
    value(string_view v);
    value(const string &v);
    value(const script_delegate &v);
    value(void (*v)(script_system *, script_instance *, script_args *));
    value(object *v);
    value(const pointer<object> &v);
    value(const vec2<real> &v);
    value(const vec3<real> &v);
    value(const vec4<real> &v);
    value(const quat<real> &v);
    value(const color &v);
    value(const mat3<real> &v);
    value(const mat4<real> &v);

    static value make_map();
    static value make_list();

    value_type type() const { return impl ? impl->type() : value_type::NIL; }

    bool is_nil() const { return !impl || impl->is_nil(); }
    bool is_valid() const { return impl && impl->is_valid(); }
    bool is_bool() const { return impl && impl->is_bool(); }
    bool is_number() const { return impl && impl->is_number(); }
    bool is_int() const { return impl && impl->is_int(); }
    bool is_string() const { return impl && impl->is_string(); }
    bool is_map() const { return impl && impl->is_map(); }
    bool is_list() const { return impl && impl->is_list(); }
    bool is_function() const { return impl && impl->is_function(); }
    bool is_pointer() const { return impl && impl->is_pointer(); }
    bool is_native() const { return impl && impl->is_native(); }
    bool is_object() const { return impl && impl->is_object(); }
    bool is_vec2() const { return impl && impl->is_vec2(); }
    bool is_vec3() const { return impl && impl->is_vec3(); }
    bool is_vec4() const { return impl && impl->is_vec4(); }
    bool is_quat() const { return impl && impl->is_quat(); }
    bool is_color() const { return impl && impl->is_color(); }
    bool is_mat3() const { return impl && impl->is_mat3(); }
    bool is_mat4() const { return impl && impl->is_mat4(); }

    bool as_bool() const { return impl ? impl->as_bool() : false; }
    double as_number() const { return impl ? impl->as_number() : 0.0; }
    int64_t as_int() const { return impl ? impl->as_int() : 0; }
    string_view as_string() const { return impl ? impl->as_string() : ""; }
    void *as_pointer() const { return impl ? impl->as_pointer() : nullptr; }
    pointer<object> as_object() const;
    script_delegate as_function() const
    {
        return impl ? impl->as_function() : script_delegate();
    }

    vec2<real> as_vec2() const { return impl ? impl->as_vec2() : vec2<real>(); }
    vec3<real> as_vec3() const { return impl ? impl->as_vec3() : vec3<real>(); }
    vec4<real> as_vec4() const { return impl ? impl->as_vec4() : vec4<real>(); }
    quat<real> as_quat() const { return impl ? impl->as_quat() : quat<real>(); }
    color as_color() const { return impl ? impl->as_color() : color(); }
    mat3<real> as_mat3() const { return impl ? impl->as_mat3() : mat3<real>(); }
    mat4<real> as_mat4() const { return impl ? impl->as_mat4() : mat4<real>(); }

    void set(const value &key, const value &val)
    {
        if (impl)
            impl->set(key, val);
    }

    value get(const value &key) const
    {
        if (impl)
            return impl->get(key);
        return value();
    }

    void set_field(string_view key, const value &v)
    {
        if (impl)
            impl->set_field(key, v);
    }

    value get_field(string_view key) const
    {
        if (impl)
            return impl->get_field(key);
        return value();
    }

    void push(const value &v)
    {
        if (impl)
            impl->push(v);
    }

    value get_at(size_t index) const
    {
        if (impl)
            return impl->get_at(index);
        return value();
    }

    size_t length() const
    {
        if (impl)
            return impl->length();
        return 0;
    }

    shared_ptr<iterator> get_iterator() const
    {
        if (impl)
            return impl->get_iterator();
        return nullptr;
    }

    void
    call(script_system *sys, script_instance *ctx, script_args *args) const;

    bool operator==(const value &other) const
    {
        if (impl)
            return impl->operator==(other);
        return other.is_nil();
    }

    bool operator!=(const value &other) const
    {
        if (impl)
            return impl->operator!=(other);
        return !other.is_nil();
    }

    bool operator<(const value &other) const
    {
        if (impl)
            return impl->operator<(other);
        return other.is_nil();
    }

    bool operator>(const value &other) const
    {
        if (impl)
            return impl->operator>(other);
        return !other.is_nil();
    }

    bool operator<=(const value &other) const
    {
        if (impl)
            return impl->operator<=(other);
        return other.is_nil();
    }

    bool operator>=(const value &other) const
    {
        if (impl)
            return impl->operator>=(other);
        return other.is_nil();
    }
};

#pragma region Native Implementation
class native_value : public ivalue
{
public:
    static const rtti TYPE;
    const rtti &get_type_info() const override { return TYPE; }

    value_type m_type;
    union
    {
        bool b_val;
        double n_val;
        int64_t i_val;
        void *p_val;
        pointer<object> o_val;
        intptr_t ref_id;
        string *s_val;
        hash_map<value, value> *t_val;
        vector<value> *a_val;
        script_delegate func;
        vec2<real> v2_val;
        vec3<real> v3_val;
        vec4<real> v4_val;
        quat<real> q_val;
        color c_val;
        mat3<real> *m3_val;
        mat4<real> *m4_val;
    };

    native_value() : m_type(value_type::NIL), i_val(0) {}
    virtual ~native_value();
    native_value(bool v) : m_type(value_type::BOOLEAN), b_val(v) {}
    native_value(double v) : m_type(value_type::NUMBER), n_val(v) {}
    native_value(int64_t v) : m_type(value_type::INTEGER), i_val(v) {}
    native_value(const string_view &v) : m_type(value_type::STRING)
    {
        s_val = new string(v.data(), v.size());
    }
    native_value(const hash_map<value, value> &v) : m_type(value_type::MAP)
    {
        t_val = new hash_map<value, value>(v);
    }
    native_value(const vector<value> &v) : m_type(value_type::LIST)
    {
        a_val = new vector<value>(v);
    }

    native_value(void *v) : m_type(value_type::POINTER), p_val(v) {}
    native_value(object *v) : m_type(value_type::OBJECT), o_val(v) {}
    native_value(const pointer<object> &v);
    native_value(const script_delegate &v)
        : m_type(value_type::FUNCTION), func(v)
    {
    }
    native_value(const vec2<real> &v) : m_type(value_type::VEC2), v2_val(v) {}
    native_value(const vec3<real> &v) : m_type(value_type::VEC3), v3_val(v) {}
    native_value(const vec4<real> &v) : m_type(value_type::VEC4), v4_val(v) {}
    native_value(const quat<real> &v) : m_type(value_type::QUAT), q_val(v) {}
    native_value(const color &v) : m_type(value_type::COLOR), c_val(v) {}
    native_value(const mat3<real> &v);
    native_value(const mat4<real> &v);

    value_type type() const override { return m_type; }

    bool as_bool() const override
    {
        return (m_type == value_type::BOOLEAN) ? b_val : false;
    }

    double as_number() const override
    {
        return (m_type == value_type::NUMBER)
                   ? n_val
                   : (m_type == value_type::INTEGER ? (double)i_val : 0.0);
    }

    int64_t as_int() const override
    {
        return (m_type == value_type::INTEGER)
                   ? i_val
                   : (m_type == value_type::NUMBER ? (int64_t)n_val : 0);
    }

    string_view as_string() const override
    {
        return (m_type == value_type::STRING) ? string_view(*s_val)
                                              : string_view();
    }

    void *as_pointer() const override;

    pointer<object> as_object() const override;

    script_delegate as_function() const override
    {
        switch (m_type)
        {
        case value_type::FUNCTION:
            return func;
        default:
            return {};
        }
    }

    bool is_vec2() const override;
    bool is_vec3() const override;
    bool is_vec4() const override;
    bool is_quat() const override;
    bool is_color() const override;
    bool is_mat3() const override;
    bool is_mat4() const override;

    vec2<real> as_vec2() const override;
    vec3<real> as_vec3() const override;
    vec4<real> as_vec4() const override;
    quat<real> as_quat() const override;
    color as_color() const override;
    mat3<real> as_mat3() const override;
    mat4<real> as_mat4() const override;

    void call(script_system *sys,
              script_instance *ctx,
              script_args *args) const override
    {
        if (m_type != value_type::FUNCTION)
            return;

        func(sys, ctx, args);
    }

    void set(const value &key, const value &val) override
    {
        switch (m_type)
        {
        case value_type::MAP:
            t_val->add_or_set(key, val);
            break;
        case value_type::LIST:
            if (key.is_int())
                (*a_val)[key.as_int()] = val;
            break;
        default:
            break;
        }
    }

    value get(const value &key) const override
    {
        switch (m_type)
        {
        case value_type::MAP:
        {
            value out;
            if (t_val->try_get_value(key, out))
                return out;
            break;
        }
        case value_type::LIST:
            if (key.is_int())
            {
                size_t idx = key.as_int();
                if (idx < a_val->size())
                    return (*a_val)[idx];
            }
            break;
        default:
            break;
        }
        return value();
    }

    void set_field(string_view key, const value &v) override
    {
        if (m_type == value_type::MAP)
            t_val->add_or_set(string(key), v);
    }

    value get_field(string_view key) const override
    {
        if (m_type == value_type::MAP)
        {
            value out;
            if (t_val->try_get_value(string(key), out))
                return out;
        }
        return value();
    }

    void push(const value &v) override
    {
        if (m_type == value_type::LIST)
            a_val->push_back(v);
    }

    value get_at(size_t index) const override
    {
        if (m_type == value_type::LIST && index < a_val->size())
            return (*a_val)[index];
        return value();
    }

    size_t length() const override
    {
        if (m_type == value_type::LIST)
            return a_val->size();
        if (m_type == value_type::MAP)
            return t_val->size();
        if (m_type == value_type::STRING)
            return s_val->size();
        return 0;
    }

    shared_ptr<iterator> get_iterator() const override;

    void init_map()
    {
        m_type = value_type::MAP;
        t_val  = new hash_map<value, value>();
    }

    void init_list()
    {
        m_type = value_type::LIST;
        a_val  = new vector<value>();
    }

    bool operator==(const value &other) const override;

    bool operator!=(const value &other) const override
    {
        return !operator==(other);
    }

    bool operator<(const value &other) const override
    {
        if (type() != other.type())
            return type() < other.type();
        if (type() == value_type::NIL)
            return false;
        if (type() == value_type::BOOLEAN)
            return as_bool() < other.as_bool();
        if (type() == value_type::NUMBER)
            return as_number() < other.as_number();
        if (type() == value_type::INTEGER)
            return as_int() < other.as_int();
        if (type() == value_type::STRING)
        {
            auto str1 = as_string();
            auto str2 = other.as_string();
            return strncmp(str1.data(), str2.data(), str1.size()) < 0;
        }
        return false;
    }

    bool operator>(const value &other) const override
    {
        if (type() != other.type())
            return type() > other.type();
        if (type() == value_type::NIL)
            return false;
        if (type() == value_type::BOOLEAN)
            return as_bool() > other.as_bool();
        if (type() == value_type::NUMBER)
            return as_number() > other.as_number();
        if (type() == value_type::INTEGER)
            return as_int() > other.as_int();
        if (type() == value_type::STRING)
        {
            auto str1 = as_string();
            auto str2 = other.as_string();
            return strncmp(str1.data(), str2.data(), str1.size()) > 0;
        }
        return false;
    }

    bool operator<=(const value &other) const override
    {
        if (type() != other.type())
            return type() < other.type();
        if (type() == value_type::NIL)
            return true;
        if (type() == value_type::BOOLEAN)
            return as_bool() <= other.as_bool();
        if (type() == value_type::NUMBER)
            return as_number() <= other.as_number();
        if (type() == value_type::INTEGER)
            return as_int() <= other.as_int();
        if (type() == value_type::STRING)
        {
            auto str1 = as_string();
            auto str2 = other.as_string();
            return strncmp(str1.data(), str2.data(), str1.size()) <= 0;
        }
        return false;
    }

    bool operator>=(const value &other) const override
    {
        if (type() != other.type())
            return type() > other.type();
        if (type() == value_type::NIL)
            return true;
        if (type() == value_type::BOOLEAN)
            return as_bool() >= other.as_bool();
        if (type() == value_type::NUMBER)
            return as_number() >= other.as_number();
        if (type() == value_type::INTEGER)
            return as_int() >= other.as_int();
        if (type() == value_type::STRING)
        {
            auto str1 = as_string();
            auto str2 = other.as_string();
            return strncmp(str1.data(), str2.data(), str1.size()) >= 0;
        }
        return false;
    }
};

struct native_map_iterator : public iterator
{
    using map_it = hash_map<zabato::value, zabato::value>::iterator;
    map_it m_it, m_end;

    native_map_iterator(hash_map<zabato::value, zabato::value> &map)
        : m_it(map.begin()), m_end(map.end())
    {
    }

    bool next() override
    {
        if (m_it != m_end)
        {
            ++m_it;
            return m_it != m_end;
        }
        return false;
    }

    zabato::value get_key() const override
    {
        if (m_it != m_end)
            return zabato::value(m_it->key);
        return zabato::value();
    }

    zabato::value get_value() const override
    {
        if (m_it != m_end)
            return m_it->value;
        return zabato::value();
    }
};

struct native_list_iterator : public iterator
{
    using list_it = vector<zabato::value>::const_iterator;
    size_t l_idx;
    const vector<zabato::value> *l_ptr;

    native_list_iterator(const vector<zabato::value> &list)
        : l_idx(0), l_ptr(&list)
    {
    }

    bool next() override
    {
        if (l_idx < l_ptr->size())
        {
            l_idx++;
            return l_idx < l_ptr->size();
        }
        return false;
    }

    zabato::value get_key() const override
    {
        if (l_idx < l_ptr->size())
            return zabato::value((int64_t)l_idx);
        return zabato::value();
    }

    zabato::value get_value() const override
    {
        if (l_idx < l_ptr->size())
            return (*l_ptr)[l_idx];
        return zabato::value();
    }
};

inline shared_ptr<iterator> native_value::get_iterator() const
{
    if (m_type == value_type::MAP)
        return make_shared<native_map_iterator>(
            const_cast<hash_map<zabato::value, zabato::value> &>(*t_val));
    if (m_type == value_type::LIST)
        return make_shared<native_list_iterator>(*a_val);
    return nullptr;
}

} // namespace zabato
