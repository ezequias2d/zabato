#pragma once

#include <zabato/model.hpp>

namespace zabato
{
class spatial;
class camera;
class gpu;

class renderer
{
public:
    virtual ~renderer() = default;

    virtual void begin(pointer<camera> cam) = 0;
    virtual void end()                      = 0;

    /**
     * @brief Submit a model for rendering.
     * @param model The model to render.
     */
    virtual void submit(pointer<model> model) = 0;

    /**
     * @brief Submit a light for the frame.
     * @param light The light to submit.
     */
    virtual void submit(pointer<class light> light) = 0;
};

class simple_renderer : public renderer
{
public:
    simple_renderer(gpu &gpu) : m_gpu(gpu) {}

    void begin(pointer<camera> cam) override final;
    void end() override final;

    void submit(pointer<model> model) override final;
    void submit(pointer<class light> light) override final;

private:
    pointer<camera> m_cam;
    gpu &m_gpu;
    int m_active_lights = 0;
};

class forward_renderer : public renderer
{
public:
    forward_renderer(gpu &gpu, script_system &script_system)
        : m_gpu(gpu), m_script_system(script_system)
    {
    }

    void begin(pointer<camera> cam) override final;
    void end() override final;

    void submit(pointer<model> model) override final;
    void submit(pointer<class light> light) override final;

private:
    pointer<camera> m_cam;
    gpu &m_gpu;
    script_system &m_script_system;
    int m_active_lights = 0;
};

} // namespace zabato
