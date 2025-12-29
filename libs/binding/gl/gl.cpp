#include <iostream>
#include <zabato/gl.hpp>
#include <zabato/window.hpp>

namespace zabato
{
#ifndef __EMSCRIPTEN__
void GLAPIENTRY gl_message_callback(GLenum source,
                                    GLenum type,
                                    GLuint id,
                                    GLenum severity,
                                    GLsizei length,
                                    const GLchar *message,
                                    const void *userParam)
{
    (void)source;
    (void)id;
    (void)length;
    (void)userParam;
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    std::cerr << "GL CALLBACK: "
              << (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "")
              << " type = 0x" << std::hex << type << ", severity = 0x"
              << severity << ", message = " << message << std::endl;
}
#endif // __EMSCRIPTEN__

GLenum to_gl_primitive_type(primitive_type pt)
{
    switch (pt)
    {
    case primitive_type::quads:
        return GL_QUADS;
    case primitive_type::triangles:
        return GL_TRIANGLES;
    case primitive_type::points:
        return GL_POINTS;
    case primitive_type::lines:
        return GL_LINES;
    }
    return 0;
}

GLenum to_gl_matrix_mode(matrix_mode mm)
{
    switch (mm)
    {
    case matrix_mode::modelview:
        return GL_MODELVIEW;
    case matrix_mode::projection:
        return GL_PROJECTION;
    case matrix_mode::texture:
        return GL_TEXTURE;
    }
    return 0;
}

GLenum to_gl_shade_model(shade_model sm)
{
    switch (sm)
    {
    case shade_model::flat:
        return GL_FLAT;
    case shade_model::smooth:
        return GL_SMOOTH;
    }
    return 0;
}

size_t palette_count(color_format format)
{
    switch (format)
    {
    case color_format::palette16:
        return 16;
    case color_format::palette64:
        return 64;
    case color_format::palette128:
        return 128;
    case color_format::palette256:
        return 256;
    default:
        return 0;
    }
}

size_t color_format_bits(color_format format)
{
    switch (format)
    {
    case color_format::rgba5551:
        return 16;
    case color_format::rgba4444:
        return 16;
    case color_format::palette16:
        return 4;
    case color_format::palette64:
    case color_format::palette128:
    case color_format::palette256:
        return 8;
    default:
        return 0;
    }
}

size_t calculate_texture_data_size(uint16_t width,
                                   uint16_t height,
                                   color_format format)
{
    size_t pcount = palette_count(format);
    if (pcount > 0)
    {
        size_t palette_size = pcount * sizeof(uint32_t);
        size_t index_size =
            ((width * color_format_bits(format) + 7) / 8) * height;
        return palette_size + index_size;
    }
    else
    {
        return width * height * (color_format_bits(format) / 8);
    }
}

GlTexture::GlTexture(uint16_t width, uint16_t height, color_format format)
    : m_width(width), m_height(height), m_format(format)
{
    glGenTextures(1, &m_handle);
}

GlTexture::~GlTexture() { destroy(); }

void GlTexture::destroy()
{
    if (m_handle)
        glDeleteTextures(1, &m_handle);
}

void GlTexture::load(uint16_t width,
                     uint16_t height,
                     color_format format,
                     size_t data_size,
                     const void *data)
{
    m_width  = width;
    m_height = height;
    m_format = format;

    const size_t expected_size =
        calculate_texture_data_size(width, height, format);

    if (data != nullptr)
    {
        if (data_size < expected_size)
        {
            std::cerr << "Error loading texture: provided data size ("
                      << data_size << ") is less than expected size ("
                      << expected_size << ")." << std::endl;
            return;
        }

        const uint8_t *byte_data = static_cast<const uint8_t *>(data);
        m_pixel_data.assign(byte_data, byte_data + expected_size);
    }
    else
        m_pixel_data.clear();

    vector<uint32_t> buffer;
    GLint internal_format = GL_RGBA8;

    if (m_format == color_format::rgba5551)
        internal_format = GL_RGB5_A1;
    else if (m_format == color_format::rgba4444)
        internal_format = GL_RGBA4;

    // Only process conversion if data is provided
    if (data != nullptr)
    {
        buffer.resize(m_width * m_height);

        switch (m_format)
        {
        case color_format::rgba5551:
        {
            const uint16_t *pixels = static_cast<const uint16_t *>(data);
            for (size_t i = 0; i < buffer.size(); ++i)
            {
                color color(color5551{pixels[i]});
                buffer[i] = color8888(color).value;
            }
            break;
        }
        case color_format::rgba4444:
        {
            const uint16_t *pixels = static_cast<const uint16_t *>(data);
            for (size_t i = 0; i < buffer.size(); ++i)
            {
                buffer[i] = color8888(color(color4444{pixels[i]})).value;
            }
            break;
        }
        case color_format::palette16:
        case color_format::palette64:
        case color_format::palette128:
        case color_format::palette256:
        {
            const bool is_nibble    = m_format == color_format::palette16;
            const size_t pcount     = palette_count(m_format);
            const uint32_t *palette = static_cast<const uint32_t *>(data);
            const uint8_t *indices =
                reinterpret_cast<const uint8_t *>(palette + pcount);

            for (size_t i = 0; i < buffer.size(); i++)
            {
                uint8_t index = is_nibble
                                    ? (indices[i / 2] >> (i % 2 ? 0 : 4)) & 0xF
                                    : indices[i];
                if (index < pcount)
                {
                    buffer[i] = palette[index];
                }
            }
            break;
        }
        case color_format::rgba8888:
        {
            const uint32_t *pixels = static_cast<const uint32_t *>(data);
            std::copy(pixels, pixels + (m_width * m_height), buffer.begin());
            break;
        }
        default:
            std::cerr << "Unknown texture format: " << (int)m_format
                      << std::endl;
            return;
        }
    }

    glBindTexture(GL_TEXTURE_2D, m_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 internal_format,
                 m_width,
                 m_height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 (data != nullptr) ? buffer.data() : nullptr);
}

void GlTexture::copy(uint16_t *width,
                     uint16_t *height,
                     color_format *format,
                     void *pixel_data,
                     size_t *size) const
{
    if (width)
        *width = m_width;
    if (height)
        *height = m_height;
    if (format)
        *format = m_format;
    if (size)
        *size = m_pixel_data.size();
    if (pixel_data)
    {
        memcpy(pixel_data, m_pixel_data.data(), m_pixel_data.size());
    }
}

color8888 GlTexture::sample8888(uint8_t u, uint8_t v) const
{
    // TODO: implement texture sampling
    return color8888{0};
}

color_format GlTexture::get_format() const { return m_format; }

vec2<uint16_t> GlTexture::get_size() const { return {m_width, m_height}; }

GlFramebuffer::GlFramebuffer(uint16_t width, uint16_t height)
{
    m_texture = new GlTexture(
        width, height, color_format::rgba8888); // Assuming 8888 for FBO
    glGenFramebuffers(1, &m_handle);
    glGenRenderbuffers(1, &m_depth_renderbuffer);
    update();
}

GlFramebuffer::~GlFramebuffer()
{
    destroy();
    if (m_texture)
    {
        delete m_texture;
        m_texture = nullptr;
    }
}

void GlFramebuffer::destroy()
{
    if (m_handle)
    {
        glDeleteFramebuffers(1, &m_handle);
        m_handle = 0;
    }
    if (m_depth_renderbuffer)
    {
        glDeleteRenderbuffers(1, &m_depth_renderbuffer);
        m_depth_renderbuffer = 0;
    }
}

void GlFramebuffer::resize(uint16_t width, uint16_t height)
{
    if (m_texture->get_size().x == width && m_texture->get_size().y == height)
        return;

    // Resize texture by reloading with null data
    m_texture->load(width, height, color_format::rgba8888, 0, nullptr);
    update();
}

void GlFramebuffer::update()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);

    // Attach texture
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           m_texture->get_handle(),
                           0);

    // Attach depth buffer
    glBindRenderbuffer(GL_RENDERBUFFER, m_depth_renderbuffer);
    vec2<uint16_t> size = m_texture->get_size();
    glRenderbufferStorage(
        GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size.x, size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER,
                              m_depth_renderbuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Error: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GlDisplayList::GlDisplayList()
{
    m_handle = glGenLists(1);
    if (m_handle == 0)
    {
        std::cerr << "Failed to generate display list!" << std::endl;
    }
}

GlDisplayList::~GlDisplayList() { destroy(); }

void GlDisplayList::destroy()
{
    if (m_handle != 0)
    {
        glDeleteLists(m_handle, 1);
        m_handle = 0;
    }
}

GLuint GlDisplayList::get_handle() const { return m_handle; }

GlGpu::GlGpu() {}

void GlGpu::new_frame()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
}

void GlGpu::begin(primitive_type type) { glBegin(to_gl_primitive_type(type)); }
void GlGpu::end() { glEnd(); }
void GlGpu::vertex(const vec3<real> &v)
{
    glVertex3f(float(v.x), float(v.y), float(v.z));
}
void GlGpu::vertex(real x, real y, real z)
{
    glVertex3f(float(x), float(y), float(z));
}

void GlGpu::color(const class color &c)
{
    glColor4f(float(c.r), float(c.g), float(c.b), float(c.a));
}

void GlGpu::color(real r, real g, real b, real a)
{
    glColor4f(float(r), float(g), float(b), float(a));
}

void GlGpu::normal(const vec3<real> &n)
{
    glNormal3f(float(n.x), float(n.y), float(n.z));
}
void GlGpu::normal(real x, real y, real z)
{
    glNormal3f(float(x), float(y), float(z));
}
void GlGpu::tex_coord(const vec2<real> &uv)
{
    glTexCoord2f(float(uv.x), float(uv.y));
}
void GlGpu::tex_coord(real u, real v) { glTexCoord2f(float(u), float(v)); }

void GlGpu::clear(const struct color &c, real depth)
{
    glClearColor(float(c.r), float(c.g), float(c.b), float(c.a));
    glClearDepth(float(depth));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GlGpu::viewport(int width, int height) { glViewport(0, 0, width, height); }

void GlGpu::push_state() { glPushAttrib(GL_ALL_ATTRIB_BITS); }
void GlGpu::pop_state() { glPopAttrib(); }
void GlGpu::set_matrix_mode(matrix_mode mode)
{
    glMatrixMode(to_gl_matrix_mode(mode));
}

void GlGpu::perspective_fov(real fov, real aspect, real near_p, real far_p)
{
    mat4<real> mat   = mat4_perspective_fov(fov, aspect, near_p, far_p);
    mat4<float> fmat = mat;
    glMultMatrixf(&fmat.m00);
}

void GlGpu::ortho(real left,
                  real right,
                  real bottom,
                  real top,
                  real near_p,
                  real far_p)
{
    mat4<real> mat   = mat4_ortho(left, right, bottom, top, near_p, far_p);
    mat4<float> fmat = mat;
    glMultMatrixf(&fmat.m00);
}

void GlGpu::translate(const vec3<real> &t)
{
    glTranslatef(float(t.x), float(t.y), float(t.z));
}
void GlGpu::translate(real x, real y, real z)
{
    glTranslatef(float(x), float(y), float(z));
}

void GlGpu::rotate(const vec3<real> &r)
{
    mat4<real> mat   = mat4_from_quat(quat_from_axis_angle({1, 0, 0}, r.x) *
                                    quat_from_axis_angle({0, 1, 0}, r.y) *
                                    quat_from_axis_angle({0, 0, 1}, r.z));
    mat4<float> fmat = mat;
    glMultMatrixf(&fmat.m00);
}

void GlGpu::rotate(real x, real y, real z) { rotate({x, y, z}); }
void GlGpu::scale(const vec3<real> &s)
{
    glScalef(float(s.x), float(s.y), float(s.z));
}
void GlGpu::scale(real x, real y, real z)
{
    glScalef(float(x), float(y), float(z));
}
void GlGpu::load_identity() { glLoadIdentity(); }
void GlGpu::load_matrix(const mat4<real> &m)
{
    mat4<float> fm(m);
    glLoadMatrixf(&fm.m00);
}
void GlGpu::push_matrix() { glPushMatrix(); }
void GlGpu::pop_matrix() { glPopMatrix(); }

void GlGpu::set_shade_model(shade_model model)
{
    glShadeModel(to_gl_shade_model(model));
}
void GlGpu::enable_lighting(bool enabled)
{
    if (enabled)
        glEnable(GL_LIGHTING);
    else
        glDisable(GL_LIGHTING);
}
void GlGpu::set_light(int id, const light *l)
{
    const GLenum light_id = GL_LIGHT0 + id;
    if (!l)
    {
        glDisable(light_id);
        return;
    }
    glEnable(light_id);
    float ambient[]  = {float(zabato::color(l->ambient).r),
                        float(zabato::color(l->ambient).g),
                        float(zabato::color(l->ambient).b),
                        1.0f};
    float diffuse[]  = {float(zabato::color(l->diffuse).r),
                        float(zabato::color(l->diffuse).g),
                        float(zabato::color(l->diffuse).b),
                        1.0f};
    float specular[] = {float(zabato::color(l->specular).r),
                        float(zabato::color(l->specular).g),
                        float(zabato::color(l->specular).b),
                        1.0f};
    glLightfv(light_id, GL_AMBIENT, ambient);
    glLightfv(light_id, GL_DIFFUSE, diffuse);
    glLightfv(light_id, GL_SPECULAR, specular);

    float pos[] = {float(l->position.x),
                   float(l->position.y),
                   float(l->position.z),
                   l->type == light_type::directional ? 0.0f : 1.0f};
    glLightfv(light_id, GL_POSITION, pos);

    if (l->type == light_type::spot)
    {
        float dir[] = {float(l->spot_direction.x),
                       float(l->spot_direction.y),
                       float(l->spot_direction.z)};
        glLightfv(light_id, GL_SPOT_DIRECTION, dir);
        glLightf(light_id, GL_SPOT_CUTOFF, float(l->spot_cutoff));
        glLightf(light_id, GL_SPOT_EXPONENT, float(l->spot_exponent));
    }
}
void GlGpu::set_material(const material *m)
{ /* ... implementation needed ... */ }

texture *
GlGpu::create_texture(uint16_t width, uint16_t height, color_format format)
{
    return new GlTexture(width, height, format);
}
void GlGpu::bind_texture(texture *tex)
{
    if (tex)
    {
        glBindTexture(GL_TEXTURE_2D,
                      static_cast<GlTexture *>(tex)->get_handle());
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}
void GlGpu::unbind_texture() { glBindTexture(GL_TEXTURE_2D, 0); }

framebuffer *GlGpu::create_framebuffer(uint16_t width, uint16_t height)
{
    return new GlFramebuffer(width, height);
}

void GlGpu::bind_framebuffer(framebuffer *fb)
{
    if (fb)
        glBindFramebuffer(GL_FRAMEBUFFER,
                          static_cast<GlFramebuffer *>(fb)->get_handle());
    else
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GlGpu::unbind_framebuffer() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

display_list *GlGpu::create_display_list() { return new GlDisplayList(); }

void GlGpu::begin_display_list(display_list *list)
{
    if (!list)
        return;
    glNewList(static_cast<GlDisplayList *>(list)->get_handle(), GL_COMPILE);
}

void GlGpu::end_display_list() { glEndList(); }

void GlGpu::call_display_list(const display_list *list)
{
    if (!list)
        return;
    glCallList(static_cast<const GlDisplayList *>(list)->get_handle());
}

void GlGpu::enable_fog(bool enabled)
{
    if (enabled)
        glEnable(GL_FOG);
    else
        glDisable(GL_FOG);
}
void GlGpu::set_fog_start(float start) { glFogf(GL_FOG_START, start); }
void GlGpu::set_fog_end(float end) { glFogf(GL_FOG_END, end); }
void GlGpu::set_fog_color(const struct color &c)
{
    float color[] = {float(c.r), float(c.g), float(c.b), float(c.a)};
    glFogfv(GL_FOG_COLOR, color);
}

GLenum to_gl_blend_factor(blend_factor bf)
{
    switch (bf)
    {
    case blend_factor::zero:
        return GL_ZERO;
    case blend_factor::one:
        return GL_ONE;
    case blend_factor::src_color:
        return GL_SRC_COLOR;
    case blend_factor::one_minus_src_color:
        return GL_ONE_MINUS_SRC_COLOR;
    case blend_factor::src_alpha:
        return GL_SRC_ALPHA;
    case blend_factor::one_minus_src_alpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    case blend_factor::dst_alpha:
        return GL_DST_ALPHA;
    case blend_factor::one_minus_dst_alpha:
        return GL_ONE_MINUS_DST_ALPHA;
    case blend_factor::dst_color:
        return GL_DST_COLOR;
    case blend_factor::one_minus_dst_color:
        return GL_ONE_MINUS_DST_COLOR;
    case blend_factor::src_alpha_saturate:
        return GL_SRC_ALPHA_SATURATE;
    default:
        return GL_ZERO;
    }
}

void GlGpu::enable_depth_test(bool enabled)
{
    if (enabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void GlGpu::enable_blend(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
        glDisable(GL_BLEND);
}

void GlGpu::set_blend_func(blend_factor src, blend_factor dst)
{
    glBlendFunc(to_gl_blend_factor(src), to_gl_blend_factor(dst));
}

void GlGpu::enable_scissor_test(bool enabled)
{
    if (enabled)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
}

void GlGpu::set_scissor(int x, int y, int width, int height)
{
    glScissor(x, y, width, height);
}

void GlGpu::set_viewport_rect(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

gpu *init_gpu()
{
    std::cout << "Initializing GPU..." << std::endl;

#ifndef __EMSCRIPTEN__
    // GLAD is only needed for desktop OpenGL
    // Emscripten provides GL functions directly through emulation
    if (!gladLoadGLLoader((GLADloadproc)get_proc_address))
    {
        std::cout << "gladLoadGLLoader failed, trying gladLoadGL..."
                  << std::endl;
        if (!gladLoadGL())
        {
            std::cerr << "Failed to load GL functions!" << std::endl;
            return nullptr;
        }
    }
    std::cout << "GL functions loaded successfully" << std::endl;
#else
    std::cout << "Emscripten: initialize_gl4es()" << std::endl;
    initialize_gl4es();
#endif

    std::cout << "GL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GL Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "GL Renderer: " << glGetString(GL_RENDERER) << std::endl;

#ifndef NDEBUG
#ifndef __EMSCRIPTEN__
    if (GLAD_GL_KHR_debug)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(gl_message_callback, 0);
    }
#endif
#endif
    return new GlGpu();
}
} // namespace zabato