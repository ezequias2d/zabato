#pragma once

#include <type_traits>

namespace zabato
{

template <typename T> class delegate;

template <typename Ret, typename... Args> class delegate<Ret(Args...)>
{
public:
    typedef Ret (*stub_type)(void *object, Args...);

    delegate() : m_object(nullptr), m_stub(nullptr) {}

    // Constructor for function pointers (explicit)
    delegate(Ret (*ptr)(Args...)) { *this = from_ptr(ptr); }

    // Template constructor for arbitrary callables (functors/lambdas)
    template <
        typename F,
        typename = typename std::enable_if<
            !std::is_same<typename std::decay<F>::type, delegate>::value>::type>
    delegate(F &&f)
    {
        using FunctorType = typename std::decay<F>::type;
        if constexpr (std::is_convertible_v<FunctorType, Ret (*)(Args...)>)
        {
            // Stateless lambda or function pointer -> optimized path
            Ret (*ptr)(Args...) = static_cast<Ret (*)(Args...)>(f);
            *this               = from_ptr(ptr);
        }
        else
        {
            // Stateful lambda / functor -> store reference (CAUTION: lifetime)
            m_object = (void *)&f;
            m_stub   = &functor_stub<typename std::remove_reference<F>::type>;
        }
    }

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
    Ret call(Args... args) const { return m_stub(m_object, args...); }

    bool is_valid() const { return m_stub != nullptr; }
    operator bool() const { return is_valid(); }

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

    template <typename F> static Ret functor_stub(void *object, Args... args)
    {
        return (*static_cast<F *>(object))(args...);
    }
};

} // namespace zabato
