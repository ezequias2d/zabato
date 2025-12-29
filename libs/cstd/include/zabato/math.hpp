#pragma once

#include <zabato/utils.hpp>

namespace zabato
{
template <typename T> struct vec2;
template <typename T> struct vec3;
template <typename T> struct vec4;
template <typename T> struct quat;
template <typename T> struct mat4;
template <typename T> struct plane3;
template <typename T> struct mat3;

/**
 * @brief A structure representing a 2-dimensional vector.
 *
 * @tparam T The underlying numeric type of the vector components.
 */
template <typename T> struct vec2
{
    /** @brief The x component of the vector. */
    T x;
    /** @brief The y component of the vector. */
    T y;

    /**
     * @brief Default constructor. Initializes x and y to 0.
     */
    constexpr vec2() : x(0), y(0) {}

    /**
     * @brief Constructs a vector with both components set to the same value.
     * @param val The value to set for both x and y.
     */
    constexpr vec2(T val) : x(val), y(val) {}

    /**
     * @brief Constructs a vector with specific x and y values.
     * @param x_val The value for the x component.
     * @param y_val The value for the y component.
     */
    constexpr vec2(T x_val, T y_val) : x(x_val), y(y_val) {}

    /**
     * @brief Adds another vector to this vector.
     * @param rhs The vector to add.
     * @return A reference to this vector after modification.
     */
    constexpr vec2<T> &operator+=(const vec2<T> &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    /**
     * @brief Subtracts another vector from this vector.
     * @param rhs The vector to subtract.
     * @return A reference to this vector after modification.
     */
    constexpr vec2<T> &operator-=(const vec2<T> &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    /**
     * @brief Multiplies this vector by a scalar.
     * @param s The scalar value to multiply by.
     * @return A reference to this vector after modification.
     */
    constexpr vec2<T> &operator*=(T s)
    {
        x *= s;
        y *= s;
        return *this;
    }

    /**
     * @brief Divides this vector by a scalar.
     * @param s The scalar value to divide by.
     * @return A reference to this vector after modification.
     */
    constexpr vec2<T> &operator/=(T s)
    {
        x /= s;
        y /= s;
        return *this;
    }

    /**
     * @brief Addition operator.
     * @param v The vector to add.
     * @return A new vector containing the sum.
     */
    constexpr vec2<T> operator+(const vec2<T> &v) const
    {
        return vec2<T>(x + v.x, y + v.y);
    }

    /**
     * @brief Subtraction operator.
     * @param v The vector to subtract.
     * @return A new vector containing the difference.
     */
    constexpr vec2<T> operator-(const vec2<T> &v) const
    {
        return vec2<T>(x - v.x, y - v.y);
    }

    /**
     * @brief Unary negation operator.
     * @return A new vector with negated components.
     */
    constexpr vec2<T> operator-() const { return vec2<T>(-x, -y); }

    /**
     * @brief Component-wise multiplication.
     * @param v The other vector.
     * @return A new vector with the component-wise product.
     */
    constexpr vec2<T> operator*(const vec2<T> &v) const
    {
        return vec2<T>(x * v.x, y * v.y);
    }

    /**
     * @brief Scalar multiplication.
     * @param s The scalar.
     * @return A new vector scaled by s.
     */
    constexpr vec2<T> operator*(T s) const { return vec2<T>(x * s, y * s); }

    /**
     * @brief Component-wise division.
     * @param v The other vector.
     * @return A new vector with the component-wise quotient.
     */
    constexpr vec2<T> operator/(const vec2<T> &v) const
    {
        return vec2<T>(x / v.x, y / v.y);
    }
    /**
     * @brief Scalar division.
     * @param s The scalar.
     * @return A new vector divided by s.
     */
    constexpr vec2<T> operator/(T s) const { return vec2<T>(x / s, y / s); }

    /**
     * @brief Equality operator.
     * @param v The vector to compare with.
     * @return True if vectors are equal (within epsilon), false otherwise.
     */
    constexpr bool operator==(const vec2<T> &v) const
    {
        return is_equal(*this, v);
    }

    /**
     * @brief Inequality operator.
     * @param v The vector to compare with.
     * @return True if vectors are not equal, false otherwise.
     */
    constexpr bool operator!=(const vec2<T> &v) const
    {
        return !is_equal(*this, v);
    }

    /**
     * @brief Access component by index.
     * @param i The index.
     * @return Reference to the component.
     */
    constexpr T &operator[](int i) { return (&x)[i]; }

    /**
     * @brief Access component by index (const).
     * @param i The index.
     * @return Const reference to the component.
     */
    constexpr const T &operator[](int i) const { return (&x)[i]; }
};

/**
 * @brief A structure representing a 3-dimensional vector.
 *
 * @tparam T The underlying numeric type of the vector components.
 */
template <typename T> struct vec3
{
    /** @brief The x component of the vector. */
    T x;
    /** @brief The y component of the vector. */
    T y;
    /** @brief The z component of the vector. */
    T z;

    /**
     * @brief Default constructor. Initializes x, y, and z to 0.
     */
    constexpr vec3() : x(0), y(0), z(0) {}

    /**
     * @brief Constructs a vector with all components set to the same value.
     * @param val The value to set for x, y, and z.
     */
    constexpr vec3(T val) : x(val), y(val), z(val) {}

    /**
     * @brief Constructs a vector with specific x, y, and z values.
     * @param x_val The value for the x component.
     * @param y_val The value for the y component.
     * @param z_val The value for the z component.
     */
    constexpr vec3(T x_val, T y_val, T z_val) : x(x_val), y(y_val), z(z_val) {}

    /**
     * @brief Adds another vector to this vector.
     * @param rhs The vector to add.
     * @return A reference to this vector after modification.
     */
    constexpr vec3<T> &operator+=(const vec3<T> &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    /**
     * @brief Subtracts another vector from this vector.
     * @param rhs The vector to subtract.
     * @return A reference to this vector after modification.
     */
    constexpr vec3<T> &operator-=(const vec3<T> &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    /**
     * @brief Multiplies this vector by a scalar.
     * @param s The scalar value to multiply by.
     * @return A reference to this vector after modification.
     */
    constexpr vec3<T> &operator*=(T s)
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    /**
     * @brief Divides this vector by a scalar.
     * @param s The scalar value to divide by.
     * @return A reference to this vector after modification.
     */
    constexpr vec3<T> &operator/=(T s)
    {
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }

    /**
     * @brief Addition operator.
     * @param v The vector to add.
     * @return A new vector containing the sum.
     */
    constexpr vec3<T> operator+(const vec3<T> &v) const
    {
        return vec3<T>(x + v.x, y + v.y, z + v.z);
    }

    /**
     * @brief Subtraction operator.
     * @param v The vector to subtract.
     * @return A new vector containing the difference.
     */
    constexpr vec3<T> operator-(const vec3<T> &v) const
    {
        return vec3<T>(x - v.x, y - v.y, z - v.z);
    }

    /**
     * @brief Unary negation operator.
     * @return A new vector with negated components.
     */
    constexpr vec3<T> operator-() const { return vec3<T>(-x, -y, -z); }

    /**
     * @brief Component-wise multiplication.
     * @param v The other vector.
     * @return A new vector with the component-wise product.
     */
    constexpr vec3<T> operator*(const vec3<T> &v) const
    {
        return vec3<T>(x * v.x, y * v.y, z * v.z);
    }

    /**
     * @brief Scalar multiplication.
     * @param s The scalar.
     * @return A new vector scaled by s.
     */
    constexpr vec3<T> operator*(T s) const
    {
        return vec3<T>(x * s, y * s, z * s);
    }

    /**
     * @brief Component-wise division.
     * @param v The other vector.
     * @return A new vector with the component-wise quotient.
     */
    constexpr vec3<T> operator/(const vec3<T> &v) const
    {
        return vec3<T>(x / v.x, y / v.y, z / v.z);
    }

    /**
     * @brief Scalar division.
     * @param s The scalar.
     * @return A new vector divided by s.
     */
    constexpr vec3<T> operator/(T s) const
    {
        return vec3<T>(x / s, y / s, z / s);
    }

    /**
     * @brief Equality operator.
     * @param v The vector to compare with.
     * @return True if vectors are equal (within epsilon), false otherwise.
     */
    constexpr bool operator==(const vec3<T> &v) const
    {
        return is_equal(*this, v);
    }

    /**
     * @brief Inequality operator.
     * @param v The vector to compare with.
     * @return True if vectors are not equal, false otherwise.
     */
    constexpr bool operator!=(const vec3<T> &v) const
    {
        return !is_equal(*this, v);
    }

    /**
     * @brief Access component by index.
     * @param i The index.
     * @return Reference to the component.
     */
    constexpr T &operator[](int i) { return (&x)[i]; }

    /**
     * @brief Access component by index (const).
     * @param i The index.
     * @return Const reference to the component.
     */
    constexpr const T &operator[](int i) const { return (&x)[i]; }

    /** @brief Returns {x, x} */
    constexpr vec2<T> xx() const { return vec2<T>(x, x); }
    /** @brief Returns {x, y} */
    constexpr vec2<T> xy() const { return vec2<T>(x, y); }
    /** @brief Returns {x, z} */
    constexpr vec2<T> xz() const { return vec2<T>(x, z); }

    /** @brief Returns {y, x} */
    constexpr vec2<T> yx() const { return vec2<T>(y, x); }
    /** @brief Returns {y, y} */
    constexpr vec2<T> yy() const { return vec2<T>(y, y); }
    /** @brief Returns {y, z} */
    constexpr vec2<T> yz() const { return vec2<T>(y, z); }

    /** @brief Returns {z, x} */
    constexpr vec2<T> zx() const { return vec2<T>(z, x); }
    /** @brief Returns {z, y} */
    constexpr vec2<T> zy() const { return vec2<T>(z, y); }
    /** @brief Returns {z, z} */
    constexpr vec2<T> zz() const { return vec2<T>(z, z); }
};

/**
 * @brief A structure representing a 4-dimensional vector.
 *
 * This structure provides basic vector operations such as addition,
 * subtraction, multiplication, and division. It is templated to support
 * different numeric types.
 *
 * @tparam T The underlying numeric type of the vector components (e.g., float,
 * double, int).
 */
template <typename T> struct vec4
{
    /** @brief The x component of the vector. */
    T x;
    /** @brief The y component of the vector. */
    T y;
    /** @brief The z component of the vector. */
    T z;
    /** @brief The w component of the vector. */
    T w;

    /**
     * @brief Default constructor. Initializes x, y, z, and w to 0.
     */
    constexpr vec4() : x(0), y(0), z(0), w(0) {}

    /**
     * @brief Constructs a vector with all components set to the same value.
     * @param val The value to set for x, y, z, and w.
     */
    constexpr vec4(T val) : x(val), y(val), z(val), w(val) {}

    /**
     * @brief Constructs a vector with specific values.
     * @param x_val The value for the x component.
     * @param y_val The value for the y component.
     * @param z_val The value for the z component.
     * @param w_val The value for the w component.
     */
    constexpr vec4(T x_val, T y_val, T z_val, T w_val)
        : x(x_val), y(y_val), z(z_val), w(w_val)
    {
    }

    /**
     * @brief Constructs a vector from a vec2 and two scalars.
     * @param xy The vector for the x and y components.
     * @param z The value for the z component.
     * @param w The value for the w component.
     */
    constexpr vec4(const vec2<T> &xy, T z, T w) : x(xy.x), y(xy.y), z(z), w(w)
    {
    }

    /**
     * @brief Constructs a vector from two vec2s.
     * @param xy The vector for the x and y components.
     * @param zw The vector for the z and w components.
     */
    constexpr vec4(const vec2<T> &xy, const vec2<T> &zw)
        : x(xy.x), y(xy.y), z(zw.x), w(zw.y)
    {
    }

    /**
     * @brief Constructs a vector from a scalar, a vec2, and a scalar.
     * @param x The value for the x component.
     * @param yz The vector for the y and z components.
     * @param w The value for the w component.
     */
    constexpr vec4(T x, const vec2<T> &yz, T w) : x(x), y(yz.x), z(yz.y), w(w)
    {
    }

    /**
     * @brief Constructs a vector from two scalars and a vec2.
     * @param x The value for the x component.
     * @param y The value for the y component.
     * @param zw The vector for the z and w components.
     */
    constexpr vec4(T x, T y, const vec2<T> &zw) : x(x), y(y), z(zw.x), w(zw.y)
    {
    }

    /**
     * @brief Constructs a vector from a scalar and a vec3.
     * @param x The value for the x component.
     * @param yzw The vector for the y, z, and w components.
     */
    constexpr vec4(T x, const vec3<T> &yzw) : x(x), y(yzw.x), z(yzw.y), w(yzw.z)
    {
    }

    /**
     * @brief Constructs a vector from a vec3 and a scalar.
     * @param xyz The vector for the x, y, and z components.
     * @param w The value for the w component.
     */
    constexpr vec4(const vec3<T> &xyz, T w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w)
    {
    }

    /**
     * @brief Copy constructor from a vector of a different type.
     * @tparam U The numeric type of the other vector.
     * @param other The other vector to copy from.
     */
    template <typename U>
    constexpr vec4(const vec4<U> &other)
        : x(T((&other.x)[0])), y(T((&other.y)[0])), z(T((&other.z)[0])),
          w(T((&other.w)[0]))
    {
    }

    /**
     * @brief Adds another vector to this vector.
     * @param rhs The vector to add.
     * @return A reference to this vector after modification.
     */
    constexpr vec4<T> &operator+=(const vec4<T> &rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    /**
     * @brief Subtracts another vector from this vector.
     * @param rhs The vector to subtract.
     * @return A reference to this vector after modification.
     */
    constexpr vec4<T> &operator-=(const vec4<T> &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    /**
     * @brief Multiplies this vector by a scalar.
     * @param s The scalar value to multiply by.
     * @return A reference to this vector after modification.
     */
    constexpr vec4<T> &operator*=(T s)
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    /**
     * @brief Divides this vector by a scalar.
     * @param s The scalar value to divide by.
     * @return A reference to this vector after modification.
     */
    constexpr vec4<T> &operator/=(T s)
    {
        x /= s;
        y /= s;
        z /= s;
        w /= s;
        return *this;
    }

    /**
     * @brief Addition operator.
     * @param v The vector to add.
     * @return A new vector containing the sum.
     */
    constexpr vec4<T> operator+(const vec4<T> &v) const
    {
        return vec4<T>(x + v.x, y + v.y, z + v.z, w + v.w);
    }

    /**
     * @brief Subtraction operator.
     * @param v The vector to subtract.
     * @return A new vector containing the difference.
     */
    constexpr vec4<T> operator-(const vec4<T> &v) const
    {
        return vec4<T>(x - v.x, y - v.y, z - v.z, w - v.w);
    }

    /**
     * @brief Unary negation operator.
     * @return A new vector with negated components.
     */
    constexpr vec4<T> operator-() const { return vec4<T>(-x, -y, -z); }

    /**
     * @brief Component-wise multiplication.
     * @param v The other vector.
     * @return A new vector with the component-wise product.
     */
    constexpr vec4<T> operator*(const vec4<T> &v) const
    {
        return vec4<T>(x * v.x, y * v.y, z * v.z, w * v.w);
    }

    /**
     * @brief Scalar multiplication.
     * @param s The scalar.
     * @return A new vector scaled by s.
     */
    constexpr vec4<T> operator*(T s) const
    {
        return vec4<T>(x * s, y * s, z * s, w * s);
    }

    /**
     * @brief Component-wise division.
     * @param v The other vector.
     * @return A new vector with the component-wise quotient.
     */
    constexpr vec4<T> operator/(const vec4<T> &v) const
    {
        return vec4<T>(x / v.x, y / v.y, z / v.z, w / v.w);
    }

    /**
     * @brief Scalar division.
     * @param s The scalar.
     * @return A new vector divided by s.
     */
    constexpr vec4<T> operator/(T s) const
    {
        return vec4<T>(x / s, y / s, z / s, w / s);
    }

    /**
     * @brief Equality operator.
     * @param v The vector to compare with.
     * @return True if vectors are equal (within epsilon), false otherwise.
     */
    constexpr bool operator==(const vec4<T> &v) const
    {
        return is_equal(*this, v);
    }
    /**
     * @brief Inequality operator.
     * @param v The vector to compare with.
     * @return True if vectors are not equal, false otherwise.
     */
    constexpr bool operator!=(const vec4<T> &v) const
    {
        return !is_equal(*this, v);
    }

    /**
     * @brief Access component by index.
     * @param i The index.
     * @return Reference to the component.
     */
    constexpr T &operator[](int i) { return (&x)[i]; }

    /**
     * @brief Access component by index (const).
     * @param i The index.
     * @return Const reference to the component.
     */
    constexpr const T &operator[](int i) const { return (&x)[i]; }

    /** @brief Returns {x, x} */
    constexpr vec2<T> xx() const { return vec2<T>(x, x); }
    /** @brief Returns {x, y} */
    constexpr vec2<T> xy() const { return vec2<T>(x, y); }
    /** @brief Returns {x, z} */
    constexpr vec2<T> xz() const { return vec2<T>(x, z); }
    /** @brief Returns {x, w} */
    constexpr vec2<T> xw() const { return vec2<T>(x, w); }

    /** @brief Returns {y, x} */
    constexpr vec2<T> yx() const { return vec2<T>(y, x); }
    /** @brief Returns {y, y} */
    constexpr vec2<T> yy() const { return vec2<T>(y, y); }
    /** @brief Returns {y, z} */
    constexpr vec2<T> yz() const { return vec2<T>(y, z); }
    /** @brief Returns {y, w} */
    constexpr vec2<T> yw() const { return vec2<T>(y, w); }

    /** @brief Returns {z, x} */
    constexpr vec2<T> zx() const { return vec2<T>(z, x); }
    /** @brief Returns {z, y} */
    constexpr vec2<T> zy() const { return vec2<T>(z, y); }
    /** @brief Returns {z, z} */
    constexpr vec2<T> zz() const { return vec2<T>(z, z); }
    /** @brief Returns {z, w} */
    constexpr vec2<T> zw() const { return vec2<T>(z, w); }

    /** @brief Returns {w, x} */
    constexpr vec2<T> wx() const { return vec2<T>(w, x); }
    /** @brief Returns {w, y} */
    constexpr vec2<T> wy() const { return vec2<T>(w, y); }
    /** @brief Returns {w, z} */
    constexpr vec2<T> wz() const { return vec2<T>(w, z); }
    /** @brief Returns {w, w} */
    constexpr vec2<T> ww() const { return vec2<T>(w, w); }

    /**
     * @brief 3-component swizzle operators.
     *
     * These methods return a new vec3 constructed from the specified components
     * of this vec4. The method names indicate the components used (e.g., xyz()
     * uses x, y, and z).
     */
    constexpr vec3<T> xxx() const { return vec3<T>(x, x, x); }
    constexpr vec3<T> xxy() const { return vec3<T>(x, x, y); }
    constexpr vec3<T> xxz() const { return vec3<T>(x, x, z); }
    constexpr vec3<T> xxw() const { return vec3<T>(x, x, w); }
    constexpr vec3<T> xyx() const { return vec3<T>(x, y, x); }
    constexpr vec3<T> xyy() const { return vec3<T>(x, y, y); }
    constexpr vec3<T> xyz() const { return vec3<T>(x, y, z); }
    constexpr vec3<T> xyw() const { return vec3<T>(x, y, w); }
    constexpr vec3<T> xzx() const { return vec3<T>(x, z, x); }
    constexpr vec3<T> xzy() const { return vec3<T>(x, z, y); }
    constexpr vec3<T> xzz() const { return vec3<T>(x, z, z); }
    constexpr vec3<T> xzw() const { return vec3<T>(x, z, w); }
    constexpr vec3<T> xwx() const { return vec3<T>(x, w, x); }
    constexpr vec3<T> xwy() const { return vec3<T>(x, w, y); }
    constexpr vec3<T> xwz() const { return vec3<T>(x, w, z); }
    constexpr vec3<T> xww() const { return vec3<T>(x, w, w); }

    constexpr vec3<T> yxx() const { return vec3<T>(y, x, x); }
    constexpr vec3<T> yxy() const { return vec3<T>(y, x, y); }
    constexpr vec3<T> yxz() const { return vec3<T>(y, x, z); }
    constexpr vec3<T> yxw() const { return vec3<T>(y, x, w); }
    constexpr vec3<T> yyx() const { return vec3<T>(y, y, x); }
    constexpr vec3<T> yyy() const { return vec3<T>(y, y, y); }
    constexpr vec3<T> yyz() const { return vec3<T>(y, y, z); }
    constexpr vec3<T> yyw() const { return vec3<T>(y, y, w); }
    constexpr vec3<T> yzx() const { return vec3<T>(y, z, x); }
    constexpr vec3<T> yzy() const { return vec3<T>(y, z, y); }
    constexpr vec3<T> yzz() const { return vec3<T>(y, z, z); }
    constexpr vec3<T> yzw() const { return vec3<T>(y, z, w); }
    constexpr vec3<T> ywx() const { return vec3<T>(y, w, x); }
    constexpr vec3<T> ywy() const { return vec3<T>(y, w, y); }
    constexpr vec3<T> ywz() const { return vec3<T>(y, w, z); }
    constexpr vec3<T> yww() const { return vec3<T>(y, w, w); }

    constexpr vec3<T> zxx() const { return vec3<T>(z, x, x); }
    constexpr vec3<T> zxy() const { return vec3<T>(z, x, y); }
    constexpr vec3<T> zxz() const { return vec3<T>(z, x, z); }
    constexpr vec3<T> zxw() const { return vec3<T>(z, x, w); }
    constexpr vec3<T> zyx() const { return vec3<T>(z, y, x); }
    constexpr vec3<T> zyy() const { return vec3<T>(z, y, y); }
    constexpr vec3<T> zyz() const { return vec3<T>(z, y, z); }
    constexpr vec3<T> zyw() const { return vec3<T>(z, y, w); }
    constexpr vec3<T> zzx() const { return vec3<T>(z, z, x); }
    constexpr vec3<T> zzy() const { return vec3<T>(z, z, y); }
    constexpr vec3<T> zzz() const { return vec3<T>(z, z, z); }
    constexpr vec3<T> zzw() const { return vec3<T>(z, z, w); }
    constexpr vec3<T> zwx() const { return vec3<T>(z, w, x); }
    constexpr vec3<T> zwy() const { return vec3<T>(z, w, y); }
    constexpr vec3<T> zwz() const { return vec3<T>(z, w, z); }
    constexpr vec3<T> zww() const { return vec3<T>(z, w, w); }

    constexpr vec3<T> wxx() const { return vec3<T>(w, x, x); }
    constexpr vec3<T> wxy() const { return vec3<T>(w, x, y); }
    constexpr vec3<T> wxz() const { return vec3<T>(w, x, z); }
    constexpr vec3<T> wxw() const { return vec3<T>(w, x, w); }
    constexpr vec3<T> wyx() const { return vec3<T>(w, y, x); }
    constexpr vec3<T> wyy() const { return vec3<T>(w, y, y); }
    constexpr vec3<T> wyz() const { return vec3<T>(w, y, z); }
    constexpr vec3<T> wyw() const { return vec3<T>(w, y, w); }
    constexpr vec3<T> wzx() const { return vec3<T>(w, z, x); }
    constexpr vec3<T> wzy() const { return vec3<T>(w, z, y); }
    constexpr vec3<T> wzz() const { return vec3<T>(w, z, z); }
    constexpr vec3<T> wzw() const { return vec3<T>(w, z, w); }
    constexpr vec3<T> wwx() const { return vec3<T>(w, w, x); }
    constexpr vec3<T> wwy() const { return vec3<T>(w, w, y); }
    constexpr vec3<T> wwz() const { return vec3<T>(w, w, z); }
    constexpr vec3<T> www() const { return vec3<T>(w, w, w); }
};

/**
 * @brief A structure representing a quaternion.
 *
 * Quaternions are used for representing rotations.
 *
 * @tparam T The underlying numeric type.
 */
template <typename T> struct quat
{
    union
    {
        struct
        {
            /** @brief The x, y, z, and w components of the quaternion. */
            T x, y, z, w;
        };
        /** @brief The quaternion components accessed as a vec4. */
        vec4<T> as_vec4;
    };

    /**
     * @brief Default constructor. Initializes identity quaternion (0, 0, 0, 1).
     */
    constexpr quat() : x(0), y(0), z(0), w(1) {}

    /**
     * @brief Constructs a new quaternion with explicit components.
     * @param x_val X component.
     * @param y_val Y component.
     * @param z_val Z component.
     * @param w_val W component.
     */
    constexpr quat(T x_val, T y_val, T z_val, T w_val)
        : x(x_val), y(y_val), z(z_val), w(w_val)
    {
    }

    /**
     * @brief Constructs a quaternion from a vector part and a scalar part.
     * @param xyz The vector part (imaginary components).
     * @param w The scalar part (real component).
     */
    constexpr quat(const vec3<T> &xyz, T w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w)
    {
    }

    /**
     * @brief Multiplies this quaternion by a scalar.
     * @param s The scalar to multiply by.
     * @return A reference to this quaternion after modification.
     */
    constexpr quat<T> &operator*=(T s)
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    /**
     * @brief Divides this quaternion by a scalar.
     * @param s The scalar to divide by.
     * @return A reference to this quaternion after modification.
     */
    constexpr quat<T> &operator/=(T s)
    {
        x /= s;
        y /= s;
        z /= s;
        w /= s;
        return *this;
    }

    /**
     * @brief Scalar multiplication.
     * @param s The scalar.
     * @return A new quaternion scaled by s.
     */
    constexpr quat<T> operator*(T s) const
    {
        return quat<T>(x * s, y * s, z * s, w * s);
    }

    /**
     * @brief Scalar division.
     * @param s The scalar.
     * @return A new quaternion divided by s.
     */
    constexpr quat<T> operator/(T s) const
    {
        return quat<T>(x / s, y / s, z / s, w / s);
    }

    /**
     * @brief Integrates the quaternion by an angular velocity over a time step
     * (Euler method).
     * @param dv The angular velocity vector.
     * @param dt The time step.
     */
    constexpr void integrate(const vec3<T> &dv, T dt);

    /**
     * @brief Converts the quaternion to axis and angle representation.
     * @param axis Output axis.
     * @param angle Output angle (radians).
     */
    constexpr void to_axis_angle(vec3<T> &axis, T &angle) const;

    /**
     * @brief Equality operator.
     * @param q The quaternion to compare with.
     * @return True if quaternions are equal, false otherwise.
     */
    constexpr bool operator==(const quat<T> &q) const
    {
        return is_equal(*this, q);
    }
    /**
     * @brief Inequality operator.
     * @param q The quaternion to compare with.
     * @return True if quaternions are not equal, false otherwise.
     */
    constexpr bool operator!=(const quat<T> &q) const
    {
        return !is_equal(*this, q);
    }
};

/**
 * @brief A structure representing a 3x3 matrix.
 *
 * Stored in column-major order.
 *
 * @tparam T The underlying numeric type.
 */
template <typename T> struct mat3
{
    // Column-major layout
    union
    {
        struct
        {
            /** @brief Matrix elements (column-major: mColRow). */
            T m00, m10, m20;
            T m01, m11, m21;
            T m02, m12, m22;
        };
        vec3<T> columns[3];
    };

    /**
     * @brief Default constructor. Initializes all elements to 0.
     */
    constexpr mat3()
    {
        m00 = m10 = m20 = {};
        m01 = m11 = m21 = {};
        m02 = m12 = m22 = {};
    }

    /**
     * @brief Constructs a matrix from three column vectors.
     * @tparam U The numeric type of the vectors.
     * @param a The first column vector.
     * @param b The second column vector.
     * @param c The third column vector.
     */
    template <typename U>
    constexpr mat3(const vec3<U> &a, const vec3<U> &b, const vec3<U> &c)
    {
        columns[0] = a;
        columns[1] = b;
        columns[2] = c;
    }

    /**
     * @brief Copy constructor from a matrix of a different numeric type.
     * @tparam U The numeric type of the other matrix.
     * @param other The other matrix.
     */
    template <typename U> constexpr mat3(const mat3<U> &other)
    {
        columns[0] = other.columns[0];
        columns[1] = other.columns[1];
        columns[2] = other.columns[2];
    }

    /**
     * @brief Creates an identity matrix.
     * @return A 3x3 identity matrix.
     */
    constexpr static mat3<T> identity()
    {
        mat3<T> m;
        m.m00 = m.m11 = m.m22 = T(1);
        return m;
    }

    /**
     * @brief Constructs a rotation matrix from a quaternion.
     * @param q The quaternion.
     */
    constexpr mat3(const quat<T> &q)
    {
        T xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
        T xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        T wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

        columns[0].x = T(1) - T(2) * (yy + zz);
        columns[0].y = T(2) * (xy + wz);
        columns[0].z = T(2) * (xz - wy);

        columns[1].x = T(2) * (xy - wz);
        columns[1].y = T(1) - T(2) * (xx + zz);
        columns[1].z = T(2) * (yz + wx);

        columns[2].x = T(2) * (xz + wy);
        columns[2].y = T(2) * (yz - wx);
        columns[2].z = T(1) - T(2) * (xx + yy);
    }

    /**
     * @brief Adds another matrix to this matrix.
     * @param rhs The matrix to add.
     * @return A reference to this matrix after modification.
     */
    constexpr mat3<T> &operator+=(const mat3<T> &rhs)
    {
        columns[0] += rhs.columns[0];
        columns[1] += rhs.columns[1];
        columns[2] += rhs.columns[2];
        return *this;
    }

    /**
     * @brief Subtracts another matrix from this matrix.
     * @param rhs The matrix to subtract.
     * @return A reference to this matrix after modification.
     */
    constexpr mat3<T> &operator-=(const mat3<T> &rhs)
    {
        columns[0] -= rhs.columns[0];
        columns[1] -= rhs.columns[1];
        columns[2] -= rhs.columns[2];
        return *this;
    }

    /**
     * @brief Divides this matrix by a scalar.
     * @param s The scalar to divide by.
     * @return A reference to this matrix after modification.
     */
    constexpr mat3<T> &operator/=(T s)
    {
        columns[0] /= s;
        columns[1] /= s;
        columns[2] /= s;
        return *this;
    }

    /**
     * @brief Access column by index.
     * @param i The column index.
     * @return Reference to the column.
     */
    vec3<T> &operator[](int i) { return columns[i]; }

    /**
     * @brief Access column by index (const).
     * @param i The column index.
     * @return Const reference to the column.
     */
    const vec3<T> &operator[](int i) const { return columns[i]; }

    /**
     * @brief Get a row as a vector.
     * @param i The row index.
     * @return The row vector.
     */
    constexpr vec3<T> row(int i) const
    {
        return vec3<T>(columns[0][i], columns[1][i], columns[2][i]);
    }

    /**
     * @brief Set a row from a vector.
     * @param i The row index.
     * @param v The vector to set.
     */
    constexpr void set_row(int i, const vec3<T> &v)
    {
        m20 = v.x;
        m21 = v.y;
        m22 = v.z;
    }
};

/**
 * @brief A structure representing a 4x4 matrix.
 *
 * Stored in column-major order.
 *
 * @tparam T The underlying numeric type.
 */
template <typename T> struct mat4
{
    // Column-major layout
    union
    {
        struct
        {
            /** @brief Matrix elements (column-major: mColRow). */
            T m00, m10, m20, m30;
            T m01, m11, m21, m31;
            T m02, m12, m22, m32;
            T m03, m13, m23, m33;
        };
        /** @brief Matrix elements accessed as a 2D array [col][row]. */
        T m[4][4];
    };

    /**
     * @brief Default constructor. Initializes all elements to 0.
     */
    constexpr mat4()
    {
        for (int i = 0; i < 16; ++i)
            (&m00)[i] = {};
    }

    /**
     * @brief Copy constructor from a matrix of a different numeric type.
     * @tparam U The numeric type of the other matrix.
     * @param other The other matrix.
     */
    template <typename U> constexpr mat4(const mat4<U> &other)
    {
        for (int i = 0; i < 16; ++i)
            (&m00)[i] = T((&other.m00)[i]);
    }

    /**
     * @brief Creates an identity matrix.
     * @return A 4x4 identity matrix.
     */
    constexpr static mat4<T> identity()
    {
        mat4<T> m;
        m.m00 = m.m11 = m.m22 = m.m33 = T(1);
        return m;
    }

    /**
     * @brief Adds another matrix to this matrix.
     * @param rhs The matrix to add.
     * @return A reference to this matrix after modification.
     */
    constexpr mat4<T> &operator+=(const mat4<T> &rhs)
    {
        m00 += rhs.m00;
        m01 += rhs.m01;
        m02 += rhs.m02;
        m03 += rhs.m03;
        m10 += rhs.m10;
        m11 += rhs.m11;
        m12 += rhs.m12;
        m13 += rhs.m13;
        m20 += rhs.m20;
        m21 += rhs.m21;
        m22 += rhs.m22;
        m23 += rhs.m23;
        m30 += rhs.m30;
        m31 += rhs.m31;
        m32 += rhs.m32;
        m33 += rhs.m33;
        return *this;
    }

    /**
     * @brief Subtracts another matrix from this matrix.
     * @param rhs The matrix to subtract.
     * @return A reference to this matrix after modification.
     */
    constexpr mat4<T> &operator-=(const mat4<T> &rhs)
    {
        m00 -= rhs.m00;
        m01 -= rhs.m01;
        m02 -= rhs.m02;
        m03 -= rhs.m03;
        m10 -= rhs.m10;
        m11 -= rhs.m11;
        m12 -= rhs.m12;
        m13 -= rhs.m13;
        m20 -= rhs.m20;
        m21 -= rhs.m21;
        m22 -= rhs.m22;
        m23 -= rhs.m23;
        m30 -= rhs.m30;
        m31 -= rhs.m31;
        m32 -= rhs.m32;
        m33 -= rhs.m33;
        return *this;
    }

    /**
     * @brief Divides this matrix by a scalar.
     * @param s The scalar to divide by.
     * @return A reference to this matrix after modification.
     */
    constexpr mat4<T> &operator/=(T s)
    {
        m00 /= s;
        m01 /= s;
        m02 /= s;
        m03 /= s;
        m10 /= s;
        m11 /= s;
        m12 /= s;
        m13 /= s;
        m20 /= s;
        m21 /= s;
        m22 /= s;
        m23 /= s;
        m30 /= s;
        m31 /= s;
        m32 /= s;
        m33 /= s;
        return *this;
    }

    /**
     * @brief Access column by index.
     * @param i The column index.
     * @return Reference to the column.
     */
    vec4<T> &operator[](int i) { return reinterpret_cast<vec4<T> &>(m[i]); }

    /**
     * @brief Access column by index (const).
     * @param i The column index.
     * @return Const reference to the column.
     */
    const vec4<T> &operator[](int i) const
    {
        return reinterpret_cast<const vec4<T> &>(m[i]);
    }

    /**
     * @brief Get a row as a vector.
     * @param i The row index.
     * @return The row vector.
     */
    constexpr vec4<T> row(int i) const
    {
        return vec4<T>(m[0][i], m[1][i], m[2][i], m[3][i]);
    }

    /**
     * @brief Set a row from a vector.
     * @param i The row index.
     * @param v The vector to set.
     */
    constexpr void set_row(int i, const vec4<T> &v)
    {
        m[0][i] = v.x;
        m[1][i] = v.y;
        m[2][i] = v.z;
        m[3][i] = v.w;
    }
};

/**
 * @brief A structure representing a plane in 3D space.
 *
 * Defined by a normal vector and a distance from the origin (Hessian normal
 * form).
 *
 * @tparam T The underlying numeric type.
 */
template <typename T> struct plane3
{
    /** @brief The normal vector of the plane. */
    vec3<T> normal;
    /** @brief The distance from the origin to the plane. */
    T d;

    /**
     * @brief Default constructor. Initializes plane with normal (0, 1, 0) and
     * distance 0.
     */
    constexpr plane3() : normal(0, 1, 0), d(0) {}

    /**
     * @brief Constructs a plane from a normal and a distance.
     * @param n The normal vector.
     * @param d_val The distance value.
     */
    constexpr plane3(const vec3<T> &n, T d_val) : normal(n), d(d_val) {}

    /**
     * @brief Constructs a plane from normal components and a distance.
     * @param a The x component of the normal.
     * @param b The y component of the normal.
     * @param c The z component of the normal.
     * @param d_val The distance value.
     */
    constexpr plane3(T a, T b, T c, T d_val) : normal(a, b, c), d(d_val) {}
};

/**
 * @brief Checks if all components of the vector are zero (within epsilon).
 * @param v The vector to check.
 * @return True if zero, false otherwise.
 */
template <typename T> constexpr bool is_zero(const vec2<T> &v)
{
    return abs(v.x) < T::epsilon() && abs(v.y) < T::epsilon();
}

/**
 * @brief Checks if two vectors are equal (within epsilon).
 * @param a The first vector.
 * @param b The second vector.
 * @return True if equal, false otherwise.
 */
template <typename T>
constexpr bool is_equal(const vec2<T> &a, const vec2<T> &b)
{
    return abs(a.x - b.x) < T::epsilon() && abs(a.y - b.y) < T::epsilon();
}

/**
 * @brief Calculates the dot product of two vectors.
 * @param v1 The first vector.
 * @param v2 The second vector.
 * @return The dot product.
 */
template <typename T> constexpr T dot(const vec2<T> &v1, const vec2<T> &v2)
{
    return v1.x * v2.x + v1.y * v2.y;
}

/**
 * @brief Calculates the squared length of the vector.
 * @param v The vector.
 * @return The squared length.
 */
template <typename T> constexpr T length_sq(const vec2<T> &v)
{
    return dot(v, v);
}

/**
 * @brief Calculates the length of the vector.
 * @param v The vector.
 * @return The length.
 */
template <typename T> constexpr T length(const vec2<T> &v)
{
    return sqrt(length_sq(v));
}

/**
 * @brief Calculates the squared distance between two vectors.
 * @param a The first vector.
 * @param b The second vector.
 * @return The squared distance.
 */
template <typename T>
constexpr T distance_sq(const vec2<T> &a, const vec2<T> &b)
{
    return length_sq(a - b);
}

/**
 * @brief Calculates the distance between two vectors.
 * @param a The first vector.
 * @param b The second vector.
 * @return The distance.
 */
template <typename T> constexpr T distance(const vec2<T> &a, const vec2<T> &b)
{
    return length(a - b);
}

/**
 * @brief Normalizes the vector (returns a vector with the same direction and
 * length 1).
 * @param v The vector to normalize.
 * @return The normalized vector.
 */
template <typename T> constexpr vec2<T> normalize(const vec2<T> &v)
{
    return v / length(v);
}

/**
 * @brief Reflects a vector off a surface defined by a normal.
 * @param v The incident vector.
 * @param n The normal vector of the surface.
 * @return The reflected vector.
 */
template <typename T>
constexpr vec2<T> reflect(const vec2<T> &v, const vec2<T> &n)
{
    return v - n * T(2) * dot(v, n);
}

/**
 * @brief Linearly interpolates between two vectors.
 * @param a The starting vector.
 * @param b The ending vector.
 * @param t The interpolation factor (0.0 to 1.0).
 * @return The interpolated vector.
 */
template <typename T>
constexpr vec2<T> lerp(const vec2<T> &a, const vec2<T> &b, T t)
{
    return a + (b - a) * t;
}

/**
 * @brief Component-wise absolute value.
 * @param a The vector.
 * @return A new vector with absolute values of components.
 */
template <typename T> constexpr vec2<T> abs(const vec2<T> &a)
{
    return {abs(a.x), abs(a.y)};
}

/**
 * @brief Component-wise minimum value.
 * @param a The vector.
 * @return A new vector with minimum values of components.
 */
template <typename T> constexpr vec2<T> min(const vec2<T> &a)
{
    return {min(a.x), min(a.y)};
}

/**
 * @brief Component-wise maximum value.
 * @param a The vector.
 * @return A new vector with maximum values of components.
 */
template <typename T> constexpr vec2<T> max(const vec2<T> &a)
{
    return {max(a.x), max(a.y)};
}

/**
 * @brief Clamps a vector component-wise between a min and max vector.
 * @param v The vector to clamp.
 * @param min_v The minimum values.
 * @param max_v The maximum values.
 * @return The clamped vector.
 */
template <typename T>
constexpr vec2<T>
clamp(const vec2<T> &v, const vec2<T> &min_v, const vec2<T> &max_v)
{
    return vec2<T>(clamp(v.x, min_v.x, max_v.x), clamp(v.y, min_v.y, max_v.y));
}

/**
 * @brief Checks if all components of the vector are zero (within epsilon).
 * @param v The vector to check.
 * @return True if zero, false otherwise.
 */
template <typename T> constexpr bool is_zero(const vec3<T> &v)
{
    return abs(v.x) < T::epsilon() && abs(v.y) < T::epsilon() &&
           abs(v.z) < T::epsilon();
}

/**
 * @brief Checks if two vectors are equal (within epsilon).
 * @param a The first vector.
 * @param b The second vector.
 * @return True if equal, false otherwise.
 */
template <typename T>
constexpr bool is_equal(const vec3<T> &a, const vec3<T> &b)
{
    return abs(a.x - b.x) < T::epsilon() && abs(a.y - b.y) < T::epsilon() &&
           abs(a.z - b.z) < T::epsilon();
}
/**
 * @brief Calculates the dot product of two vectors.
 * @param v1 The first vector.
 * @param v2 The second vector.
 * @return The dot product.
 */
template <typename T> constexpr T dot(const vec3<T> &v1, const vec3<T> &v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
/**
 * @brief Calculates the squared length of the vector.
 * @param v The vector.
 * @return The squared length.
 */
template <typename T> constexpr T length_sq(const vec3<T> &v)
{
    return dot(v, v);
}

/**
 * @brief Calculates the length of the vector.
 * @param v The vector.
 * @return The length.
 */
template <typename T> constexpr T length(const vec3<T> &v)
{
    return sqrt(length_sq(v));
}

/**
 * @brief Calculates the squared distance between two vectors.
 * @param a The first vector.
 * @param b The second vector.
 * @return The squared distance.
 */
template <typename T>
constexpr T distance_sq(const vec3<T> &a, const vec3<T> &b)
{
    return length_sq(a - b);
}

/**
 * @brief Calculates the distance between two vectors.
 * @param a The first vector.
 * @param b The second vector.
 * @return The distance.
 */
template <typename T> constexpr T distance(const vec3<T> &a, const vec3<T> &b)
{
    return length(a - b);
}

/**
 * @brief Normalizes the vector (returns a vector with the same direction and
 * length 1).
 * @param v The vector to normalize.
 * @return The normalized vector.
 */
template <typename T> constexpr vec3<T> normalize(const vec3<T> &v)
{
    return v / length(v);
}

/**
 * @brief Calculates the cross product of two vectors.
 * @param v1 The first vector.
 * @param v2 The second vector.
 * @return The cross product vector.
 */
template <typename T>
constexpr vec3<T> cross(const vec3<T> &v1, const vec3<T> &v2)
{
    return vec3<T>(v1.y * v2.z - v1.z * v2.y,
                   v1.z * v2.x - v1.x * v2.z,
                   v1.x * v2.y - v1.y * v2.x);
}

/**
 * @brief Reflects a vector off a surface defined by a normal.
 * @param v The incident vector.
 * @param n The normal vector of the surface.
 * @return The reflected vector.
 */
template <typename T>
constexpr vec3<T> reflect(const vec3<T> &v, const vec3<T> &n)
{
    return v - n * T(2) * dot(v, n);
}

/**
 * @brief Linearly interpolates between two vectors.
 * @param a The starting vector.
 * @param b The ending vector.
 * @param t The interpolation factor (0.0 to 1.0).
 * @return The interpolated vector.
 */
template <typename T>
constexpr vec3<T> lerp(const vec3<T> &a, const vec3<T> &b, T t)
{
    return a + (b - a) * t;
}

/**
 * @brief Component-wise absolute value.
 * @param a The vector.
 * @return A new vector with absolute values of components.
 */
template <typename T> constexpr vec3<T> abs(const vec3<T> &a)
{
    return {abs(a.x), abs(a.y), abs(a.z)};
}

/**
 * @brief Component-wise minimum value.
 * @param a The first vector.
 * @param b The second vector.
 * @return A new vector with minimum values of components.
 */
template <typename T> constexpr vec3<T> min(const vec3<T> &a, const vec3<T> &b)
{
    return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)};
}

/**
 * @brief Component-wise maximum value.
 * @param a The first vector.
 * @param b The second vector.
 * @return A new vector with maximum values of components.
 */
template <typename T> constexpr vec3<T> max(const vec3<T> &a, const vec3<T> &b)
{
    return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)};
}

/**
 * @brief Clamps a vector component-wise between a min and max vector.
 * @param v The vector to clamp.
 * @param min_v The minimum values.
 * @param max_v The maximum values.
 * @return The clamped vector.
 */
template <typename T>
constexpr vec3<T>
clamp(const vec3<T> &v, const vec3<T> &min_v, const vec3<T> &max_v)
{
    return vec3<T>(clamp(v.x, min_v.x, max_v.x),
                   clamp(v.y, min_v.y, max_v.y),
                   clamp(v.z, min_v.z, max_v.z));
}

/**
 * @brief Checks if all components of the vector are zero (within epsilon).
 * @param v The vector to check.
 * @return True if zero, false otherwise.
 */
template <typename T> constexpr bool is_zero(const vec4<T> &v)
{
    return abs(v.x) < T::epsilon() && abs(v.y) < T::epsilon() &&
           abs(v.z) < T::epsilon() && abs(v.w) < T::epsilon();
}

/**
 * @brief Checks if two vectors are equal (within epsilon).
 * @param a The first vector.
 * @param b The second vector.
 * @return True if equal, false otherwise.
 */
template <typename T>
constexpr bool is_equal(const vec4<T> &a, const vec4<T> &b)
{
    return abs(a.x - b.x) < T::epsilon() && abs(a.y - b.y) < T::epsilon() &&
           abs(a.z - b.z) < T::epsilon() && abs(a.w - b.w) < T::epsilon();
}

/**
 * @brief Calculates the dot product of two vectors.
 * @param v1 The first vector.
 * @param v2 The second vector.
 * @return The dot product.
 */
template <typename T> constexpr T dot(const vec4<T> &v1, const vec4<T> &v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

/**
 * @brief Calculates the squared length of the vector.
 * @param v The vector.
 * @return The squared length.
 */
template <typename T> constexpr T length_sq(const vec4<T> &v)
{
    return dot(v, v);
}

/**
 * @brief Calculates the length of the vector.
 * @param v The vector.
 * @return The length.
 */
template <typename T> constexpr T length(const vec4<T> &v)
{
    return sqrt(length_sq(v));
}

/**
 * @brief Calculates the squared distance between two vectors.
 * @param a The first vector.
 * @param b The second vector.
 * @return The squared distance.
 */
template <typename T>
constexpr T distance_sq(const vec4<T> &a, const vec4<T> &b)
{
    return length_sq(a - b);
}

/**
 * @brief Calculates the distance between two vectors.
 * @param a The first vector.
 * @param b The second vector.
 * @return The distance.
 */
template <typename T> constexpr T distance(const vec4<T> &a, const vec4<T> &b)
{
    return length(a - b);
}

/**
 * @brief Normalizes the vector (returns a vector with the same direction and
 * length 1).
 * @param v The vector to normalize.
 * @return The normalized vector.
 */
template <typename T> constexpr vec4<T> normalize(const vec4<T> &v)
{
    return v / length(v);
}

/**
 * @brief Reflects a vector off a surface defined by a normal.
 * @param v The incident vector.
 * @param n The normal vector of the surface.
 * @return The reflected vector.
 */
template <typename T>
constexpr vec4<T> reflect(const vec4<T> &v, const vec4<T> &n)
{
    return v - n * T(2) * dot(v, n);
}

/**
 * @brief Linearly interpolates between two vectors.
 * @param a The starting vector.
 * @param b The ending vector.
 * @param t The interpolation factor (0.0 to 1.0).
 * @return The interpolated vector.
 */
template <typename T>
constexpr vec4<T> lerp(const vec4<T> &a, const vec4<T> &b, T t)
{
    return a + (b - a) * t;
}

/**
 * @brief Component-wise absolute value.
 * @param a The vector.
 * @return A new vector with absolute values of components.
 */
template <typename T> constexpr vec4<T> abs(const vec4<T> &a)
{
    return {abs(a.x), abs(a.y), abs(a.z), abs(a.w)};
}

/**
 * @brief Component-wise minimum value.
 * @param a The first vector.
 * @param b The second vector.
 * @return A new vector with minimum values of components.
 */
template <typename T> constexpr vec4<T> min(const vec4<T> &a, const vec4<T> &b)
{
    return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)};
}

/**
 * @brief Component-wise maximum value.
 * @param a The first vector.
 * @param b The second vector.
 * @return A new vector with maximum values of components.
 */
template <typename T> constexpr vec4<T> max(const vec4<T> &a, const vec4<T> &b)
{
    return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)};
}

/**
 * @brief Clamps a vector component-wise between a min and max vector.
 * @param v The vector to clamp.
 * @param min_v The minimum values.
 * @param max_v The maximum values.
 * @return The clamped vector.
 */
template <typename T>
constexpr vec4<T>
clamp(const vec4<T> &v, const vec4<T> &min_v, const vec4<T> &max_v)
{
    return vec4<T>(clamp(v.x, min_v.x, max_v.x),
                   clamp(v.y, min_v.y, max_v.y),
                   clamp(v.z, min_v.z, max_v.z),
                   clamp(v.w, min_v.w, max_v.w));
}

/**
 * @brief Checks if all components of the quaternion are zero (within epsilon).
 * @param q The quaternion to check.
 * @return True if zero, false otherwise.
 */
template <typename T> constexpr bool is_zero(const quat<T> &q)
{
    return abs(q.x) < T::epsilon() && abs(q.y) < T::epsilon() &&
           abs(q.z) < T::epsilon() && abs(q.w) < T::epsilon();
}

/**
 * @brief Checks if two quaternions are equal (within epsilon).
 * @param a The first quaternion.
 * @param b The second quaternion.
 * @return True if equal, false otherwise.
 */
template <typename T>
constexpr bool is_equal(const quat<T> &a, const quat<T> &b)
{
    return is_zero(a.as_vec4 - b.as_vec4);
}

/**
 * @brief Calculates the dot product of two quaternions.
 * @param q1 The first quaternion.
 * @param q2 The second quaternion.
 * @return The dot product.
 */
template <typename T> constexpr T dot(const quat<T> &q1, const quat<T> &q2)
{
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

/**
 * @brief Calculates the squared length of the quaternion.
 * @param q The quaternion.
 * @return The squared length.
 */
template <typename T> constexpr T length_sq(const quat<T> &q)
{
    return dot(q, q);
}

/**
 * @brief Calculates the length of the quaternion.
 * @param q The quaternion.
 * @return The length.
 */
template <typename T> constexpr T length(const quat<T> &q)
{
    return sqrt(length_sq(q));
}

/**
 * @brief Normalizes the quaternion (returns a unit quaternion).
 * @param q The quaternion to normalize.
 * @return The normalized quaternion.
 */
template <typename T> constexpr quat<T> normalize(const quat<T> &q)
{
    T inv_len = T(1) / length(q);
    return quat<T>(q.x * inv_len, q.y * inv_len, q.z * inv_len, q.w * inv_len);
}

/**
 * @brief Calculates the conjugate of the quaternion.
 * @param q The quaternion.
 * @return The conjugate quaternion.
 */
template <typename T> constexpr quat<T> conjugate(const quat<T> &q)
{
    return quat<T>(-q.x, -q.y, -q.z, q.w);
}

/**
 * @brief Calculates the inverse of the quaternion.
 * @param q The quaternion.
 * @return The inverse quaternion.
 */
template <typename T> constexpr quat<T> inverse(const quat<T> &q)
{
    return conjugate(q) * (T(1) / length_sq(q));
}

/**
 * @brief Multiplies two quaternions.
 * @param q1 The first quaternion.
 * @param q2 The second quaternion.
 * @return The product of the two quaternions.
 */
template <typename T>
constexpr quat<T> operator*(const quat<T> &q1, const quat<T> &q2)
{
    return quat<T>(q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
                   q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
                   q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
                   q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z);
}

/**
 * @brief Multiplies a quaternion by a vector (rotates the vector).
 * @param q The quaternion.
 * @param v The vector to rotate.
 * @return The rotated vector.
 */
template <typename T>
constexpr vec3<T> operator*(const quat<T> &q, const vec3<T> &v)
{
    vec3<T> q_vec(q.x, q.y, q.z);
    vec3<T> uv  = cross(q_vec, v);
    vec3<T> uuv = cross(q_vec, uv);

    return v + ((uv * q.w) + uuv) * T(2);
}

/**
 * @brief Multiplies a vector by a quaternion (rotates the vector).
 * @param v The vector to rotate.
 * @param q The quaternion.
 * @return The rotated vector.
 */
template <typename T>
constexpr vec3<T> operator*(const vec3<T> &v, const quat<T> &q)
{
    vec3<T> q_vec(q.x, q.y, q.z);
    vec3<T> uv  = cross(q_vec, v);
    vec3<T> uuv = cross(q_vec, uv);

    return v + (uuv - (uv * q.w)) * T(2);
}

/**
 * @brief Integrates the quaternion by an angular velocity over a time step.
 * @param dv The angular velocity vector.
 * @param dt The time step.
 */
template <typename T> constexpr void quat<T>::integrate(const vec3<T> &dv, T dt)
{
    quat<T> q(dv.x * dt, dv.y * dt, dv.z * dt, T(0));
    q = q * (*this);

    x += q.x * T(0.5);
    y += q.y * T(0.5);
    z += q.z * T(0.5);
    w += q.w * T(0.5);

    *this = normalize(*this);
}

/**
 * @brief Converts the quaternion to axis and angle representation.
 * @param axis Output axis.
 * @param angle Output angle (radians).
 */
template <typename T>
constexpr void quat<T>::to_axis_angle(vec3<T> &axis, T &angle) const
{
    quat<T> q = *this;
    if (q.w > T(1))
        q = normalize(q);

    angle = T(2) * acos(q.w);
    T s   = sqrt(T(1) - q.w * q.w);

    if (s < T::epsilon())
    {
        axis.x = T(1);
        axis.y = T(0);
        axis.z = T(0);
    }
    else
    {
        axis.x = q.x / s;
        axis.y = q.y / s;
        axis.z = q.z / s;
    }
}

/**
 * @brief Creates a quaternion from an axis and an angle.
 * @param axis The rotation axis.
 * @param angle The rotation angle (in radians).
 * @return The resulting quaternion.
 */
template <typename T>
constexpr quat<T> quat_from_axis_angle(const vec3<T> &axis, T angle)
{
    T half_angle   = angle * T(0.5);
    auto [s, c]    = sincos(half_angle);
    vec3<T> n_axis = normalize(axis);
    return quat<T>(n_axis.x * s, n_axis.y * s, n_axis.z * s, c);
}

/**
 * @brief Creates a quaternion from a rotation matrix components.
 * @param m00 Matrix element 0,0.
 * @param m01 Matrix element 0,1.
 * @param m02 Matrix element 0,2.
 * @param m10 Matrix element 1,0.
 * @param m11 Matrix element 1,1.
 * @param m12 Matrix element 1,2.
 * @param m20 Matrix element 2,0.
 * @param m21 Matrix element 2,1.
 * @param m22 Matrix element 2,2.
 * @return The resulting quaternion.
 */
template <typename T>
static quat<T>
quat_from_mat3(T m00, T m01, T m02, T m10, T m11, T m12, T m20, T m21, T m22)
{
    T trace = m00 + m11 + m22;
    quat<T> q;
    if (trace > T(0))
    {
        T s = sqrt(trace + T(1)) * T(2);
        q.w = T(0.25) * s;
        q.x = (m21 - m12) / s;
        q.y = (m02 - m20) / s;
        q.z = (m10 - m01) / s;
    }
    else if (m00 > m11 && m00 > m22)
    {
        T s = sqrt(T(1) + m00 - m11 - m22) * T(2);
        q.w = (m21 - m12) / s;
        q.x = T(0.25) * s;
        q.y = (m01 + m10) / s;
        q.z = (m02 + m20) / s;
    }
    else if (m11 > m22)
    {
        T s = sqrt(T(1) + m11 - m00 - m22) * T(2);
        q.w = (m02 - m20) / s;
        q.x = (m01 + m10) / s;
        q.y = T(0.25) * s;
        q.z = (m12 + m21) / s;
    }
    else
    {
        T s = sqrt(T(1) + m22 - m00 - m11) * T(2);
        q.w = (m10 - m01) / s;
        q.x = (m02 + m20) / s;
        q.y = (m12 + m21) / s;
        q.z = T(0.25) * s;
    }
    return q;
}

/**
 * @brief Creates a quaternion from a rotation matrix.
 * @param m The rotation matrix.
 * @return The resulting quaternion.
 */
template <typename T> constexpr quat<T> quat_from_mat3(const mat3<T> &m)
{
    return quat_from_mat3(
        m.m00, m.m01, m.m02, m.m10, m.m11, m.m12, m.m20, m.m21, m.m22);
}

/**
 * @brief Performs spherical linear interpolation between two quaternions.
 * @param q1 The starting quaternion.
 * @param q2 The ending quaternion.
 * @param t The interpolation factor (0.0 to 1.0).
 * @return The interpolated quaternion.
 */
template <typename T>
constexpr quat<T> slerp(const quat<T> &q1, const quat<T> &q2, T t)
{
    T cos_omega      = dot(q1, q2);
    quat<T> q2_prime = q2;

    if (cos_omega < T(0))
    {
        q2_prime  = quat<T>(-q2.x, -q2.y, -q2.z, -q2.w);
        cos_omega = -cos_omega;
    }

    T s1, s2;
    if (cos_omega > T(0.9999))
    {
        s1 = T(1) - t;
        s2 = t;
    }
    else
    {
        T omega         = acos(cos_omega);
        T inv_sin_omega = T(1) / sin(omega);
        s1              = sin((T(1) - t) * omega) * inv_sin_omega;
        s2              = sin(t * omega) * inv_sin_omega;
    }

    return quat<T>(q1.x * s1 + q2_prime.x * s2,
                   q1.y * s1 + q2_prime.y * s2,
                   q1.z * s1 + q2_prime.z * s2,
                   q1.w * s1 + q2_prime.w * s2);
}

/**
 * @brief Adds two 3x3 matrices.
 * @param a The first matrix.
 * @param b The second matrix.
 * @return The sum matrix.
 */
template <typename T>
constexpr mat3<T> operator+(const mat3<T> &a, const mat3<T> &b)
{
    mat3<T> r;
    r.m00 = a.m00 + b.m00;
    r.m01 = a.m01 + b.m01;
    r.m02 = a.m02 + b.m02;
    r.m10 = a.m10 + b.m10;
    r.m11 = a.m11 + b.m11;
    r.m12 = a.m12 + b.m12;
    r.m20 = a.m20 + b.m20;
    r.m21 = a.m21 + b.m21;
    r.m22 = a.m22 + b.m22;
    return r;
}

/**
 * @brief Subtracts two 3x3 matrices.
 * @param a The first matrix.
 * @param b The second matrix.
 * @return The difference matrix.
 */
template <typename T>
constexpr mat3<T> operator-(const mat3<T> &a, const mat3<T> &b)
{
    mat3<T> r;
    r.m00 = a.m00 - b.m00;
    r.m01 = a.m01 - b.m01;
    r.m02 = a.m02 - b.m02;
    r.m10 = a.m10 - b.m10;
    r.m11 = a.m11 - b.m11;
    r.m12 = a.m12 - b.m12;
    r.m20 = a.m20 - b.m20;
    r.m21 = a.m21 - b.m21;
    r.m22 = a.m22 - b.m22;
    return r;
}

/**
 * @brief Divides a 3x3 matrix by a scalar.
 * @param m The matrix.
 * @param s The scalar.
 * @return The quotient matrix.
 */
template <typename T> constexpr mat3<T> operator/(const mat3<T> &m, T s)
{
    mat3<T> r;
    r.m00 = m.m00 / s;
    r.m01 = m.m01 / s;
    r.m02 = m.m02 / s;
    r.m10 = m.m10 / s;
    r.m11 = m.m11 / s;
    r.m12 = m.m12 / s;
    r.m20 = m.m20 / s;
    r.m21 = m.m21 / s;
    r.m22 = m.m22 / s;
    return r;
}

/**
 * @brief Multiplies two 3x3 matrices.
 * @param a The first matrix.
 * @param b The second matrix.
 * @return The product matrix.
 */
template <typename T>
constexpr mat3<T> operator*(const mat3<T> &a, const mat3<T> &b)
{
    mat3<T> r;
    r.m00 = a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20;
    r.m10 = a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20;
    r.m20 = a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20;

    r.m01 = a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21;
    r.m11 = a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21;
    r.m21 = a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21;

    r.m02 = a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22;
    r.m12 = a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22;
    r.m22 = a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22;
    return r;
}

/**
 * @brief Multiplies a 3x3 matrix by a 3D vector.
 * @param m The matrix.
 * @param v The vector.
 * @return The transformed vector.
 */
template <typename T>
constexpr vec3<T> operator*(const mat3<T> &m, const vec3<T> &v)
{
    return vec3<T>(m.m00 * v.x + m.m01 * v.y + m.m02 * v.z,
                   m.m10 * v.x + m.m11 * v.y + m.m12 * v.z,
                   m.m20 * v.x + m.m21 * v.y + m.m22 * v.z);
}

/**
 * @brief Component-wise minimum value for 3x3 matrices.
 * @param a The first matrix.
 * @param b The second matrix.
 * @return A new matrix with minimum values of components.
 */
template <typename T> constexpr mat3<T> min(const mat3<T> &a, const mat3<T> &b)
{
    mat3<T> r;
    r.m00 = min(a.m00, b.m00);
    r.m01 = min(a.m01, b.m01);
    r.m02 = min(a.m02, b.m02);
    r.m10 = min(a.m10, b.m10);
    r.m11 = min(a.m11, b.m11);
    r.m12 = min(a.m12, b.m12);
    r.m20 = min(a.m20, b.m20);
    r.m21 = min(a.m21, b.m21);
    r.m22 = min(a.m22, b.m22);
    return r;
}

/**
 * @brief Component-wise maximum value for 3x3 matrices.
 * @param a The first matrix.
 * @param b The second matrix.
 * @return A new matrix with maximum values of components.
 */
template <typename T> constexpr mat3<T> max(const mat3<T> &a, const mat3<T> &b)
{
    mat3<T> r;
    r.m00 = max(a.m00, b.m00);
    r.m01 = max(a.m01, b.m01);
    r.m02 = max(a.m02, b.m02);
    r.m10 = max(a.m10, b.m10);
    r.m11 = max(a.m11, b.m11);
    r.m12 = max(a.m12, b.m12);
    r.m20 = max(a.m20, b.m20);
    r.m21 = max(a.m21, b.m21);
    r.m22 = max(a.m22, b.m22);
    return r;
}

/**
 * @brief Transposes a 3x3 matrix.
 * @param m The matrix to transpose.
 * @return The transposed matrix.
 */
template <typename T> constexpr mat3<T> transpose(const mat3<T> &m)
{
    mat3<T> r;
    r.m00 = m.m00;
    r.m01 = m.m10;
    r.m02 = m.m20;
    r.m10 = m.m01;
    r.m11 = m.m11;
    r.m12 = m.m21;
    r.m20 = m.m02;
    r.m21 = m.m12;
    r.m22 = m.m22;
    return r;
}
/**
 * @brief Adds two 4x4 matrices.
 * @param a The first matrix.
 * @param b The second matrix.
 * @return The sum matrix.
 */
template <typename T>
constexpr mat4<T> operator+(const mat4<T> &a, const mat4<T> &b)
{
    mat4<T> r;
    r.m00 = a.m00 + b.m00;
    r.m01 = a.m01 + b.m01;
    r.m02 = a.m02 + b.m02;
    r.m03 = a.m03 + b.m03;
    r.m10 = a.m10 + b.m10;
    r.m11 = a.m11 + b.m11;
    r.m12 = a.m12 + b.m12;
    r.m13 = a.m13 + b.m13;
    r.m20 = a.m20 + b.m20;
    r.m21 = a.m21 + b.m21;
    r.m22 = a.m22 + b.m22;
    r.m23 = a.m23 + b.m23;
    r.m30 = a.m30 + b.m30;
    r.m31 = a.m31 + b.m31;
    r.m32 = a.m32 + b.m32;
    r.m33 = a.m33 + b.m33;
    return r;
}

/**
 * @brief Subtracts two 4x4 matrices.
 * @param a The first matrix.
 * @param b The second matrix.
 * @return The difference matrix.
 */
template <typename T>
constexpr mat4<T> operator-(const mat4<T> &a, const mat4<T> &b)
{
    mat4<T> r;
    r.m00 = a.m00 - b.m00;
    r.m01 = a.m01 - b.m01;
    r.m02 = a.m02 - b.m02;
    r.m03 = a.m03 - b.m03;
    r.m10 = a.m10 - b.m10;
    r.m11 = a.m11 - b.m11;
    r.m12 = a.m12 - b.m12;
    r.m13 = a.m13 - b.m13;
    r.m20 = a.m20 - b.m20;
    r.m21 = a.m21 - b.m21;
    r.m22 = a.m22 - b.m22;
    r.m23 = a.m23 - b.m23;
    r.m30 = a.m30 - b.m30;
    r.m31 = a.m31 - b.m31;
    r.m32 = a.m32 - b.m32;
    r.m33 = a.m33 - b.m33;
    return r;
}

/**
 * @brief Divides a 4x4 matrix by a scalar.
 * @param m The matrix.
 * @param s The scalar.
 * @return The quotient matrix.
 */
template <typename T> constexpr mat4<T> operator/(const mat4<T> &m, T s)
{
    mat4<T> r;
    r.m00 = m.m00 / s;
    r.m01 = m.m01 / s;
    r.m02 = m.m02 / s;
    r.m03 = m.m03 / s;
    r.m10 = m.m10 / s;
    r.m11 = m.m11 / s;
    r.m12 = m.m12 / s;
    r.m13 = m.m13 / s;
    r.m20 = m.m20 / s;
    r.m21 = m.m21 / s;
    r.m22 = m.m22 / s;
    r.m23 = m.m23 / s;
    r.m30 = m.m30 / s;
    r.m31 = m.m31 / s;
    r.m32 = m.m32 / s;
    r.m33 = m.m33 / s;
    return r;
}

/**
 * @brief Multiplies two 4x4 matrices.
 * @param a The first matrix.
 * @param b The second matrix.
 * @return The product matrix.
 */
template <typename T>
constexpr mat4<T> operator*(const mat4<T> &a, const mat4<T> &b)
{
    mat4<T> r;
    r.m00 = a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20 + a.m03 * b.m30;
    r.m10 = a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20 + a.m13 * b.m30;
    r.m20 = a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20 + a.m23 * b.m30;
    r.m30 = a.m30 * b.m00 + a.m31 * b.m10 + a.m32 * b.m20 + a.m33 * b.m30;

    r.m01 = a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21 + a.m03 * b.m31;
    r.m11 = a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21 + a.m13 * b.m31;
    r.m21 = a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21 + a.m23 * b.m31;
    r.m31 = a.m30 * b.m01 + a.m31 * b.m11 + a.m32 * b.m21 + a.m33 * b.m31;

    r.m02 = a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22 + a.m03 * b.m32;
    r.m12 = a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22 + a.m13 * b.m32;
    r.m22 = a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22 + a.m23 * b.m32;
    r.m32 = a.m30 * b.m02 + a.m31 * b.m12 + a.m32 * b.m22 + a.m33 * b.m32;

    r.m03 = a.m00 * b.m03 + a.m01 * b.m13 + a.m02 * b.m23 + a.m03 * b.m33;
    r.m13 = a.m10 * b.m03 + a.m11 * b.m13 + a.m12 * b.m23 + a.m13 * b.m33;
    r.m23 = a.m20 * b.m03 + a.m21 * b.m13 + a.m22 * b.m23 + a.m23 * b.m33;
    r.m33 = a.m30 * b.m03 + a.m31 * b.m13 + a.m32 * b.m23 + a.m33 * b.m33;
    return r;
}

/**
 * @brief Multiplies a 4x4 matrix by a 4D vector.
 * @param m The matrix.
 * @param v The vector.
 * @return The transformed vector.
 */
template <typename T>
constexpr vec4<T> operator*(const mat4<T> &m, const vec4<T> &v)
{
    return vec4<T>(m.m00 * v.x + m.m01 * v.y + m.m02 * v.z + m.m03 * v.w,
                   m.m10 * v.x + m.m11 * v.y + m.m12 * v.z + m.m13 * v.w,
                   m.m20 * v.x + m.m21 * v.y + m.m22 * v.z + m.m23 * v.w,
                   m.m30 * v.x + m.m31 * v.y + m.m32 * v.z + m.m33 * v.w);
}

/**
 * @brief Component-wise minimum value for 4x4 matrices.
 * @param a The first matrix.
 * @param b The second matrix.
 * @return A new matrix with minimum values of components.
 */
template <typename T> constexpr mat4<T> min(const mat4<T> &a, const mat4<T> &b)
{
    mat4<T> r;
    r.m00 = min(a.m00, b.m00);
    r.m01 = min(a.m01, b.m01);
    r.m02 = min(a.m02, b.m02);
    r.m03 = min(a.m03, b.m03);
    r.m10 = min(a.m10, b.m10);
    r.m11 = min(a.m11, b.m11);
    r.m12 = min(a.m12, b.m12);
    r.m13 = min(a.m13, b.m13);
    r.m20 = min(a.m20, b.m20);
    r.m21 = min(a.m21, b.m21);
    r.m22 = min(a.m22, b.m22);
    r.m23 = min(a.m23, b.m23);
    r.m30 = min(a.m30, b.m30);
    r.m31 = min(a.m31, b.m31);
    r.m32 = min(a.m32, b.m32);
    r.m33 = min(a.m33, b.m33);
    return r;
}

/**
 * @brief Component-wise maximum value for 4x4 matrices.
 * @param a The first matrix.
 * @param b The second matrix.
 * @return A new matrix with maximum values of components.
 */
template <typename T> constexpr mat4<T> max(const mat4<T> &a, const mat4<T> &b)
{
    mat4<T> r;
    r.m00 = max(a.m00, b.m00);
    r.m01 = max(a.m01, b.m01);
    r.m02 = max(a.m02, b.m02);
    r.m03 = max(a.m03, b.m03);
    r.m10 = max(a.m10, b.m10);
    r.m11 = max(a.m11, b.m11);
    r.m12 = max(a.m12, b.m12);
    r.m13 = max(a.m13, b.m13);
    r.m20 = max(a.m20, b.m20);
    r.m21 = max(a.m21, b.m21);
    r.m22 = max(a.m22, b.m22);
    r.m23 = max(a.m23, b.m23);
    r.m30 = max(a.m30, b.m30);
    r.m31 = max(a.m31, b.m31);
    r.m32 = max(a.m32, b.m32);
    r.m33 = max(a.m33, b.m33);
    return r;
}

/**
 * @brief Transposes a 4x4 matrix.
 * @param m The matrix to transpose.
 * @return The transposed matrix.
 */
template <typename T> constexpr mat4<T> transpose(const mat4<T> &m)
{
    mat4<T> r;
    r.m00 = m.m00;
    r.m01 = m.m10;
    r.m02 = m.m20;
    r.m03 = m.m30;
    r.m10 = m.m01;
    r.m11 = m.m11;
    r.m12 = m.m21;
    r.m13 = m.m31;
    r.m20 = m.m02;
    r.m21 = m.m12;
    r.m22 = m.m22;
    r.m23 = m.m32;
    r.m30 = m.m03;
    r.m31 = m.m13;
    r.m32 = m.m23;
    r.m33 = m.m33;
    return r;
}

/**
 * @brief Creates a scaling matrix.
 * @param v The scaling vector.
 * @return A 3x3 scaling matrix.
 */
template <typename T> constexpr mat3<T> mat3_scaling(const vec3<T> &v)
{
    mat3<T> m = mat3<T>::identity();
    m.m00     = v.x;
    m.m11     = v.y;
    m.m22     = v.z;
    return m;
}

/**
 * @brief Creates a scaling matrix from scalars.
 * @param x X scale.
 * @param y Y scale.
 * @param z Z scale.
 * @return A 3x3 scaling matrix.
 */
template <typename T> constexpr mat3<T> mat3_scaling(T x, T y, T z)
{
    mat3<T> m = mat3<T>::identity();
    m.m00     = x;
    m.m11     = y;
    m.m22     = z;
    return m;
}

/**
 * @brief Creates a translation matrix.
 * @param v The translation vector.
 * @return A 4x4 translation matrix.
 */
template <typename T> constexpr mat4<T> mat4_translation(const vec3<T> &v)
{
    mat4<T> m = mat4<T>::identity();
    m.m03     = v.x;
    m.m13     = v.y;
    m.m23     = v.z;
    return m;
}

/**
 * @brief Creates a scaling matrix.
 * @param v The scaling vector.
 * @return A 4x4 scaling matrix.
 */
template <typename T> constexpr mat4<T> mat4_scaling(const vec3<T> &v)
{
    mat4<T> m = mat4<T>::identity();
    m.m00     = v.x;
    m.m11     = v.y;
    m.m22     = v.z;
    return m;
}

/**
 * @brief Creates a rotation matrix around the X axis.
 * @param angle The rotation angle in radians.
 * @return A 4x4 rotation matrix.
 */
template <typename T> constexpr mat4<T> mat4_rotation_x(T angle)
{
    mat4<T> m   = mat4<T>::identity();
    auto [s, c] = sincos(angle);
    m.m11       = c;
    m.m12       = -s;
    m.m21       = s;
    m.m22       = c;
    return m;
}

/**
 * @brief Creates a rotation matrix around the Y axis.
 * @param angle The rotation angle in radians.
 * @return A 4x4 rotation matrix.
 */
template <typename T> constexpr mat4<T> mat4_rotation_y(T angle)
{
    mat4<T> m   = mat4<T>::identity();
    auto [s, c] = sincos(angle);
    m.m00       = c;
    m.m02       = s;
    m.m20       = -s;
    m.m22       = c;
    return m;
}

/**
 * @brief Creates a rotation matrix around the Z axis.
 * @param angle The rotation angle in radians.
 * @return A 4x4 rotation matrix.
 */
template <typename T> constexpr mat4<T> mat4_rotation_z(T angle)
{
    mat4<T> m   = mat4<T>::identity();
    auto [s, c] = sincos(angle);
    m.m00       = c;
    m.m01       = -s;
    m.m10       = s;
    m.m11       = c;
    return m;
}

/**
 * @brief Converts a quaternion to a 3x3 rotation matrix.
 * @param q The quaternion.
 * @return A 3x3 rotation matrix.
 */
template <typename T> constexpr mat3<T> mat3_from_quat(const quat<T> &q)
{
    mat3<T> m;
    T xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    T xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    T wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

    m.m00 = T(1) - T(2) * (yy + zz);
    m.m01 = T(2) * (xy - wz);
    m.m02 = T(2) * (xz + wy);

    m.m10 = T(2) * (xy + wz);
    m.m11 = T(1) - T(2) * (xx + zz);
    m.m12 = T(2) * (yz - wx);

    m.m20 = T(2) * (xz - wy);
    m.m21 = T(2) * (yz + wx);
    m.m22 = T(1) - T(2) * (xx + yy);

    return m;
}

/**
 * @brief Converts a quaternion to a 4x4 rotation matrix.
 * @param q The quaternion.
 * @return A 4x4 rotation matrix.
 */
template <typename T> constexpr mat4<T> mat4_from_quat(const quat<T> &q)
{
    mat4<T> m;
    T xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    T xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    T wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

    m.m00 = T(1) - T(2) * (yy + zz);
    m.m01 = T(2) * (xy - wz);
    m.m02 = T(2) * (xz + wy);
    m.m03 = T(0);
    m.m10 = T(2) * (xy + wz);
    m.m11 = T(1) - T(2) * (xx + zz);
    m.m12 = T(2) * (yz - wx);
    m.m13 = T(0);
    m.m20 = T(2) * (xz - wy);
    m.m21 = T(2) * (yz + wx);
    m.m22 = T(1) - T(2) * (xx + yy);
    m.m23 = T(0);
    m.m30 = T(0);
    m.m31 = T(0);
    m.m32 = T(0);
    m.m33 = T(1);
    return m;
}

/**
 * @brief Creates a look-at view matrix.
 * @param cameraPosition The position of the camera.
 * @param cameraTarget The target point the camera is looking at.
 * @param cameraUpVector The up vector of the camera.
 * @return A 4x4 view matrix.
 */
template <typename T>
constexpr mat4<T> mat4_look_at(const vec3<T> &cameraPosition,
                               const vec3<T> &cameraTarget,
                               const vec3<T> &cameraUpVector)
{
    vec3<T> zaxis = normalize(cameraPosition - cameraTarget);
    vec3<T> xaxis = normalize(cross(cameraUpVector, zaxis));
    vec3<T> yaxis = cross(zaxis, xaxis);

    mat4<T> m;
    m.m00 = xaxis.x;
    m.m10 = yaxis.x;
    m.m20 = zaxis.x;
    m.m30 = 0.0;
    m.m01 = xaxis.y;
    m.m11 = yaxis.y;
    m.m21 = zaxis.y;
    m.m31 = 0.0;
    m.m02 = xaxis.z;
    m.m12 = yaxis.z;
    m.m22 = zaxis.z;
    m.m32 = 0.0;
    m.m03 = -dot(xaxis, cameraPosition);
    m.m13 = -dot(yaxis, cameraPosition);
    m.m23 = -dot(zaxis, cameraPosition);
    m.m33 = 1.0;
    return m;
}

/**
 * @brief Creates an orthographic projection matrix.
 * @param left The left coordinate of the viewing volume.
 * @param right The right coordinate of the viewing volume.
 * @param bottom The bottom coordinate of the viewing volume.
 * @param top The top coordinate of the viewing volume.
 * @param near_plane The near clipping plane distance.
 * @param far_plane The far clipping plane distance.
 * @return A 4x4 orthographic projection matrix.
 */
template <typename T>
constexpr mat4<T>
mat4_ortho(T left, T right, T bottom, T top, T near_plane, T far_plane)
{
    mat4<T> m = mat4<T>::identity();
    m.m00     = T(2) / (right - left);
    m.m11     = T(2) / (top - bottom);
    m.m22     = -T(2) / (far_plane - near_plane);
    m.m03     = -(right + left) / (right - left);
    m.m13     = -(top + bottom) / (top - bottom);
    m.m23     = -(far_plane + near_plane) / (far_plane - near_plane);
    return m;
}

/**
 * @brief Creates a perspective projection matrix.
 * @param fov_y The field of view angle in the y direction (in radians).
 * @param aspect The aspect ratio (width / height).
 * @param near_plane The near clipping plane distance.
 * @param far_plane The far clipping plane distance.
 * @return A 4x4 perspective projection matrix.
 */
template <typename T>
constexpr mat4<T>
mat4_perspective_fov(T fov_y, T aspect, T near_plane, T far_plane)
{
    mat4<T> m{};
    T const tan_half_fovy = tan(fov_y / T(2));
    m.m00                 = T(1) / (aspect * tan_half_fovy);
    m.m11                 = T(1) / (tan_half_fovy);
    m.m22 = -(far_plane + near_plane) / (far_plane - near_plane);
    m.m32 = -T(1);
    m.m23 = -(T(2) * far_plane * near_plane) / (far_plane - near_plane);
    return m;
}

/**
 * @brief Decomposes a transformation matrix into position, scale, and rotation.
 * @param m The transformation matrix.
 * @param out_pos Output parameter for position.
 * @param out_scale Output parameter for scale.
 * @param out_rot Output parameter for rotation (quaternion).
 */
template <typename T>
void mat4_decompose(const mat4<T> &m,
                    vec3<T> &out_pos,
                    vec3<T> &out_scale,
                    quat<T> &out_rot)
{
    out_pos    = {m.m03, m.m13, m.m23};
    vec3<T> c0 = {m.m00, m.m10, m.m20};
    vec3<T> c1 = {m.m01, m.m11, m.m21};
    vec3<T> c2 = {m.m02, m.m12, m.m22};
    T sx       = length(c0);
    T sy       = length(c1);
    T sz       = length(c2);
    if (sx == T(0))
        sx = T(1);
    if (sy == T(0))
        sy = T(1);
    if (sz == T(0))
        sz = T(1);
    out_scale = {sx, sy, sz};
    // Normalize columns to get rotation matrix
    T rm00 = m.m00 / sx, rm01 = m.m01 / sy, rm02 = m.m02 / sz;
    T rm10 = m.m10 / sx, rm11 = m.m11 / sy, rm12 = m.m12 / sz;
    T rm20 = m.m20 / sx, rm21 = m.m21 / sy, rm22 = m.m22 / sz;
    out_rot =
        quat_from_mat3(rm00, rm01, rm02, rm10, rm11, rm12, rm20, rm21, rm22);
}

/**
 * @brief Calculates the inverse of a 3x3 matrix.
 * @param mat The input matrix.
 * @param result Output parameter for the inverted matrix.
 * @return True if the matrix is invertible, false otherwise.
 */
template <typename T>
constexpr bool inverse(const mat3<T> &mat, mat3<T> &result)
{
    const T m00 = mat.m00, m01 = mat.m01, m02 = mat.m02;
    const T m10 = mat.m10, m11 = mat.m11, m12 = mat.m12;
    const T m20 = mat.m20, m21 = mat.m21, m22 = mat.m22;

    const T det = m00 * (m11 * m22 - m12 * m21) -
                  m01 * (m10 * m22 - m12 * m20) + m02 * (m10 * m21 - m11 * m20);

    if (abs(det) < T::epsilon())
    {
        result = {};
        return false;
    }

    const T invDet = T(1) / det;

    result.m00 = (m11 * m22 - m12 * m21) * invDet;
    result.m01 = (m02 * m21 - m01 * m22) * invDet;
    result.m02 = (m01 * m12 - m02 * m11) * invDet;

    result.m10 = (m12 * m20 - m10 * m22) * invDet;
    result.m11 = (m00 * m22 - m02 * m20) * invDet;
    result.m12 = (m02 * m10 - m00 * m12) * invDet;

    result.m20 = (m10 * m21 - m11 * m20) * invDet;
    result.m21 = (m01 * m20 - m00 * m21) * invDet;
    result.m22 = (m00 * m11 - m01 * m10) * invDet;

    return true;
}

/**
 * @brief Calculates the inverse of a 4x4 matrix.
 * @param mat The input matrix.
 * @param result Output parameter for the inverted matrix.
 * @return True if the matrix is invertible, false otherwise.
 */
template <typename T>
constexpr bool inverse(const mat4<T> &mat, mat4<T> &result)
{
    const T a = mat.m00, b = mat.m10, c = mat.m20, d = mat.m30;
    const T e = mat.m01, f = mat.m11, g = mat.m21, h = mat.m31;
    const T i = mat.m02, j = mat.m12, k = mat.m22, l = mat.m32;
    const T m = mat.m03, n = mat.m13, o = mat.m23, p = mat.m33;

    const T kp_lo = k * p - l * o;
    const T jp_ln = j * p - l * n;
    const T jo_kn = j * o - k * n;
    const T ip_lm = i * p - l * m;
    const T io_km = i * o - k * m;
    const T in_jm = i * n - j * m;

    const T a00 = +(f * kp_lo - g * jp_ln + h * jo_kn);
    const T a10 = -(e * kp_lo - g * ip_lm + h * io_km);
    const T a20 = +(e * jp_ln - f * ip_lm + h * in_jm);
    const T a30 = -(e * jo_kn - f * io_km + g * in_jm);

    const T det = a * a00 + b * a10 + c * a20 + d * a30;

    if (abs(det) < T::epsilon())
    {
        result = {};
        return false;
    }

    const T invDet = T(1.0) / det;

    const T gp_ho = g * p - h * o;
    const T fp_hn = f * p - h * n;
    const T fo_gn = f * o - g * n;
    const T ep_hm = e * p - h * m;
    const T eo_gm = e * o - g * m;
    const T en_fm = e * n - f * m;

    const T gl_hk = g * l - h * k;
    const T fl_hj = f * l - h * j;
    const T fk_gj = f * k - g * j;
    const T el_hi = e * l - h * i;
    const T ek_gi = e * k - g * i;
    const T ej_fi = e * j - f * i;

    result.m00 = a00 * invDet;
    result.m10 = -(b * kp_lo - c * jp_ln + d * jo_kn) * invDet;
    result.m20 = +(b * gp_ho - c * fp_hn + d * fo_gn) * invDet;
    result.m30 = -(b * gl_hk - c * fl_hj + d * fk_gj) * invDet;

    result.m01 = a10 * invDet;
    result.m11 = +(a * kp_lo - c * ip_lm + d * io_km) * invDet;
    result.m21 = -(a * gp_ho - c * ep_hm + d * eo_gm) * invDet;
    result.m31 = +(a * gl_hk - c * el_hi + d * ek_gi) * invDet;

    result.m02 = a20 * invDet;
    result.m12 = -(a * jp_ln - b * ip_lm + d * in_jm) * invDet;
    result.m22 = +(a * fp_hn - b * ep_hm + d * en_fm) * invDet;
    result.m32 = -(a * fl_hj - b * el_hi + d * ej_fi) * invDet;

    result.m03 = a30 * invDet;
    result.m13 = +(a * jo_kn - b * io_km + c * in_jm) * invDet;
    result.m23 = -(a * fo_gn - b * eo_gm + c * en_fm) * invDet;
    result.m33 = +(a * fk_gj - b * ek_gi + c * ej_fi) * invDet;

    return true;
}

/**
 * @brief Normalizes a plane (normalizes the normal vector and scales distance).
 * @param p The plane to normalize.
 * @return The normalized plane.
 */
template <typename T> constexpr plane3<T> normalize(const plane3<T> &p)
{
    T len     = length(p.normal);
    T inv_len = T(1) / len;
    return plane3<T>(p.normal * inv_len, p.d * inv_len);
}

/**
 * @brief Calculates the signed distance from a point to a plane.
 * @param p The plane.
 * @param v The point.
 * @return The signed distance (positive if point is on the side of the normal).
 */
template <typename T>
constexpr T signed_distance(const plane3<T> &p, const vec3<T> &v)
{
    return dot(p.normal, v) + p.d;
}

/**
 * @brief Creates a plane from a point and a normal.
 * @param point A point on the plane.
 * @param normal The normal vector of the plane.
 * @return The constructed plane.
 */
template <typename T>
constexpr plane3<T> plane_from_point_normal(const vec3<T> &point,
                                            const vec3<T> &normal)
{
    return plane3<T>(normal, -dot(normal, point));
}

/**
 * @brief Creates a plane from three points.
 * @param p1 The first point.
 * @param p2 The second point.
 * @param p3 The third point.
 * @return The constructed plane.
 */
template <typename T>
constexpr plane3<T>
plane_from_points(const vec3<T> &p1, const vec3<T> &p2, const vec3<T> &p3)
{
    vec3<T> normal = normalize(cross(p2 - p1, p3 - p1));
    return plane_from_point_normal(p1, normal);
}

} // namespace zabato