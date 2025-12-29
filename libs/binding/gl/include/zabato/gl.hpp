#pragma once
#include <zabato/gpu.hpp>
#include <zabato/vector.hpp>

#ifdef __EMSCRIPTEN__
#include <GL/gl.h>
#include <GL/glext.h>
#include <gl4esinit.h>
#else
#include <glad/glad.h>
#endif

namespace zabato
{
GLenum to_gl_primitive_type(primitive_type pt);
GLenum to_gl_matrix_mode(matrix_mode mm);
GLenum to_gl_shade_model(shade_model sm);

class GlTexture : public texture
{
public:
    GlTexture(uint16_t width, uint16_t height, color_format format);
    ~GlTexture() override;

    void destroy() override;
    void load(uint16_t width,
              uint16_t height,
              color_format format,
              size_t data_size,
              const void *data) override;
    void copy(uint16_t *width,
              uint16_t *height,
              color_format *format,
              void *pixel_data,
              size_t *size) const override;
    color8888 sample8888(uint8_t u, uint8_t v) const override;

    GLuint get_handle() const { return m_handle; }

    color_format get_format() const override;

    vec2<uint16_t> get_size() const override;

private:
    GLuint m_handle = 0;
    uint16_t m_width;
    uint16_t m_height;
    color_format m_format;
    vector<uint8_t> m_pixel_data;
};

class GlFramebuffer : public framebuffer
{
public:
    GlFramebuffer(uint16_t width, uint16_t height);
    ~GlFramebuffer() override;

    void destroy() override;
    texture *get_texture() const override { return m_texture; }
    void resize(uint16_t width, uint16_t height) override;
    vec2<uint16_t> get_size() const override { return m_texture->get_size(); }

    GLuint get_handle() const { return m_handle; }

    // Helper to ensure texture is valid
    void update();

private:
    GLuint m_handle             = 0;
    GLuint m_depth_renderbuffer = 0;
    GlTexture *m_texture        = nullptr;
};

class GlGpu : public gpu
{
public:
    GlGpu();
    ~GlGpu() override = default;

    void new_frame() override;
    void begin(primitive_type type) override;
    void end() override;
    void vertex(const vec3<real> &v) override;
    void vertex(real x, real y, real z) override;
    void color(const class color &c) override;
    void color(real r, real g, real b, real a) override;
    void normal(const vec3<real> &n) override;
    void normal(real x, real y, real z) override;
    void tex_coord(const vec2<real> &uv) override;
    void tex_coord(real u, real v) override;
    void clear(const struct color &c, real depth) override;
    void viewport(int width, int height) override;
    void push_state() override;
    void pop_state() override;

    void set_matrix_mode(matrix_mode mode) override;
    void perspective_fov(real fov, real aspect, real near, real far) override;
    void ortho(real left,
               real right,
               real bottom,
               real top,
               real near,
               real far) override;
    void translate(const vec3<real> &t) override;
    void translate(real x, real y, real z) override;
    void rotate(const vec3<real> &r) override;
    void rotate(real x, real y, real z) override;
    void scale(const vec3<real> &s) override;
    void scale(real x, real y, real z) override;
    void load_identity() override;
    void load_matrix(const mat4<real> &m) override;
    void push_matrix() override;
    void pop_matrix() override;

    void set_shade_model(shade_model model) override;
    void enable_lighting(bool enabled) override;
    void set_light(int id, const light *l) override;
    void set_material(const material *m) override;

    texture *create_texture(uint16_t width,
                            uint16_t height,
                            color_format format) override;
    void bind_texture(texture *tex) override;
    void unbind_texture() override;

    framebuffer *create_framebuffer(uint16_t width, uint16_t height) override;
    void bind_framebuffer(framebuffer *fb) override;
    void unbind_framebuffer() override;

    display_list *create_display_list() override;
    void begin_display_list(display_list *list) override;
    void end_display_list() override;
    void call_display_list(const display_list *list) override;

    void enable_fog(bool enabled) override;
    void set_fog_start(float start) override;
    void set_fog_end(float end) override;
    void set_fog_color(const struct color &c) override;

    void enable_depth_test(bool enabled) override;
    void enable_blend(bool enabled) override;
    void set_blend_func(blend_factor src, blend_factor dst) override;
    void enable_scissor_test(bool enabled) override;
    void set_scissor(int x, int y, int width, int height) override;
    void set_viewport_rect(int x, int y, int width, int height) override;
};

/**
 * @class GlDisplayList
 * @brief Concrete implementation of `display_list` for OpenGL.
 */
class GlDisplayList : public display_list
{
public:
    GlDisplayList();
    virtual ~GlDisplayList();
    void destroy() override final;
    GLuint get_handle() const;

private:
    GLuint m_handle = 0;
};

} // namespace zabato