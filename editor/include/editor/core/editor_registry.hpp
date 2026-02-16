#pragma once

#include <zabato/delegate.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/real.hpp>
#include <zabato/rtti.hpp>

namespace zabato::editor
{
class editor_app;

using editor_draw_func  = delegate<void(void *, editor_app &, real)>;
using preview_draw_func = delegate<void(void *, editor_app &, real)>;

class editor_registry
{
public:
    static void register_editor(const rtti &type, editor_draw_func func);
    static editor_draw_func get_editor(const rtti &type);

    static void register_preview(const rtti &type, preview_draw_func func);
    static preview_draw_func get_preview(const rtti &type);

    // Deep search
    static editor_draw_func find_editor(const rtti &type);
    static preview_draw_func find_preview(const rtti &type);

private:
    static hash_map<const rtti *, editor_draw_func> s_editors;
    static hash_map<const rtti *, preview_draw_func> s_previews;
};

void register_material_inspector();
void register_mesh_inspector();
void register_texture_inspector();
void register_object_inspector();
} // namespace zabato::editor
