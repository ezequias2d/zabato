#pragma once

#include <zabato/camera.hpp>
#include <zabato/delegate.hpp>
#include <zabato/gpu.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/picking.hpp>
#include <zabato/rtti.hpp>
#include <zabato/spatial.hpp>

namespace zabato::editor
{
class editor_app;

struct gizmo_context
{
    zabato::gpu &gpu;
    const camera &cam;
    const ray3<real> *mouse_ray;
    editor_app &app;
    zabato::color color = zabato::color::white();
    bool occluded       = false;

    // Output
    spatial *hovered = nullptr;
    real hit_dist    = -1.0;
    bool selected    = false;
};

using gizmo_draw_callback = delegate<void(const spatial *, gizmo_context &)>;

class gizmo_registry
{
public:
    static void register_drawer(const rtti &type, gizmo_draw_callback callback);

    static void draw(const spatial *node, gizmo_context &ctx);

private:
    static hash_map<const rtti *, gizmo_draw_callback> s_drawers;
};

} // namespace zabato::editor
