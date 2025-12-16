#pragma once

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <zabato/tuple.hpp>

namespace zabato
{
/**
 * @brief Policy for floating-point arithmetic.
 *
 * This policy uses standard `float` as the underlying storage type.
 * It provides a wrapper around standard math functions.
 */
struct float_policy
{
    using storage_type = float;

    static constexpr storage_type from_double(double v) { return v; }
    static constexpr storage_type from_float(float v) { return v; }
    static constexpr storage_type from_int(int32_t v)
    {
        return static_cast<storage_type>(v);
    }
    static constexpr storage_type from_int(uint32_t v)
    {
        return static_cast<storage_type>(v);
    }
    static constexpr storage_type from_int(int64_t v)
    {
        return static_cast<storage_type>(v);
    }
    static constexpr storage_type from_int(uint64_t v)
    {
        return static_cast<storage_type>(v);
    }
    static constexpr storage_type from_int(uint16_t v)
    {
        return static_cast<storage_type>(v);
    }
    static constexpr storage_type from_int(int16_t v)
    {
        return static_cast<storage_type>(v);
    }
    static constexpr storage_type from_int(uint8_t v)
    {
        return static_cast<storage_type>(v);
    }
    static constexpr storage_type from_int(int8_t v)
    {
        return static_cast<storage_type>(v);
    }
    static constexpr double to_double(storage_type v) { return v; }
    static constexpr float to_float(storage_type v) { return v; }
    static constexpr int32_t to_int(storage_type v) { return v; }
    static constexpr uint32_t to_uint(storage_type v) { return v; }
    static constexpr int64_t to_int64(storage_type v) { return v; }
    static constexpr uint64_t to_uint64(storage_type v) { return v; }
    static constexpr uint16_t to_uint16(storage_type v) { return v; }
    static constexpr int16_t to_int16(storage_type v) { return v; }
    static constexpr uint8_t to_uint8(storage_type v) { return v; }
    static constexpr int8_t to_int8(storage_type v) { return v; }

    static constexpr storage_type PI      = 3.1415926535f;
    static constexpr storage_type EPSILON = 1e-6;
    static constexpr storage_type MAX     = FLT_MAX;
    static constexpr storage_type MIN     = FLT_MIN;

    static constexpr storage_type add(storage_type a, storage_type b)
    {
        return a + b;
    }
    static constexpr storage_type sub(storage_type a, storage_type b)
    {
        return a - b;
    }
    static constexpr storage_type mul(storage_type a, storage_type b)
    {
        return a * b;
    }
    static constexpr storage_type div(storage_type a, storage_type b)
    {
        return a / b;
    }
    static constexpr storage_type neg(storage_type a) { return -a; }
    static constexpr storage_type mod(storage_type a, storage_type b)
    {
        return fmodf(a, b);
    }
    static constexpr storage_type abs(storage_type a) { return fabsf(a); }
    static constexpr storage_type sqrt(storage_type a) { return sqrtf(a); }
    static constexpr storage_type exp(storage_type a) { return expf(a); }
    static constexpr storage_type log(storage_type a) { return logf(a); }
    static constexpr storage_type pow(storage_type base, storage_type exp)
    {
        return powf(base, exp);
    }
    static constexpr storage_type min(storage_type a, storage_type b)
    {
        return a < b ? a : b;
    }
    static constexpr storage_type max(storage_type a, storage_type b)
    {
        return a > b ? a : b;
    }
    static constexpr storage_type floor(storage_type a) { return floorf(a); }
    static constexpr storage_type ceil(storage_type a) { return ceilf(a); }
    static constexpr storage_type round(storage_type a) { return roundf(a); }
    static constexpr storage_type sin(storage_type a) { return sinf(a); }
    static constexpr storage_type cos(storage_type a) { return cosf(a); }
    static constexpr storage_type acos(storage_type a) { return acosf(a); }
    static constexpr storage_type atan2(storage_type y, storage_type x)
    {
        return atan2f(y, x);
    }
    static constexpr storage_type tan(storage_type a) { return tanf(a); }
    static tuple<storage_type, storage_type> sincos(storage_type a)
    {
        return {sinf(a), cosf(a)};
    }

    static constexpr bool less_than(storage_type a, storage_type b)
    {
        return a < b;
    }
    static constexpr bool equals(storage_type a, storage_type b)
    {
        return a == b;
    }
};

/**
 * @brief Policy for fixed-point arithmetic.
 *
 * This policy implements fixed-point arithmetic using integers.
 * The scaling factor is determined by the `FractionalBits` template parameter.
 *
 * @tparam FractionalBits The number of bits used for the fractional part.
 */
template <int FractionalBits> struct fixed_point_policy
{
    using storage_type      = int32_t;
    using wide_storage_type = int64_t;

    static constexpr storage_type scale = static_cast<storage_type>(1)
                                          << FractionalBits;
    static constexpr storage_type fraction_mask =
        scale - static_cast<storage_type>(1);

    static constexpr storage_type from_double(double v)
    {
        return static_cast<storage_type>(v * scale);
    }
    static constexpr storage_type from_float(float v)
    {
        return static_cast<storage_type>(v * scale);
    }
    static constexpr storage_type from_int(int32_t v)
    {
        return static_cast<storage_type>(v * scale);
    }
    static constexpr storage_type from_int(uint32_t v)
    {
        return static_cast<storage_type>(v * scale);
    }
    static constexpr storage_type from_int(int64_t v)
    {
        return static_cast<storage_type>(v * scale);
    }
    static constexpr storage_type from_int(uint64_t v)
    {
        return static_cast<storage_type>(v * scale);
    }
    static constexpr storage_type from_int(int16_t v)
    {
        return static_cast<storage_type>(v * scale);
    }
    static constexpr storage_type from_int(uint16_t v)
    {
        return static_cast<storage_type>(v * scale);
    }
    static constexpr storage_type from_int(int8_t v)
    {
        return static_cast<storage_type>(v * scale);
    }
    static constexpr storage_type from_int(uint8_t v)
    {
        return static_cast<storage_type>(v * scale);
    }
    static constexpr double to_double(storage_type v)
    {
        return static_cast<double>(v) / scale;
    }
    static constexpr float to_float(storage_type v)
    {
        return static_cast<float>(v) / scale;
    }
    static constexpr int32_t to_int(storage_type v)
    {
        return static_cast<int32_t>(div(v, scale));
    }
    static constexpr uint32_t to_uint(storage_type v)
    {
        return static_cast<uint32_t>(div(v, scale));
    }
    static constexpr int64_t to_int64(storage_type v)
    {
        return static_cast<int64_t>(div(v, scale));
    }
    static constexpr uint64_t to_uint64(storage_type v)
    {
        return static_cast<uint64_t>(div(v, scale));
    }
    static constexpr int16_t to_int16(storage_type v)
    {
        return static_cast<int16_t>(div(v, scale));
    }
    static constexpr uint16_t to_uint16(storage_type v)
    {
        return static_cast<uint16_t>(div(v, scale));
    }
    static constexpr int8_t to_int8(storage_type v)
    {
        return static_cast<int8_t>(div(v, scale));
    }
    static constexpr uint8_t to_uint8(storage_type v)
    {
        return static_cast<uint8_t>(div(v, scale));
    }

    static constexpr storage_type PI       = from_float(3.1415926535f);
    static constexpr storage_type PI_DIV_2 = PI >> 1;
    static constexpr storage_type LN2      = from_float(0.69314718056f);
    static constexpr storage_type EPSILON  = 1;
    static constexpr storage_type MAX      = (storage_type)INT32_MAX;
    static constexpr storage_type MIN      = (storage_type)INT32_MIN;

    static constexpr storage_type add(storage_type a, storage_type b)
    {
        return a + b;
    }
    static constexpr storage_type sub(storage_type a, storage_type b)
    {
        return a - b;
    }
    static constexpr storage_type neg(storage_type a) { return -a; }
    static constexpr storage_type mod(storage_type a, storage_type b)
    {
        // a - trunc(a/b) * b
        return sub(a, mul(floor(div(a, b)), b));
    }

    static constexpr storage_type mul(storage_type a, storage_type b)
    {
        return static_cast<storage_type>(
            (static_cast<wide_storage_type>(a) * b) >> FractionalBits);
    }

    static constexpr storage_type div(storage_type a, storage_type b)
    {
        return static_cast<storage_type>(
            (static_cast<wide_storage_type>(a) << FractionalBits) / b);
    }

    static constexpr storage_type abs(storage_type a) { return a > 0 ? a : -a; }

    /** @brief Calculates approximate square root using Newton's method. */
    static constexpr storage_type sqrt(storage_type a)
    {
        if (a < 0)
            return 0;

        wide_storage_type n = static_cast<wide_storage_type>(a)
                              << FractionalBits;
        wide_storage_type res  = n;
        wide_storage_type last = 0;

        do
        {
            last = res;
            res  = (res + n / res) / 2;
        } while (last != res);

        return static_cast<storage_type>(res);
    }

    /** @brief Calculates approximate exponential e^x. */
    static constexpr storage_type exp(storage_type x)
    {
        if (x == 0)
            return from_int(1);

        // e^x = 2^(x / ln(2))
        storage_type y = div(x, LN2);

        // y = I + F (Integer + Fraction)
        int32_t i_part      = to_int(y);
        storage_type f_part = y - from_int(i_part);

        // 2^I is a bit shift
        storage_type two_pow_i =
            (i_part >= 0) ? (from_int(1) << i_part) : (from_int(1) >> -i_part);

        // 2^F = e^(F * ln(2)) using Taylor series for e^z where z = F * ln(2)
        storage_type z          = mul(f_part, LN2);
        storage_type series_sum = from_int(1);
        storage_type term       = from_int(1);

        for (int i = 1; i < 10; i++)
        {
            term       = mul(term, z);
            term       = div(term, from_int(i));
            series_sum = add(series_sum, term);
        }

        return mul(two_pow_i, series_sum);
    }

    /** @brief Calculates approximate natural logarithm ln(x). */
    static constexpr storage_type log(storage_type x)
    {
        if (x <= 0)
            return MIN;

        // Find E such that x = M * 2^E, with M in [1, 2)
        int32_t exponent = (31 - __builtin_clz(x)) - FractionalBits;
        storage_type mantissa =
            (exponent >= 0) ? (x >> exponent) : (x << -exponent);

        // ln(M * 2^E) = ln(M) + E * ln(2)
        storage_type e_ln2 = mul(from_int(exponent), LN2);

        // Calculate ln(M) using Taylor series for ln((1+y)/(1-y))
        storage_type one        = from_int(1);
        storage_type y          = div(sub(mantissa, one), add(mantissa, one));
        storage_type y_sq       = mul(y, y);
        storage_type series_sum = y;
        storage_type term       = y;

        for (int i = 1; i < 10; ++i)
        {
            term       = mul(term, y_sq);
            series_sum = add(series_sum, div(term, from_int(2 * i + 1)));
        }

        return add(e_ln2, mul(from_int(2), series_sum));
    }

    static constexpr storage_type pow(storage_type base, storage_type exp_val)
    {
        if (base == 0)
            return 0;
        // pow(b, e) = exp(e * log(b))
        return exp(mul(exp_val, log(base)));
    }

    static constexpr storage_type min(storage_type a, storage_type b)
    {
        return a < b ? a : b;
    }
    static constexpr storage_type max(storage_type a, storage_type b)
    {
        return a > b ? a : b;
    }

    static constexpr storage_type floor(storage_type a)
    {
        return a & ~fraction_mask;
    }
    static constexpr storage_type ceil(storage_type a)
    {
        return (a + fraction_mask) & ~fraction_mask;
    }
    static constexpr storage_type round(storage_type a)
    {
        return floor(a + (scale >> 1));
    }

    static constexpr storage_type sin(storage_type a)
    {
        auto [sin, cos] = sincos(a);
        return sin;
    }

    static constexpr storage_type cos(storage_type a)
    {
        auto [sin, cos] = sincos(a);
        return cos;
    }

    static constexpr storage_type acos(storage_type x)
    {
        return atan2(sqrt(from_int(1) - mul(x, x)), x);
    }

    static constexpr storage_type atan2(storage_type y, storage_type x)
    {
        if (x == 0)
            return y > 0 ? PI >> 1 : -(PI >> 1);
        if (y == 0)
            return x > 0 ? 0 : PI;

        storage_type angle = 0;
        for (int i = 0; i < CORDIC_NTAB; ++i)
        {
            storage_type x_shifted = x >> i;
            storage_type y_shifted = y >> i;
            if (y > 0)
            {
                x += y_shifted;
                y -= x_shifted;
                angle += cordic_ctab[i];
            }
            else
            {
                x -= y_shifted;
                y += x_shifted;
                angle -= cordic_ctab[i];
            }
        }
        return angle;
    }

    static constexpr storage_type tan(storage_type a)
    {
        auto [sin_val, cos_val] = sincos(a);
        return div(sin_val, cos_val);
    }

    /** @brief Calculates sine and cosine using CORDIC algorithm. */
    static constexpr tuple<storage_type, storage_type>
    sincos(storage_type angle)
    {
#pragma region Range Reduction
        // Map the input angle to the range [-pi/2, pi/2] and determine the
        // quadrant.
        auto quadrant_fixed = static_cast<storage_type>(
            (static_cast<wide_storage_type>(angle) << FractionalBits) /
            PI_DIV_2);
        auto quadrant = quadrant_fixed >> FractionalBits;

        // Use floor-like behavior for negative angles. If the angle is negative
        // and has a fractional part, decrement the quadrant.
        if (quadrant_fixed < 0 && (quadrant_fixed & fraction_mask) != 0)
            quadrant--;

        storage_type quadrant_term = mul(from_int(quadrant), PI_DIV_2);
        storage_type reduced_angle = sub(angle, quadrant_term);
#pragma endregion Range Reduction

        storage_type x = CORDIC_K_INV;
        storage_type y = 0;

        for (int k = 0; k < CORDIC_NTAB; ++k)
        {
            storage_type y_shifted = y >> k;
            storage_type x_shifted = x >> k;

            if (reduced_angle >= 0)
            {
                x = x - y_shifted;
                y = y + x_shifted;
                reduced_angle -= cordic_ctab[k];
            }
            else
            {
                x = x + y_shifted;
                y = y - x_shifted;
                reduced_angle += cordic_ctab[k];
            }
        }

        uint8_t quad_idx = (quadrant % 4 + 4) % 4;

        switch (quad_idx)
        {
        case 0:
            return {y, x};
        case 1:
            return {x, -y};
        case 2:
            return {-y, -x};
        case 3:
            return {-x, y};
        }
    }

    static constexpr bool less_than(storage_type a, storage_type b)
    {
        return a < b;
    }
    static constexpr bool equals(storage_type a, storage_type b)
    {
        return a == b;
    }

private:
    static constexpr int CORDIC_NTAB = 16;
    static constexpr storage_type CORDIC_K_INV =
        static_cast<storage_type>(0.6072529350088813 * scale);
    static constexpr storage_type cordic_ctab[CORDIC_NTAB] = {
        static_cast<storage_type>(0.7853981633974483 * scale),
        static_cast<storage_type>(0.4636476090008061 * scale),
        static_cast<storage_type>(0.2449786631268641 * scale),
        static_cast<storage_type>(0.1243549945467614 * scale),
        static_cast<storage_type>(0.0624188099959573 * scale),
        static_cast<storage_type>(0.0312398334302683 * scale),
        static_cast<storage_type>(0.0156237286204768 * scale),
        static_cast<storage_type>(0.0078123410601011 * scale),
        static_cast<storage_type>(0.0039062301319669 * scale),
        static_cast<storage_type>(0.0019531225164788 * scale),
        static_cast<storage_type>(0.0009765621895593 * scale),
        static_cast<storage_type>(0.0004882812111948 * scale),
        static_cast<storage_type>(0.0002441406201494 * scale),
        static_cast<storage_type>(0.0001220703118936 * scale),
        static_cast<storage_type>(0.0000610351561742 * scale),
        static_cast<storage_type>(0.0000305175781153 * scale)};
};

/**
 * @brief Wrapper class for real number arithmetic using a specified policy.
 *
 * @tparam Policy The arithmetic policy to use (e.g., float_policy,
 * fixed_point_policy).
 */
template <typename Policy> class custom_real
{
public:
    using storage_type = typename Policy::storage_type;

    /** @brief Default constructor. Initializes to zero. */
    constexpr custom_real() : m_value(Policy::from_int(0)) {}

    /** @name Constructors
     * Construct from various arithmetic types.
     * @{
     */
    constexpr custom_real(double v) : m_value(Policy::from_double(v)) {}
    constexpr custom_real(float v) : m_value(Policy::from_float(v)) {}
    constexpr custom_real(int32_t v) : m_value(Policy::from_int(v)) {}
    constexpr custom_real(uint32_t v) : m_value(Policy::from_int(v)) {}
    constexpr custom_real(int64_t v) : m_value(Policy::from_int(v)) {}
    constexpr custom_real(uint64_t v) : m_value(Policy::from_int(v)) {}
    constexpr custom_real(int16_t v) : m_value(Policy::from_int(v)) {}
    constexpr custom_real(uint16_t v) : m_value(Policy::from_int(v)) {}
    constexpr custom_real(int8_t v) : m_value(Policy::from_int(v)) {}
    constexpr custom_real(uint8_t v) : m_value(Policy::from_int(v)) {}
    /** @} */

    /** @name Conversion Operators
     * Explicit conversion to standard arithmetic types.
     * @{
     */
    constexpr explicit operator double() const
    {
        return Policy::to_double(m_value);
    }
    constexpr explicit operator float() const
    {
        return Policy::to_float(m_value);
    }
    constexpr explicit operator int32_t() const
    {
        return Policy::to_int(m_value);
    }
    constexpr explicit operator uint32_t() const
    {
        return Policy::to_uint(m_value);
    }
    constexpr explicit operator int64_t() const
    {
        return Policy::to_int64(m_value);
    }
    constexpr explicit operator uint64_t() const
    {
        return Policy::to_uint64(m_value);
    }
    constexpr explicit operator int16_t() const
    {
        return Policy::to_int16(m_value);
    }
    constexpr explicit operator uint16_t() const
    {
        return Policy::to_uint16(m_value);
    }
    constexpr explicit operator int8_t() const
    {
        return Policy::to_int8(m_value);
    }
    constexpr explicit operator uint8_t() const
    {
        return Policy::to_uint8(m_value);
    }
    /** @} */

    /** @brief Unary plus. */
    custom_real operator+() const { return from_raw(m_value); }
    /** @brief Unary minus (negation). */
    custom_real operator-() const { return from_raw(Policy::neg(m_value)); }

    /** @name Arithmetic Operators
     * @{
     */
    constexpr custom_real operator+(const custom_real &rhs) const
    {
        return from_raw(Policy::add(m_value, rhs.m_value));
    }

    constexpr custom_real operator-(const custom_real &rhs) const
    {
        return from_raw(Policy::sub(m_value, rhs.m_value));
    }

    constexpr custom_real operator*(const custom_real &rhs) const
    {
        return from_raw(Policy::mul(m_value, rhs.m_value));
    }

    constexpr custom_real operator/(const custom_real &rhs) const
    {
        return from_raw(Policy::div(m_value, rhs.m_value));
    }

    constexpr custom_real &operator+=(const custom_real &rhs)
    {
        m_value = Policy::add(m_value, rhs.m_value);
        return *this;
    }

    constexpr custom_real &operator-=(const custom_real &rhs)
    {
        m_value = Policy::sub(m_value, rhs.m_value);
        return *this;
    }

    constexpr custom_real &operator*=(const custom_real &rhs)
    {
        m_value = Policy::mul(m_value, rhs.m_value);
        return *this;
    }

    constexpr custom_real &operator/=(const custom_real &rhs)
    {
        m_value = Policy::div(m_value, rhs.m_value);
        return *this;
    }
    /** @} */

    /** @name Comparison Operators
     * @{
     */
    constexpr bool operator<(const custom_real &rhs) const
    {
        return Policy::less_than(m_value, rhs.m_value);
    }

    constexpr bool operator>(const custom_real &rhs) const
    {
        return Policy::less_than(rhs.m_value, m_value);
    }

    constexpr bool operator<=(const custom_real &rhs) const
    {
        return !Policy::less_than(rhs.m_value, m_value);
    }

    constexpr bool operator>=(const custom_real &rhs) const
    {
        return !Policy::less_than(m_value, rhs.m_value);
    }

    constexpr bool operator==(const custom_real &rhs) const
    {
        return Policy::equals(m_value, rhs.m_value);
    }

    constexpr bool operator!=(const custom_real &rhs) const
    {
        return !Policy::equals(m_value, rhs.m_value);
    }
    /** @} */

    /** @brief Returns the raw underlying value. */
    storage_type raw() const { return m_value; }

    /** @brief Returns PI constant from policy. */
    static custom_real pi() { return from_raw(Policy::PI); }

    /** @brief Returns epsilon from policy. */
    static custom_real epsilon() { return from_raw(Policy::EPSILON); }

    /** @brief Returns max value from policy. */
    static custom_real max_val() { return from_raw(Policy::MAX); }

private:
    constexpr static custom_real from_raw(storage_type raw_value)
    {
        custom_real r;
        r.m_value = raw_value;
        return r;
    }

    template <typename P>
    friend constexpr custom_real<P> pow(const custom_real<P> &,
                                        const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> log(const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> exp(const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> max(const custom_real<P> &,
                                        const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> mod(const custom_real<P> &,
                                        const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> min(const custom_real<P> &,
                                        const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> floor(const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> ceil(const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> round(const custom_real<P> &);

    template <typename P>
    friend constexpr custom_real<P> abs(const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> sqrt(const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> sin(const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> cos(const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> acos(const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> atan2(const custom_real<P> &,
                                          const custom_real<P> &);
    template <typename P>
    friend constexpr custom_real<P> tan(const custom_real<P> &);
    template <typename P>
    friend constexpr tuple<custom_real<P>, custom_real<P>>
    sincos(const custom_real<P> &);

    storage_type m_value;
};

using real = custom_real<float_policy>;

/**
 * @brief Calculates the arccosine of a value.
 * @param val The input value.
 * @return The arccosine of val.
 */
template <typename Policy>
constexpr custom_real<Policy> acos(const custom_real<Policy> &val)
{
    return custom_real<Policy>::from_raw(Policy::acos(val.m_value));
}

/**
 * @brief Calculates the arc tangent of y/x.
 * @param x The x coordinate.
 * @param y The y coordinate.
 * @return The angle in radians.
 */
template <typename Policy>
constexpr custom_real<Policy> atan2(const custom_real<Policy> &x,
                                    const custom_real<Policy> &y)
{
    return custom_real<Policy>::from_raw(Policy::atan2(x.m_value, y.m_value));
}

/**
 * @brief Calculates the floor of a value.
 * @param val The input value.
 * @return The largest integer not greater than val.
 */
template <typename Policy>
constexpr custom_real<Policy> floor(const custom_real<Policy> &val)
{
    return custom_real<Policy>::from_raw(Policy::floor(val.m_value));
}

/**
 * @brief Calculates the ceiling of a value.
 * @param val The input value.
 * @return The smallest integer not less than val.
 */
template <typename Policy>
constexpr custom_real<Policy> ceil(const custom_real<Policy> &val)
{
    return custom_real<Policy>::from_raw(Policy::ceil(val.m_value));
}

/**
 * @brief Rounds a value to the nearest integer.
 * @param val The input value.
 * @return The rounded value.
 */
template <typename Policy>
constexpr custom_real<Policy> round(const custom_real<Policy> &val)
{
    return custom_real<Policy>::from_raw(Policy::round(val.m_value));
}

/**
 * @brief Calculates the absolute value.
 * @param val The input value.
 * @return The absolute value of val.
 */
template <typename Policy>
constexpr custom_real<Policy> abs(const custom_real<Policy> &val)
{
    return custom_real<Policy>::from_raw(Policy::abs(val.m_value));
}

/**
 * @brief Calculates the square root of a value.
 * @param val The input value.
 * @return The square root of val.
 */
template <typename Policy>
constexpr custom_real<Policy> sqrt(const custom_real<Policy> &val)
{
    return custom_real<Policy>::from_raw(Policy::sqrt(val.m_value));
}

/**
 * @brief Calculates the power of a base to an exponent.
 * @param base The base value.
 * @param exp The exponent value.
 * @return The result of base raised to the power of exp.
 */
template <typename Policy>
constexpr custom_real<Policy> pow(const custom_real<Policy> &base,
                                  const custom_real<Policy> &exp)
{
    return custom_real<Policy>::from_raw(Policy::pow(base.raw(), exp.raw()));
}

/**
 * @brief Calculates the natural logarithm of a value.
 * @param val The input value.
 * @return The natural logarithm of val.
 */
template <typename Policy>
constexpr custom_real<Policy> log(const custom_real<Policy> &val)
{
    return custom_real<Policy>::from_raw(Policy::log(val.raw()));
}

/**
 * @brief Calculates the exponential of a value (e^val).
 * @param val The input value.
 * @return The exponential of val.
 */
template <typename Policy>
constexpr custom_real<Policy> exp(const custom_real<Policy> &val)
{
    return custom_real<Policy>::from_raw(Policy::exp(val.raw()));
}

/**
 * @brief Calculates the sine of an angle.
 * @param val The angle in radians.
 * @return The sine of the angle.
 */
template <typename Policy>
constexpr custom_real<Policy> sin(const custom_real<Policy> &val)
{
    return custom_real<Policy>::from_raw(Policy::sin(val.m_value));
}

/**
 * @brief Calculates the tangent of an angle.
 * @param val The angle in radians.
 * @return The tangent of the angle.
 */
template <typename Policy>
constexpr custom_real<Policy> tan(const custom_real<Policy> &val)
{
    return custom_real<Policy>::from_raw(Policy::tan(val.m_value));
}

/**
 * @brief Returns the smaller of two values.
 * @param a First value.
 * @param b Second value.
 * @return The smaller value.
 */
template <typename Policy>
constexpr custom_real<Policy> min(const custom_real<Policy> &a,
                                  const custom_real<Policy> &b)
{
    return custom_real<Policy>::from_raw(Policy::min(a.m_value, b.m_value));
}

/**
 * @brief Returns the larger of two values.
 * @param a First value.
 * @param b Second value.
 * @return The larger value.
 */
template <typename Policy>
constexpr custom_real<Policy> max(const custom_real<Policy> &a,
                                  const custom_real<Policy> &b)
{
    return custom_real<Policy>::from_raw(Policy::max(a.m_value, b.m_value));
}

/**
 * @brief Calculates the modulo of two values.
 * @param a The dividend.
 * @param b The divisor.
 * @return The remainder of a / b.
 */
template <typename Policy>
constexpr custom_real<Policy> mod(const custom_real<Policy> &a,
                                  const custom_real<Policy> &b)
{
    return custom_real<Policy>::from_raw(Policy::mod(a.m_value, b.m_value));
}

/**
 * @brief Calculates the cosine of an angle.
 * @param val The angle in radians.
 * @return The cosine of the angle.
 */
template <typename Policy>
constexpr custom_real<Policy> cos(const custom_real<Policy> &val)
{
    return custom_real<Policy>::from_raw(Policy::cos(val.m_value));
}

/**
 * @brief Calculates sine and cosine of an angle.
 * @param val The angle in radians.
 * @return A tuple containing {sin(val), cos(val)}.
 */
template <typename Policy>
tuple<custom_real<Policy>, custom_real<Policy>> constexpr sincos(
    const custom_real<Policy> &val)
{
    auto [s, c] = Policy::sincos(val.m_value);
    return {custom_real<Policy>::from_raw(s), custom_real<Policy>::from_raw(c)};
}

/**
 * @brief Converts degrees to radians.
 * @param deg The angle in degrees.
 * @return The angle in radians.
 */
template <typename Policy>
constexpr custom_real<Policy> to_rad(const custom_real<Policy> &deg)
{
    return deg * (custom_real<Policy>::pi() / custom_real<Policy>(180));
}

/**
 * @brief Converts radians to degrees.
 * @param rad The angle in radians.
 * @return The angle in degrees.
 */
template <typename Policy>
constexpr custom_real<Policy> to_deg(const custom_real<Policy> &rad)
{
    return rad * (custom_real<Policy>(180) / custom_real<Policy>::pi());
}

/**
 * @brief Clamps a value between a minimum and maximum.
 * @param v The value to clamp.
 * @param min_val The minimum allowed value.
 * @param max_val The maximum allowed value.
 * @return The clamped value.
 */
template <typename T>
inline constexpr T clamp(const T &v, const T &min_val, const T &max_val)
{
    return max(min(v, max_val), min_val);
}

/**
 * @brief Maps a value from one range to another.
 * @param src The value to map.
 * @param srcMin The lower bound of the source range.
 * @param srcMax The upper bound of the source range.
 * @param dstMin The lower bound of the destination range.
 * @param dstMax The upper bound of the destination range.
 * @return The mapped value.
 */
template <typename T>
constexpr T map(T src, T srcMin, T srcMax, T dstMin, T dstMax)
{
    const T src_offset = src - srcMin;
    const T src_range  = srcMax - srcMin;
    const T dst_range  = dstMax - dstMin;
    const T offset     = src_offset / src_range;
    const T dst_offset = offset * dst_range;
    return clamp(dstMin + dst_offset, dstMin, dstMax);
}

/**
 * @brief Linearly interpolates between two values.
 * @param a The start value.
 * @param b The end value.
 * @param t The interpolation factor (usually 0.0 - 1.0).
 * @return The interpolated value.
 */
template <typename T> constexpr T lerp(const T &a, const T &b, const T &t)
{
    return a + (b - a) * t;
}

} // namespace zabato