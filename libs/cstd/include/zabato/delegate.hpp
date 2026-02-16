#pragma once

#include <new>
#include <stddef.h>
#include <string.h>
#include <type_traits>
#include <zabato/allocator.hpp>
#include <zabato/utils.hpp>

namespace zabato
{

template <typename T, typename Allocator = allocator<uint8_t>> class delegate;

template <typename Ret, typename... Args, typename Allocator>
class delegate<Ret(Args...), Allocator>
{
public:
    using allocator_type             = Allocator;
    static constexpr size_t sbo_size = 32;

    delegate() : m_object(nullptr), m_stub(nullptr), m_allocator(), m_size(0) {}
    delegate(std::nullptr_t) : delegate() {}

    delegate(const delegate &other)
        : m_allocator(other.m_allocator), m_size(other.m_size)
    {
        if (other.m_stub)
        {
            m_stub    = other.m_stub;
            m_copier  = other.m_copier;
            m_mover   = other.m_mover;
            m_deleter = other.m_deleter;

            if (other.is_using_sbo())
            {
                m_object = (void *)m_storage;
                if (other.m_copier)
                {
                    other.m_copier(m_object, other.m_object, m_allocator);
                }
                else
                {
                    memcpy(m_storage, other.m_storage, sbo_size);
                }
            }
            else
            {
                if (other.m_copier)
                {
                    m_object =
                        other.m_copier(nullptr, other.m_object, m_allocator);
                }
                else if (other.m_deleter)
                {
                    m_object = nullptr;
                    m_stub   = nullptr;
                }
                else
                {
                    m_object = other.m_object;
                }
            }
        }
    }

    delegate(delegate &&other) noexcept
        : m_allocator(zabato::move(other.m_allocator)), m_size(other.m_size)
    {
        move_from(zabato::move(other));
    }

    ~delegate() { destroy(); }

    delegate &operator=(const delegate &other)
    {
        if (this != &other)
        {
            delegate tmp(other);
            swap(tmp);
        }
        return *this;
    }

    delegate &operator=(delegate &&other) noexcept
    {
        if (this != &other)
        {
            destroy();
            move_from(zabato::move(other));
        }
        return *this;
    }

    void swap(delegate &other) noexcept
    {
        bool this_sbo  = is_using_sbo();
        bool other_sbo = other.is_using_sbo();

        char temp_storage[sizeof(delegate)];
        memcpy(temp_storage, this, sizeof(delegate));
        memcpy(this, &other, sizeof(delegate));
        memcpy(&other, temp_storage, sizeof(delegate));

        if (other_sbo)
            m_object = (void *)m_storage;
        if (this_sbo)
            other.m_object = (void *)other.m_storage;
    }

    delegate(Ret (*ptr)(Args...)) { *this = from_ptr(ptr); }

    template <
        typename F,
        typename = typename std::enable_if<
            !std::is_same<typename std::decay<F>::type, delegate>::value &&
            std::is_invocable_r_v<Ret, F, Args...>>::type>
    delegate(F &&f)
    {
        using FunctorType = typename std::decay<F>::type;

        if constexpr (std::is_convertible_v<FunctorType, Ret (*)(Args...)>)
        {
            Ret (*ptr)(Args...) = static_cast<Ret (*)(Args...)>(f);
            *this               = from_ptr(ptr);
        }
        else
        {
            m_size = sizeof(FunctorType);

            if constexpr (sizeof(FunctorType) <= sbo_size &&
                          std::is_nothrow_move_constructible_v<FunctorType>)
            {
                m_object = (void *)m_storage;
                new (m_object) FunctorType(zabato::forward<F>(f));

                m_stub = &functor_stub<FunctorType>;

                if constexpr (std::is_copy_constructible_v<FunctorType>)
                {
                    m_copier = [](void *dest, void *src, Allocator &) -> void *
                    {
                        new (dest)
                            FunctorType(*static_cast<FunctorType *>(src));
                        return dest;
                    };
                }

                m_deleter = [](void *obj, Allocator &)
                { static_cast<FunctorType *>(obj)->~FunctorType(); };

                m_mover = [](void *dest, void *src)
                {
                    new (dest) FunctorType(
                        zabato::move(*static_cast<FunctorType *>(src)));
                    static_cast<FunctorType *>(src)->~FunctorType();
                };
            }
            else
            {
                m_object = m_allocator.allocate(sizeof(FunctorType));
                new (m_object) FunctorType(zabato::forward<F>(f));

                m_stub = &functor_stub<FunctorType>;

                if constexpr (std::is_copy_constructible_v<FunctorType>)
                {
                    m_copier =
                        [](void *dest, void *src, Allocator &alloc) -> void *
                    {
                        void *new_ptr = alloc.allocate(sizeof(FunctorType));
                        new (new_ptr)
                            FunctorType(*static_cast<FunctorType *>(src));
                        return new_ptr;
                    };
                }
                else
                    m_copier = nullptr;

                m_mover = nullptr;

                m_deleter = [](void *obj, Allocator &alloc)
                {
                    static_cast<FunctorType *>(obj)->~FunctorType();
                    alloc.deallocate(static_cast<uint8_t *>(obj),
                                     sizeof(FunctorType));
                };
            }
        }
    }

    template <class T, Ret (T::*TMethod)(Args...)>
    static delegate from_method(T *object)
    {
        delegate d;
        if (object)
        {
            d.m_object = object;
            d.m_stub   = &method_stub<T, TMethod>;
        }
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
        if (ptr)
        {
            d.m_object = reinterpret_cast<void *>(ptr);
            d.m_stub   = &ptr_stub;
        }
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
    void *m_object     = nullptr;
    using stub_type    = Ret (*)(void *, Args...);
    using copier_type  = void *(*)(void *, void *, Allocator &);
    using deleter_type = void (*)(void *, Allocator &);
    using mover_type   = void (*)(void *, void *);

    stub_type m_stub       = nullptr;
    copier_type m_copier   = nullptr;
    deleter_type m_deleter = nullptr;
    mover_type m_mover     = nullptr;

    Allocator m_allocator;
    size_t m_size;

    alignas(16) uint8_t m_storage[sbo_size];

    bool is_using_sbo() const { return m_object == (void *)m_storage; }

    void destroy()
    {
        if (m_object && m_deleter)
        {
            m_deleter(m_object, m_allocator);
        }
        m_object = nullptr;
        m_stub   = nullptr;
        m_size   = 0;
    }

    void move_from(delegate &&other)
    {
        if (other.m_stub)
        {
            m_stub    = other.m_stub;
            m_copier  = other.m_copier;
            m_mover   = other.m_mover;
            m_deleter = other.m_deleter;

            if (other.is_using_sbo())
            {
                m_object = (void *)m_storage;
                if (m_mover)
                {
                    m_mover(m_object, other.m_object);
                }
                else
                {
                    memcpy(m_storage, other.m_storage, sbo_size);
                }
            }
            else
            {
                m_object = other.m_object;
            }

            other.m_object  = nullptr;
            other.m_stub    = nullptr;
            other.m_size    = 0;
            other.m_deleter = nullptr;
        }
        else
        {
            m_object = nullptr;
            m_stub   = nullptr;
        }
    }

    template <class T, Ret (T::*TMethod)(Args...)>
    static Ret method_stub(void *object, Args... args)
    {
        T *p = static_cast<T *>(object);
        return (p->*TMethod)(args...);
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

    template <Ret (*TFunc)(Args...)>
    static Ret function_stub(void *, Args... args)
    {
        return TFunc(args...);
    }
};

} // namespace zabato
