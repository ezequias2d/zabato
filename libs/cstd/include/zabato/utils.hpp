#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace zabato
{
template <typename T> inline const T &min(const T &a, const T &b)
{
    return (a < b) ? a : b;
}

template <typename T> inline const T &max(const T &a, const T &b)
{
    return (a > b) ? a : b;
}

template <typename T> struct remove_reference
{
    using type = T;
};
template <typename T> struct remove_reference<T &>
{
    using type = T;
};
template <typename T> struct remove_reference<T &&>
{
    using type = T;
};

template <typename T>
typename remove_reference<T>::type &&move(T &&arg) noexcept
{
    return static_cast<typename remove_reference<T>::type &&>(arg);
}

template <typename T>
constexpr T &&forward(typename remove_reference<T>::type &arg) noexcept
{
    return static_cast<T &&>(arg);
}

template <typename T>
constexpr T &&forward(typename remove_reference<T>::type &&arg) noexcept
{
    return static_cast<T &&>(arg);
}

template <typename T> inline void swap(T &a, T &b)
{
    T temp = move(a);
    a      = move(b);
    b      = move(temp);
}

template <typename T> struct is_pod
{
    static const bool value = false;
};

template <> struct is_pod<float>
{
    static const bool value = true;
};

template <> struct is_pod<double>
{
    static const bool value = true;
};

template <> struct is_pod<char>
{
    static const bool value = true;
};

template <> struct is_pod<uint8_t>
{
    static const bool value = true;
};

template <> struct is_pod<int8_t>
{
    static const bool value = true;
};

template <> struct is_pod<uint16_t>
{
    static const bool value = true;
};

template <> struct is_pod<int16_t>
{
    static const bool value = true;
};

template <> struct is_pod<uint32_t>
{
    static const bool value = true;
};

template <> struct is_pod<int32_t>
{
    static const bool value = true;
};

template <> struct is_pod<uint64_t>
{
    static const bool value = true;
};

template <> struct is_pod<int64_t>
{
    static const bool value = true;
};

template <> struct is_pod<bool>
{
    static const bool value = true;
};

template <typename T> struct is_pod<T *>
{
    static const bool value = true;
};

template <typename T> struct hash
{
    size_t operator()(const T &op) const
    {
        const size_t size  = sizeof(T);
        const uint8_t *str = reinterpret_cast<const uint8_t *>(&op);
        uint32_t hash      = 2166136261u;
        for (size_t i = 0; i < size; i++)
        {
            hash ^= str[i];
            hash *= 16777619;
        }
        return hash;
    }
};

template <> struct hash<const char *>
{
    size_t operator()(const char *str) const
    {
        size_t length = strlen(str);
        uint32_t hash = 2166136261u;
        for (size_t i = 0; i < length; i++)
        {
            hash ^= str[i];
            hash *= 16777619;
        }
        return hash;
    }

    size_t operator()(const char *str, size_t len) const
    {
        uint32_t hash = 2166136261u;
        for (size_t i = 0; i < len; i++)
        {
            hash ^= str[i];
            hash *= 16777619;
        }
        return hash;
    }
};

template <typename T> struct equal_to
{
    bool operator()(const T &a, const T &b) const { return a == b; }

    template <typename U> bool operator()(const U &a, const T &b) const
    {
        return a == b;
    }
};

template <> struct equal_to<const char *>
{
    bool operator()(const char *a, const char *b) const
    {
        return strcmp(a, b) == 0;
    }
};

template <class RandomIt, class Compare>
void sort(RandomIt first, RandomIt last, Compare comp)
{
    if (first == last)
        return;

    struct Range
    {
        RandomIt l, r;
    };
    Range stack[64];
    int top      = 0;
    stack[top++] = {first, last - 1};

    while (top)
    {
        auto [l, r] = stack[--top];
        while (l < r)
        {
            RandomIt i   = l;
            RandomIt j   = r;
            RandomIt mid = l + (r - l) / 2;

            if (comp(*mid, *l))
                swap(*mid, *l);
            if (comp(*r, *l))
                swap(*r, *l);
            if (comp(*r, *mid))
                swap(*r, *mid);

            auto pivot = *mid;

            do
            {
                while (comp(*i, pivot))
                    ++i;
                while (comp(pivot, *j))
                    --j;
                if (i <= j)
                {
                    swap(*i, *j);
                    ++i;
                    --j;
                }
            } while (i <= j);

            // Tail recursion elimination
            if (j - l < r - i)
            { // Left partition is smaller
                if (l < j)
                    stack[top++] = {l, j};
                l = i; // Continue with right partition (larger)
            }
            else
            { // Right partition is smaller
                if (i < r)
                    stack[top++] = {i, r};
                r = j; // Continue with left partition (larger)
            }
        }
    }
}

// convenience overload using operator<
template <class RandomIt> inline void sort(RandomIt first, RandomIt last)
{
    sort(first, last, [](const auto &a, const auto &b) { return a < b; });
}

template <bool B, class T = void> struct enable_if
{
};

template <class T> struct enable_if<true, T>
{
    using type = T;
};

template <bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;

} // namespace zabato
