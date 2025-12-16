#pragma once

#include <stddef.h>

namespace zabato
{
// Forward declarations
template <typename... Types> class tuple;

// Base case: empty tuple
template <> class tuple<>
{
public:
    tuple() = default;
};

// Specialization for single element
template <typename T0> class tuple<T0>
{
public:
    tuple() = default;
    tuple(const T0 &v0) : _0(v0) {}
    template <typename U0> tuple(U0 &&v0) : _0(static_cast<U0 &&>(v0)) {}

    T0 _0;
};

// Specialization for two elements
template <typename T0, typename T1> class tuple<T0, T1>
{
public:
    constexpr tuple() = default;
    constexpr tuple(const T0 &v0, const T1 &v1) : _0(v0), _1(v1) {}
    template <typename U0, typename U1>
    constexpr tuple(U0 &&v0, U1 &&v1)
        : _0(static_cast<U0 &&>(v0)), _1(static_cast<U1 &&>(v1))
    {
    }

    T0 _0;
    T1 _1;
};

// Specialization for three elements
template <typename T0, typename T1, typename T2> class tuple<T0, T1, T2>
{
public:
    tuple() = default;
    tuple(const T0 &v0, const T1 &v1, const T2 &v2) : _0(v0), _1(v1), _2(v2) {}
    template <typename U0, typename U1, typename U2>
    tuple(U0 &&v0, U1 &&v1, U2 &&v2)
        : _0(static_cast<U0 &&>(v0)), _1(static_cast<U1 &&>(v1)),
          _2(static_cast<U2 &&>(v2))
    {
    }

    T0 _0;
    T1 _1;
    T2 _2;
};

// Specialization for four elements
template <typename T0, typename T1, typename T2, typename T3>
class tuple<T0, T1, T2, T3>
{
public:
    tuple() = default;
    tuple(const T0 &v0, const T1 &v1, const T2 &v2, const T3 &v3)
        : _0(v0), _1(v1), _2(v2), _3(v3)
    {
    }
    template <typename U0, typename U1, typename U2, typename U3>
    tuple(U0 &&v0, U1 &&v1, U2 &&v2, U3 &&v3)
        : _0(static_cast<U0 &&>(v0)), _1(static_cast<U1 &&>(v1)),
          _2(static_cast<U2 &&>(v2)), _3(static_cast<U3 &&>(v3))
    {
    }

    T0 _0;
    T1 _1;
    T2 _2;
    T3 _3;
};

// Helper to get element type at index I
template <size_t I, typename T> struct tuple_element;

template <typename T0> struct tuple_element<0, tuple<T0>>
{
    using type = T0;
};

template <typename T0, typename T1> struct tuple_element<0, tuple<T0, T1>>
{
    using type = T0;
};

template <typename T0, typename T1> struct tuple_element<1, tuple<T0, T1>>
{
    using type = T1;
};

template <typename T0, typename T1, typename T2>
struct tuple_element<0, tuple<T0, T1, T2>>
{
    using type = T0;
};

template <typename T0, typename T1, typename T2>
struct tuple_element<1, tuple<T0, T1, T2>>
{
    using type = T1;
};

template <typename T0, typename T1, typename T2>
struct tuple_element<2, tuple<T0, T1, T2>>
{
    using type = T2;
};

template <typename T0, typename T1, typename T2, typename T3>
struct tuple_element<0, tuple<T0, T1, T2, T3>>
{
    using type = T0;
};

template <typename T0, typename T1, typename T2, typename T3>
struct tuple_element<1, tuple<T0, T1, T2, T3>>
{
    using type = T1;
};

template <typename T0, typename T1, typename T2, typename T3>
struct tuple_element<2, tuple<T0, T1, T2, T3>>
{
    using type = T2;
};

template <typename T0, typename T1, typename T2, typename T3>
struct tuple_element<3, tuple<T0, T1, T2, T3>>
{
    using type = T3;
};

// Convenience alias
template <size_t I, typename T>
using tuple_element_t = typename tuple_element<I, T>::type;

// Get functions
template <size_t I, typename T0> auto &get(tuple<T0> &t)
{
    static_assert(I == 0);
    return t._0;
}

template <size_t I, typename T0, typename T1> auto &get(tuple<T0, T1> &t)
{
    static_assert(I < 2);
    if constexpr (I == 0)
        return t._0;
    else if constexpr (I == 1)
        return t._1;
}

template <size_t I, typename T0, typename T1, typename T2>
auto &get(tuple<T0, T1, T2> &t)
{
    static_assert(I < 3);
    if constexpr (I == 0)
        return t._0;
    else if constexpr (I == 1)
        return t._1;
    else if constexpr (I == 2)
        return t._2;
}

template <size_t I, typename T0, typename T1, typename T2, typename T3>
auto &get(tuple<T0, T1, T2, T3> &t)
{
    static_assert(I < 4);
    if constexpr (I == 0)
        return t._0;
    else if constexpr (I == 1)
        return t._1;
    else if constexpr (I == 2)
        return t._2;
    else if constexpr (I == 3)
        return t._3;
}

// Const versions
template <size_t I, typename T0> const auto &get(const tuple<T0> &t)
{
    static_assert(I == 0);
    return t._0;
}

template <size_t I, typename T0, typename T1>
const auto &get(const tuple<T0, T1> &t)
{
    static_assert(I < 2);
    if constexpr (I == 0)
        return t._0;
    else if constexpr (I == 1)
        return t._1;
}

template <size_t I, typename T0, typename T1, typename T2>
const auto &get(const tuple<T0, T1, T2> &t)
{
    static_assert(I < 3);
    if constexpr (I == 0)
        return t._0;
    else if constexpr (I == 1)
        return t._1;
    else if constexpr (I == 2)
        return t._2;
}

template <size_t I, typename T0, typename T1, typename T2, typename T3>
const auto &get(const tuple<T0, T1, T2, T3> &t)
{
    static_assert(I < 4);
    if constexpr (I == 0)
        return t._0;
    else if constexpr (I == 1)
        return t._1;
    else if constexpr (I == 2)
        return t._2;
    else if constexpr (I == 3)
        return t._3;
}

// Tuple size helper
template <typename T> struct tuple_size;

template <> struct tuple_size<tuple<>>
{
    static constexpr size_t value = 0;
};

template <typename T0> struct tuple_size<tuple<T0>>
{
    static constexpr size_t value = 1;
};

template <typename T0, typename T1> struct tuple_size<tuple<T0, T1>>
{
    static constexpr size_t value = 2;
};

template <typename T0, typename T1, typename T2>
struct tuple_size<tuple<T0, T1, T2>>
{
    static constexpr size_t value = 3;
};

template <typename T0, typename T1, typename T2, typename T3>
struct tuple_size<tuple<T0, T1, T2, T3>>
{
    static constexpr size_t value = 4;
};

// Helper function to make tuples
template <typename... Types> auto make_tuple(Types &&...args)
{
    return tuple<Types...>(static_cast<Types &&>(args)...);
}
} // namespace zabato