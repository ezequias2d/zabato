#pragma once

#include <cstdint>
#include <zabato/math.hpp>
#include <zabato/real.hpp>

#include <stdint.h>

namespace zabato
{
struct color8888;
struct color5551;
struct color4444;
struct color;

#pragma region Main color

/**
 * @struct color
 * @brief A color class with four components (r, g, b, a) using the `real` type.
 *
 * This class provides a high-precision representation of a color and serves as
 * the primary type for color mathematics and conversions.
 */
struct color
{
    real r, g, b, a;

    /** @brief Default constructor. Initializes to opaque black (0,0,0,1). */
    constexpr color() : r(0), g(0), b(0), a(1) {}
    /** @brief Constructs a color from individual RGBA components. Alpha
     * defaults to 1. */
    constexpr color(real red, real green, real blue, real alpha = real(1))
        : r(red), g(green), b(blue), a(alpha)
    {
    }
    /** @brief Constructs a color from an RGB vector and an alpha value. */
    constexpr explicit color(const vec3<real> &rgb, real alpha = real(1))
        : r(rgb.x), g(rgb.y), b(rgb.z), a(alpha)
    {
    }

    /** @brief Constructs a color from an RGBA vector. */
    constexpr explicit color(const vec4<real> &v)
        : r(v.x), g(v.y), b(v.z), a(v.w)
    {
    }

    template <typename T>
    constexpr explicit color(const T &c) : color(c.as_color())
    {
    }

    /** @brief Returns a mutable reference to the color as a vec4. */
    constexpr vec4<real> as_vec4() const { return vec4<real>(r, g, b, a); }

    /** @brief Returns a mutable pointer to the underlying component array. */
    constexpr real *as_array() { return &r; }
    /** @brief Returns a const pointer to the underlying component array. */
    constexpr const real *as_array() const { return &r; }

    constexpr color operator+(const color &c) const
    {
        return color(r + c.r, g + c.g, b + c.b, a + c.a);
    }

    constexpr color operator-(const color &c) const
    {
        return color(r - c.r, g - c.g, b - c.b, a - c.a);
    }

    /** @brief Modulates this color by another. Performs component-wise
     * multiplication. */
    constexpr color operator*(const color &c) const
    {
        return color(r * c.r, g * c.g, b * c.b, a * c.a);
    }

    /** @brief Scales all color components by a scalar value. */
    constexpr color operator*(real s) const
    {
        return color(r * s, g * s, b * s, a * s);
    }

    constexpr bool operator==(const color &c) const
    {
        return r == c.r && g == c.g && b == c.b;
    }

    constexpr bool operator!=(const color &c) const { return !(*this == c); }

    static constexpr color black() { return color(0, 0, 0); }
    static constexpr color white() { return color(1, 1, 1); }
    static constexpr color red() { return color(1, 0, 0); }
    static constexpr color green() { return color(0, 1, 0); }
    static constexpr color blue() { return color(0, 0, 1); }
    static constexpr color yellow() { return color(1, 1, 0); }
    static constexpr color cyan() { return color(0, 1, 1); }
    static constexpr color magenta() { return color(1, 0, 1); }

    /**
     * @brief Creates a color from a 24-bit hexadecimal value (e.g., 0xFF8000).
     * @param hex The hexadecimal RGB value. Alpha is set to 1.0.
     * @return A new color object.
     */
    static constexpr color from_hex(uint32_t hex)
    {
        real r = real((hex >> 16) & 0xFF) / real(255.0f);
        real g = real((hex >> 8) & 0xFF) / real(255.0f);
        real b = real(hex & 0xFF) / real(255.0f);
        return color(r, g, b);
    }
};

#pragma endregion

#pragma region Packed Colors

/**
 * @struct color8888
 * @brief Represents a 32-bit RGBA color with 8 bits per channel.
 */
struct color8888
{
    uint32_t value;

    constexpr color8888() : value(0) {}
    constexpr color8888(uint32_t value) : value(value) {}
    constexpr color8888(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        : value(((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) |
                ((uint32_t)r))
    {
    }

    /**
     * @brief Constructs a packed 32-bit color from a floating-point color
     * object.
     * @param c The source color object.
     */
    explicit color8888(const color &c)
    {
        auto clamped = clamp(c.as_vec4(), vec4<real>(0), vec4<real>(1));
        uint32_t r8 =
            static_cast<uint32_t>(clamped.x * real(255.0f) + real(0.5f));
        uint32_t g8 = static_cast<uint32_t>(clamped.y * real(255.0f) + 0.5f);
        uint32_t b8 = static_cast<uint32_t>(clamped.z * real(255.0f) + 0.5f);
        uint32_t a8 = static_cast<uint32_t>(clamped.w * real(255.0f) + 0.5f);
        value       = (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
    }

    constexpr uint8_t r() const { return (value >> 0) & 0xFF; }
    constexpr uint8_t g() const { return (value >> 8) & 0xFF; }
    constexpr uint8_t b() const { return (value >> 16) & 0xFF; }
    constexpr uint8_t a() const { return (value >> 24) & 0xFF; }

    constexpr real rr() const { return real(r()) / real(255.0f); }
    constexpr real gg() const { return real(g()) / real(255.0f); }
    constexpr real bb() const { return real(b()) / real(255.0f); }
    constexpr real aa() const { return real(a()) / real(255.0f); }

    constexpr color as_color() const { return color(rr(), gg(), bb(), aa()); }
};

/**
 * @struct color5551
 * @brief Represents a 16-bit RGBA color with 5 bits for RGB and 1 bit for
 * alpha.
 */
struct color5551
{
    uint16_t value;

    constexpr color5551() : value(0) {}
    constexpr color5551(uint16_t value) : value(value) {}
    constexpr color5551(real r, real b, real g, real a = real(1.0))
        : color5551(color{r, g, b, a})
    {
    }
    constexpr color5551(real value) : color5551(value, value, value, real(1.0))
    {
    }

    /**
     * @brief Constructs a packed 16-bit color from a floating-point color
     * object.
     * @param c The source color object.
     */
    constexpr explicit color5551(const color &c)
    {
        color8888 c8888(c);
        uint8_t r8  = (c8888.value >> 0) & 0xFF;
        uint8_t g8  = (c8888.value >> 8) & 0xFF;
        uint8_t b8  = (c8888.value >> 16) & 0xFF;
        uint8_t a8  = (c8888.value >> 24) & 0xFF;
        uint16_t r5 = r8 >> 3, g5 = g8 >> 3, b5 = b8 >> 3,
                 a1 = (a8 >= 128) ? 1 : 0;
        value       = (a1 << 15) | (b5 << 10) | (g5 << 5) | r5;
    }

    constexpr uint8_t r() const { return (value >> 0) & 0x1F; }
    constexpr uint8_t g() const { return (value >> 5) & 0x1F; }
    constexpr uint8_t b() const { return (value >> 10) & 0x1F; }
    constexpr uint8_t a() const { return (value >> 15) & 0x01; }

    constexpr real rr() const { return real(r()) / real(31.0f); }
    constexpr real gg() const { return real(g()) / real(31.0f); }
    constexpr real bb() const { return real(b()) / real(31.0f); }
    constexpr real aa() const { return real(a()) / real(1.0f); }

    constexpr color as_color() const { return color(rr(), gg(), bb(), aa()); }
};

/**
 * @struct color4444
 * @brief Represents a 16-bit RGBA color with 4 bits per channel.
 */
struct color4444
{
    uint16_t value;

    constexpr color4444() : value(0) {}
    constexpr color4444(uint16_t value) : value(value) {}

    /**
     * @brief Constructs a packed 16-bit color from a floating-point color
     * object.
     * @param c The source color object.
     */
    constexpr explicit color4444(const color &c)
    {
        color8888 c8888(c);
        uint8_t r8 = (c8888.value >> 0) & 0xFF, g8 = (c8888.value >> 8) & 0xFF,
                b8  = (c8888.value >> 16) & 0xFF,
                a8  = (c8888.value >> 24) & 0xFF;
        uint16_t r4 = min(15, (r8 + 8) >> 4) & 0xF;
        uint16_t g4 = min(15, (g8 + 8) >> 4) & 0xF;
        uint16_t b4 = min(15, (b8 + 8) >> 4) & 0xF;
        uint16_t a4 = min(15, (a8 + 8) >> 4) & 0xF;
        value       = (a4 << 12) | (b4 << 8) | (g4 << 4) | r4;
    }

    constexpr uint8_t r() const { return (value >> 0) & 0xF; }
    constexpr uint8_t g() const { return (value >> 4) & 0xF; }
    constexpr uint8_t b() const { return (value >> 8) & 0xF; }
    constexpr uint8_t a() const { return (value >> 12) & 0xF; }

    constexpr real rr() const { return real(r()) / real(15.0f); }
    constexpr real gg() const { return real(g()) / real(15.0f); }
    constexpr real bb() const { return real(b()) / real(15.0f); }
    constexpr real aa() const { return real(a()) / real(15.0f); }

    constexpr color as_color() const { return color(rr(), gg(), bb(), aa()); }
};

#pragma endregion

#pragma region Operations

/**
 * @brief Linearly interpolates between two colors.
 * @param c1 The start color.
 * @param c2 The end color.
 * @param t The interpolation factor (0.0 to 1.0).
 * @return The interpolated color.
 */
inline constexpr color lerp(const color &c1, const color &c2, real t)
{
    return color(lerp(c1.as_vec4(), c2.as_vec4(), t));
}

/**
 * @brief Converts a color to its grayscale equivalent using luminance weights.
 *
 * Uses the Rec. 601 luma coefficients: R * 0.299 + G * 0.587 + B * 0.114.
 *
 * @param c The source color.
 * @return A grayscale color (all RGB components equal to luma, alpha
 * preserved).
 */
inline constexpr color grayscale(const color &c)
{
    real bright = dot(vec3<real>(c.r, c.g, c.b),
                      vec3<real>(real(0.299f), real(0.587f), real(0.114f)));
    return color(bright, bright, bright, c.a);
}

/**
 * @brief Inverts the RGB components of a color (1 - component).
 * @param c The source color.
 * @return The inverted color. Alpha is preserved.
 */
inline constexpr color invert(const color &c)
{
    return color(real(1) - c.r, real(1) - c.g, real(1) - c.b, c.a);
}

/**
 * @brief Returns a copy of the color with a new alpha value.
 * @param c The source color.
 * @param new_alpha The new alpha value.
 * @return The modified color.
 */
inline constexpr color with_alpha(const color &c, real new_alpha)
{
    return color(c.r, c.g, c.b, new_alpha);
}

/**
 * @brief Returns a color with its RGB components multiplied by its alpha.
 *
 * Useful for compositing operations that expect premultiplied alpha.
 *
 * @param c The source color.
 * @return The premultiplied color.
 */
inline constexpr color premultiply_alpha(const color &c)
{
    return color(c.r * c.a, c.g * c.a, c.b * c.a, c.a);
}

/**
 * @brief Blends a source color over a destination color using the source's
 * alpha.
 *
 * Implements the standard alpha blending formula:
 * R = Src * Alpha + Dst * (1 - Alpha)
 *
 * @param source The source (foreground) color.
 * @param dest The destination (background) color.
 * @return The blended color.
 */
inline constexpr color alpha_blend(const color &source, const color &dest)
{
    real alpha = source.a;
    return source * alpha + dest * (real(1) - alpha);
}

#pragma endregion

#pragma region Color Space & Model Conversions

/**
 * @brief Converts an sRGB color component to linear space.
 * @param c The sRGB component value (0.0 to 1.0).
 * @return The linear component value.
 */
inline constexpr real srgb_to_linear_component(real c)
{
    return c <= real(0.04045f)
               ? c / real(12.92f)
               : pow((c + real(0.055f)) / real(1.055f), real(2.4f));
}

/**
 * @brief Converts an sRGB color to linear space.
 *
 * Required for correct physical lighting calculations.
 *
 * @param c The sRGB color.
 * @return The linear color. Alpha is passed through unchanged.
 */
inline constexpr color to_linear(const color &c)
{
    return color(srgb_to_linear_component(c.r),
                 srgb_to_linear_component(c.g),
                 srgb_to_linear_component(c.b),
                 c.a);
}

/**
 * @brief Converts a linear color component to sRGB space (gamma correction).
 * @param c The linear component value.
 * @return The sRGB component value.
 */
inline constexpr real linear_to_srgb_component(real c)
{
    return c <= real(0.0031308f)
               ? c * real(12.92f)
               : real(1.055f) * pow(c, real(1.0f / 2.4f)) - real(0.055f);
}

/**
 * @brief Converts a linear color to sRGB space.
 *
 * Used before displaying the final color on a screen.
 *
 * @param c The linear color.
 * @return The sRGB color. Alpha is passed through unchanged.
 */
inline constexpr color to_srgb(const color &c)
{
    return color(linear_to_srgb_component(c.r),
                 linear_to_srgb_component(c.g),
                 linear_to_srgb_component(c.b),
                 c.a);
}

/**
 * @brief Converts an RGB color to an HSV (Hue, Saturation, Value) vector.
 *
 * @param c The RGB color.
 * @return A vec3 where x=Hue (0-1), y=Saturation (0-1), z=Value (0-1).
 */
inline constexpr vec3<real> to_hsv(const color &c)
{
    vec4<real> k = {
        real(0.0f), real(-1.0f / 3.0f), real(2.0f / 3.0f), real(-1.0f)};
    vec4<real> p = (c.g < c.b) ? vec4<real>(c.b, c.g, k.w, k.z)
                               : vec4<real>(c.g, c.b, k.x, k.y);
    vec4<real> q = (c.r < p.x) ? vec4<real>(p.x, p.y, p.w, c.r)
                               : vec4<real>(c.r, p.y, p.z, p.x);
    real d       = q.x - min(q.w, q.y);
    return vec3<real>(abs(q.z + (q.w - q.y) / (real(6.0f) * d + real(1e-10f))),
                      d / (q.x + real(1e-10f)),
                      q.x);
}

/**
 * @brief Converts an HSV vector to an RGB color.
 *
 * @param hsv The HSV vector where x=Hue, y=Saturation, z=Value.
 * @return The RGB color.
 */
inline constexpr color from_hsv(const vec3<real> &hsv)
{
    auto fract = [](real val) constexpr { return val - floor(val); };

    real h = hsv.x, s = hsv.y, v = hsv.z;
    vec4<real> k = {
        real(1.0f), real(2.0f / 3.0f), real(1.0f / 3.0f), real(3.0f)};
    vec3<real> p = {abs(fract(h + k.x) * real(6.0f) - k.w),
                    abs(fract(h + k.y) * real(6.0f) - k.w),
                    abs(fract(h + k.z) * real(6.0f) - k.w)};
    return color(
        lerp(vec3<real>(real(1.0f)),
             clamp(p - vec3<real>(1.0f), vec3<real>(0.0f), vec3<real>(1.0f)),
             s) *
        v);
}

/**
 * @brief Converts an RGB color to an HSL (Hue, Saturation, Lightness) vector.
 *
 * @param c The RGB color.
 * @return A vec3 where x=Hue (0-1), y=Saturation (0-1), z=Lightness (0-1).
 */
inline constexpr vec3<real> to_hsl(const color &c)
{
    real max_c = max(max(c.r, c.g), c.b);
    real min_c = min(min(c.r, c.g), c.b);
    real h = 0, s = 0, l = (max_c + min_c) / real(2);
    if (max_c != min_c)
    {
        real d = max_c - min_c;
        s      = l > real(0.5f) ? d / (real(2.0f) - max_c - min_c)
                                : d / (max_c + min_c);
        if (max_c == c.r)
            h = (c.g - c.b) / d + (c.g < c.b ? real(6) : real(0));
        else if (max_c == c.g)
            h = (c.b - c.r) / d + real(2);
        else
            h = (c.r - c.g) / d + real(4);
        h /= real(6);
    }
    return vec3<real>(h, s, l);
}

/**
 * @brief Converts an HSL vector to an RGB color.
 *
 * @param hsl The HSL vector where x=Hue, y=Saturation, z=Lightness.
 * @return The RGB color.
 */
inline constexpr color from_hsl(const vec3<real> &hsl)
{
    real h = hsl.x, s = hsl.y, l = hsl.z;
    if (s == real(0))
        return color(l, l, l);
    auto hue2rgb = [](real p, real q, real t) constexpr
    {
        if (t < real(0))
            t += real(1);
        if (t > real(1))
            t -= real(1);
        if (t < real(1.0f / 6.0f))
            return p + (q - p) * real(6) * t;
        if (t < real(1.0f / 2.0f))
            return q;
        if (t < real(2.0f / 3.0f))
            return p + (q - p) * (real(2.0f / 3.0f) - t) * real(6);
        return p;
    };
    real q = l < real(0.5f) ? l * (real(1) + s) : l + s - l * s;
    real p = real(2) * l - q;
    return color(hue2rgb(p, q, h + real(1.0f / 3.0f)),
                 hue2rgb(p, q, h),
                 hue2rgb(p, q, h - real(1.0f / 3.0f)));
}

/**
 * @brief Calculates the complementary color (180 degrees on the color wheel).
 * @param c The source color.
 * @return The complementary color.
 */
inline constexpr color complementary(const color &c)
{
    vec3<real> hsv = to_hsv(c);
    hsv.x          = mod(hsv.x + real(0.5f), real(1.0f));
    return from_hsv(hsv);
}

/**
 * @brief Calculates the two other colors in a triadic harmony (120 degrees
 * apart).
 * @param c The source color.
 * @return A tuple containing the two triadic colors.
 */
inline constexpr tuple<color, color> triadic(const color &c)
{
    vec3<real> hsv = to_hsv(c);
    real h1        = mod(hsv.x + real(1.0f / 3.0f), real(1));
    real h2        = mod(hsv.x + real(2.0f / 3.0f), real(1));
    return {from_hsv({h1, hsv.y, hsv.z}), from_hsv({h2, hsv.y, hsv.z})};
}

/**
 * @brief Calculates two analogous colors (adjacent on the color wheel).
 *
 * Returns colors offset by +/- 30 degrees (1/12th of a circle).
 *
 * @param c The source color.
 * @return A tuple containing the two analogous colors.
 */
inline constexpr tuple<color, color> analogous(const color &c)
{
    vec3<real> hsv = to_hsv(c);
    real h1        = mod(hsv.x + real(1.0f / 12.0f), real(1));    // 30 degrees
    real h2 = mod(hsv.x - real(1.0f / 12.0f) + real(1), real(1)); // -30 degrees
    return {from_hsv({h1, hsv.y, hsv.z}), from_hsv({h2, hsv.y, hsv.z})};
}

#pragma endregion

} // namespace zabato