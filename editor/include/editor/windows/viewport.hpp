#pragma once

#include <zabato/camera.hpp>
#include <zabato/gpu.hpp>
#include <zabato/renderer.hpp>
#include <zabato/world.hpp>

namespace zabato::editor
{

class viewport_window
{
public:
    viewport_window(const char *title);
    ~viewport_window();

    void init();
    void
    render(world &world, renderer &renderer, pointer<camera> cam, gpu &gpu);

    bool is_focused() const { return m_is_focused; }
    bool is_hovered() const { return m_is_hovered; }
    bool is_image_hovered() const { return m_is_image_hovered; }

    using render_callback_t = delegate<void(world &w, camera &cam, gpu &g)>;
    void set_on_scene_render(render_callback_t callback)
    {
        m_on_scene_render = callback;
    }

    using overlay_callback_t = delegate<void(world &w, camera &cam)>;
    void set_on_overlay_render(overlay_callback_t callback)
    {
        m_on_overlay_render = callback;
    }

    using drop_callback_t = delegate<void(const char *)>;
    void set_on_drop(drop_callback_t callback) { m_on_drop = callback; }

    vec2<real> get_size() const { return {m_width, m_height}; }

private:
    const char *m_title = "Viewport";
    framebuffer *m_fbo  = nullptr;
    int m_width         = 0;
    int m_height        = 0;

    // State
    bool m_is_focused       = false;
    bool m_is_hovered       = false;
    bool m_is_image_hovered = false;

    render_callback_t m_on_scene_render;
    overlay_callback_t m_on_overlay_render;
    drop_callback_t m_on_drop;
};

} // namespace zabato::editor
