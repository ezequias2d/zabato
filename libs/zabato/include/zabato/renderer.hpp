#pragma once

#include "zabato/model.hpp"
namespace zabato
{
class spatial;
class camera;
class gpu;

class renderer
{
public:
    virtual ~renderer() = default;

    virtual void begin(camera &cam) = 0;
    virtual void end()              = 0;

    /**
     * @brief Submit a model for rendering.
     * @param model The model to render.
     */
    virtual void submit(model *model) = 0;
};

class simple_renderer : public renderer
{
public:
    simple_renderer(gpu &gpu) : m_gpu(gpu) {}

    void begin(camera &cam) override final;
    void end() override final;

    void submit(model *model) override final;

private:
    camera *m_cam;
    gpu &m_gpu;
};

} // namespace zabato
