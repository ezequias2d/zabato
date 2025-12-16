#pragma once

#include <zabato/color.hpp>
#include <zabato/math.hpp>
#include <zabato/real.hpp>
#include <zabato/resource.hpp>
#include <stdint.h>

namespace zabato
{

#pragma region Enums

/** @brief The type of primitive to be rendered in a begin/end block. */
enum class primitive_type : uint8_t
{
    quads,
    triangles,
    points,
    lines,
};

/** @brief The target matrix for transform operations. */
enum class matrix_mode : uint8_t
{
    modelview,
    projection,
    texture,
};

/** @brief The shading model for lighting calculations. */
enum class shade_model : uint8_t
{
    flat,
    smooth,
};

/** @brief The type of a light source. */
enum class light_type : uint16_t
{
    directional,
    point,
    spot,
};

/** @brief The color format of a texture's pixel data. */
enum class color_format : uint8_t
{
    undefined,
    rgba5551,
    rgba4444,
    palette16,
    palette64,
    palette128,
    palette256,
};

/** @brief Blending factors for source and destination. */
enum class blend_factor : uint8_t
{
    zero,
    one,
    src_color,
    one_minus_src_color,
    src_alpha,
    one_minus_src_alpha,
    dst_alpha,
    one_minus_dst_alpha,
    dst_color,
    one_minus_dst_color,
    src_alpha_saturate,
};

static inline const char *color_format_name(color_format format)
{
    switch (format)
    {
    case color_format::rgba5551:
        return "RGBA5551";
    case color_format::rgba4444:
        return "RGBA4444";
    case color_format::palette16:
        return "Palette16";
    case color_format::palette64:
        return "Palette64";
    case color_format::palette128:
        return "Palette128";
    case color_format::palette256:
        return "Palette256";
    default:
        return "Undefined";
    }
}

#pragma endregion

#pragma region Data Structs

/**
 * @struct light
 * @brief Describes a light source for the lighting system.
 */
struct light
{
    light_type type;
    color5551 ambient;
    color5551 diffuse;
    color5551 specular;
    vec3<real> position;
    real spot_cutoff;
    vec3<real> spot_direction;
    real spot_exponent;
    real constant_attenuation;
    real linear_attenuation;
    real quadratic_attenuation;
};

/**
 * @struct material
 * @brief Describes the surface properties of an object for lighting.
 */
struct material
{
    class texture *texture = nullptr;
    color5551 ambient;
    color5551 diffuse;
    color5551 specular;
    color5551 emission;
    real shininess;
};

#pragma endregion

#pragma region Texture Interface

/**
 * @class texture
 * @brief An abstract handle to a GPU texture resource.
 *
 * Textures are created and managed by a `gpu_context` instance.
 */
class texture : public resource
{
public:
    static constexpr chunk_id CHUNK_ID = chunk_id("TEXD");

    virtual ~texture() = default;

    /** @brief Deletes the texture and releases its resources. */
    virtual void destroy() = 0;

    /** @brief Gets the size of the texture. */
    virtual vec2<uint16_t> get_size() const = 0;

    /**
     * @brief Loads pixel data into the texture.
     * @param data_size The size of the pixel data buffer in bytes.
     * @param data A pointer to the raw pixel data.
     */
    virtual void load(uint16_t width,
                      uint16_t height,
                      color_format format,
                      size_t data_size,
                      const void *data) = 0;

    /**
     * @brief Copies the texture's pixel data out to a buffer.
     * @param[out] width Pointer to store the texture width.
     * @param[out] height Pointer to store the texture height.
     * @param[out] format Pointer to store the texture format.
     * @param[out] pixel_data Buffer to copy the pixel data into.
     * @param[in,out] size On input, the size of the buffer. On output, the size
     * of the copied data.
     */
    virtual void copy(uint16_t *width,
                      uint16_t *height,
                      color_format *format,
                      void *pixel_data,
                      size_t *size) const = 0;

    /**
     * @brief Samples a color from the texture at given UV coordinates.
     * @param u The horizontal texture coordinate (0-255).
     * @param v The vertical texture coordinate (0-255).
     * @return The sampled color in 8888 format.
     */
    virtual color8888 sample8888(uint8_t u, uint8_t v) const = 0;

    /**
     * @brief Gets the color format of the texture.
     * @return The color format of the texture.
     */
    virtual color_format get_format() const = 0;
};

#pragma endregion

#pragma region Display List Interface

/**
 * @class display_list
 * @brief An abstract handle to a pre-compiled list of GPU commands.
 *
 * Display lists are created and managed by a `gpu_context` instance.
 */
class display_list : public resource
{
public:
    static constexpr chunk_id CHUNK_ID = chunk_id("DISL");

    virtual ~display_list() = default;

    /** @brief Deletes the display list and releases its resources. */
    virtual void destroy() = 0;
};

#pragma endregion

#pragma region GPU Interface

/**
 * @class gpu_context
 * @brief The main interface for all rendering operations.
 *
 * This class represents the state of the GPU and provides all functions
 * for drawing, transformations, and state management.
 */
class gpu
{
public:
    virtual ~gpu() = default;

#pragma region Basic Drawing

    virtual void new_frame()                                     = 0;
    virtual void begin(primitive_type type)                      = 0;
    virtual void end()                                           = 0;
    virtual void vertex(const vec3<real> &v)                     = 0;
    virtual void vertex(real x, real y, real z)                  = 0;
    virtual void color(const class color &c)                     = 0;
    virtual void color(real r, real g, real b, real a = real(1)) = 0;
    virtual void normal(const vec3<real> &n)                     = 0;
    virtual void normal(real x, real y, real z)                  = 0;
    virtual void tex_coord(const vec2<real> &uv)                 = 0;
    virtual void tex_coord(real u, real v)                       = 0;
    virtual void clear(const struct color &c, real depth)        = 0;
    virtual void viewport(int width, int height)                 = 0;

#pragma endregion

#pragma region Transforms

    virtual void set_matrix_mode(matrix_mode mode) = 0;
    virtual void
    perspective_fov(real fov, real aspect, real near, real far) = 0;
    virtual void ortho(real left,
                       real right,
                       real bottom,
                       real top,
                       real near,
                       real far)                                = 0;
    virtual void translate(const vec3<real> &t)                 = 0;
    virtual void translate(real x, real y, real z)              = 0;
    virtual void rotate(const vec3<real> &r)                    = 0;
    virtual void rotate(real x, real y, real z)                 = 0;
    virtual void scale(const vec3<real> &s)                     = 0;
    virtual void scale(real x, real y, real z)                  = 0;
    virtual void load_identity()                                = 0;
    virtual void load_matrix(const mat4<real> &m)               = 0;
    virtual void push_matrix()                                  = 0;
    virtual void pop_matrix()                                   = 0;

#pragma endregion

#pragma region Lighting

    virtual void set_shade_model(shade_model model) = 0;
    virtual void enable_lighting(bool enabled)      = 0;
    virtual void set_light(int id, const light *l)  = 0;
    virtual void set_material(const material *m)    = 0;

#pragma endregion

#pragma region Textures

    virtual texture *
    create_texture(uint16_t width, uint16_t height, color_format format) = 0;
    virtual void bind_texture(texture *tex)                              = 0;
    virtual void unbind_texture()                                        = 0;

#pragma endregion

#pragma region Display Lists

    virtual display_list *create_display_list()              = 0;
    virtual void begin_display_list(display_list *list)      = 0;
    virtual void end_display_list()                          = 0;
    virtual void call_display_list(const display_list *list) = 0;

#pragma endregion

#pragma region Fog / Depth Cueing

    virtual void enable_fog(bool enabled)             = 0;
    virtual void set_fog_start(float start)           = 0;
    virtual void set_fog_end(float end)               = 0;
    virtual void set_fog_color(const struct color &c) = 0;

#pragma endregion

    // New methods for ImGui support
    virtual void enable_depth_test(bool enabled) = 0;
    virtual void enable_blend(bool enabled) = 0;
    virtual void set_blend_func(blend_factor src, blend_factor dst) = 0;
    virtual void enable_scissor_test(bool enabled) = 0;
    virtual void set_scissor(int x, int y, int width, int height) = 0;
    virtual void set_viewport_rect(int x, int y, int width, int height) = 0;
};

#pragma region Global API Functions

/**
 * @brief Initializes the GPU context and returns a pointer to it.
 * @return A pointer to the created GPU context, or nullptr on failure.
 */
gpu *init_gpu();

#pragma endregion

} // namespace zabato

#include <zabato/gpu_resource.hpp>