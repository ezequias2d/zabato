#pragma once

#include <cstdint>
#include <stdint.h>
#include <string.h>
#include <type_traits>
#include <zabato/endian.hpp>
#include <zabato/real.hpp>

namespace zabato
{

namespace internal
{

template <class To, class From>
constexpr std::enable_if_t<sizeof(To) == sizeof(From) &&
                               std::is_trivially_copyable_v<From> &&
                               std::is_trivially_copyable_v<To>,
                           To>
bit_cast(const From &src) noexcept
{
    To dst;
    memcpy(&dst, &src, sizeof(To));
    return dst;
}

constexpr bool is_big_endian_host() { return !is_little_endian; }

template <typename T>
constexpr typename std::enable_if<sizeof(T) == 1, T>::type byteswap(T val)
{
    return val;
}

template <typename T>
constexpr typename std::enable_if<sizeof(T) == 2, T>::type byteswap(T val)
{
    if (is_big_endian_host())
        return val;
    uint16_t u = bit_cast<uint16_t>(val);
    u          = ((u & 0xFF00) >> 8) | ((u & 0x00FF) << 8);
    return bit_cast<T, uint16_t>(u);
}

template <typename T>
constexpr typename std::enable_if<sizeof(T) == 4, T>::type byteswap(T val)
{
    if (is_big_endian_host())
        return val;
    uint32_t u = bit_cast<uint32_t>(val);
    u          = ((u & 0xFF000000) >> 24) | ((u & 0x00FF0000) >> 8) |
        ((u & 0x0000FF00) << 8) | ((u & 0x000000FF) << 24);
    return bit_cast<T>(u);
}

template <typename T>
constexpr typename std::enable_if<sizeof(T) == 8, T>::type byteswap(T val)
{
    if (is_big_endian_host())
        return val;
    uint64_t u = bit_cast<uint64_t>(val);
    u          = ((u & 0xFF00000000000000ULL) >> 56) |
        ((u & 0x00FF000000000000ULL) >> 40) |
        ((u & 0x0000FF0000000000ULL) >> 24) |
        ((u & 0x000000FF00000000ULL) >> 8) |
        ((u & 0x00000000FF000000ULL) << 8) |
        ((u & 0x0000000000FF0000ULL) << 24) |
        ((u & 0x000000000000FF00ULL) << 40) |
        ((u & 0x00000000000000FFULL) << 56);
    return bit_cast<T>(u);
}
} // namespace internal

#pragma pack(push, 1)
/**
 * @brief A wrapper class for handling Big Endian data types transparently.
 *
 * This class wraps a scalar type T and ensures that it is stored in Big Endian
 * format in memory. Accessing the value (via implicit conversion or operators)
 * will automatically convert it to the host's native endianness. This is useful
 * for file I/O operations where data is stored in a specific endianness (e.g.,
 * network protocols, file formats).
 *
 * The class supports standard arithmetic and bitwise operators, making it
 * behave almost like the underlying type T.
 *
 * @tparam T The underlying arithmetic type (integer or floating point).
 */
template <typename T> class BigEndian
{
    static_assert(std::is_arithmetic<T>::value || std::is_class<T>::value,
                  "BigEndian supports arithmetic types and wrapper classes");

private:
    T m_data;

public:
    /**
     * @brief Default constructor. Initializes value to 0.
     */
    constexpr BigEndian() : m_data(0) {}

    /**
     * @brief Constructor from a native value.
     * @param val The value in host endianness. It will be converted to Big
     * Endian for storage.
     */
    constexpr BigEndian(T val) : m_data(internal::byteswap(val)) {}

    constexpr BigEndian(const BigEndian &other) = default;

    /**
     * @brief Constructor from a real value (with explicit cast).
     * @param val The real value.
     */
    template <
        typename U                                                        = T,
        typename std::enable_if<!std::is_same<U, real>::value, int>::type = 0>
    constexpr BigEndian(real val)
        : m_data(internal::byteswap(static_cast<T>(val)))
    {
    }

#pragma region Operators
    /**
     * @brief Implicit conversion to the native type T.
     * @return The value converted to host endianness.
     */
    constexpr operator T() const { return internal::byteswap(m_data); }

    /**
     * @brief Assignment operator from a native value.
     * @param val The value in host endianness.
     * @return Reference to this object.
     */
    BigEndian &operator=(T val)
    {
        m_data = internal::byteswap(val);
        return *this;
    }

    BigEndian &operator+=(T val)
    {
        *this = static_cast<T>(*this) + val;
        return *this;
    }

    BigEndian &operator-=(T val)
    {
        *this = static_cast<T>(*this) - val;
        return *this;
    }

    BigEndian &operator*=(T val)
    {
        *this = static_cast<T>(*this) * val;
        return *this;
    }

    BigEndian &operator/=(T val)
    {
        *this = static_cast<T>(*this) / val;
        return *this;
    }
#pragma endregion Operators

#pragma region Integer Only Operators

    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, BigEndian &>::type
    operator%=(T val)
    {
        *this = static_cast<T>(*this) % val;
        return *this;
    }

    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, BigEndian &>::type
    operator&=(T val)
    {
        *this = static_cast<T>(*this) & val;
        return *this;
    }

    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, BigEndian &>::type
    operator|=(T val)
    {
        *this = static_cast<T>(*this) | val;
        return *this;
    }

    template <typename U = T>
    typename std::enable_if<std::is_integral<U>::value, BigEndian &>::type
    operator^=(T val)
    {
        *this = static_cast<T>(*this) ^ val;
        return *this;
    }

#pragma endregion Integer Only Operators

#pragma region Increment/Decrement Operators

    BigEndian &operator++()
    {
        *this += 1;
        return *this;
    }
    BigEndian &operator--()
    {
        *this -= 1;
        return *this;
    }

    BigEndian operator++(int)
    {
        BigEndian temp = *this;
        ++*this;
        return temp;
    }
    BigEndian operator--(int)
    {
        BigEndian temp = *this;
        --*this;
        return temp;
    }

#pragma endregion Increment / Decrement Operators

    /**
     * @brief Gets a pointer to the raw Big Endian data.
     * @return Const pointer to the data.
     */
    const T *raw() const { return &m_data; }

    /**
     * @brief Gets a pointer to the raw Big Endian data.
     * @return Pointer to the data.
     */
    T *raw() { return &m_data; }
};
#pragma pack(pop)

#pragma region Type Aliases

using be_int8_t  = BigEndian<int8_t>;
using be_uint8_t = BigEndian<uint8_t>;

using be_int16_t  = BigEndian<int16_t>;
using be_uint16_t = BigEndian<uint16_t>;

using be_int32_t  = BigEndian<int32_t>;
using be_uint32_t = BigEndian<uint32_t>;

using be_int64_t  = BigEndian<int64_t>;
using be_uint64_t = BigEndian<uint64_t>;

using be_float  = BigEndian<float>;
using be_double = BigEndian<double>;

using be_real = BigEndian<real>;

#pragma endregion Type Aliases

} // namespace zabato
