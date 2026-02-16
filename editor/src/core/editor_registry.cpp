#include <editor/core/editor_registry.hpp>

namespace zabato::editor
{

hash_map<const rtti *, editor_draw_func> editor_registry::s_editors;
hash_map<const rtti *, preview_draw_func> editor_registry::s_previews;

void editor_registry::register_editor(const rtti &type, editor_draw_func func)
{
    s_editors.add_or_set(&type, func);
}

editor_draw_func editor_registry::get_editor(const rtti &type)
{
    auto it = s_editors.find(&type);
    if (it != s_editors.end())
        return it->value;
    return editor_draw_func();
}

void editor_registry::register_preview(const rtti &type, preview_draw_func func)
{
    s_previews.add_or_set(&type, func);
}

preview_draw_func editor_registry::get_preview(const rtti &type)
{
    auto it = s_previews.find(&type);
    if (it != s_previews.end())
        return it->value;
    return preview_draw_func();
}

editor_draw_func editor_registry::find_editor(const rtti &type)
{
    const rtti *current = &type;
    while (current)
    {
        auto func = get_editor(*current);
        if (func.is_valid())
            return func;
        current = current->base();
    }
    return editor_draw_func();
}

preview_draw_func editor_registry::find_preview(const rtti &type)
{
    const rtti *current = &type;
    while (current)
    {
        auto func = get_preview(*current);
        if (func.is_valid())
            return func;
        current = current->base();
    }
    return preview_draw_func();
}

} // namespace zabato::editor
