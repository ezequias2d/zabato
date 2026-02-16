#pragma once

#include <zabato/game_message.hpp>
#include <zabato/spatial.hpp>
#include <zabato/vector.hpp>

namespace zabato::editor
{

class hierarchy_window
{
public:
    hierarchy_window() = default;

    void render(spatial *root, class editor_app &app);
    void on_message(const game_message &msg, class editor_app &app);

private:
    void draw_node(spatial *node, class editor_app &app);
    void check_prefab_drop(editor_app &app, uuid parent_id);

    vector<object *> m_selection;

    symbol_ref cmd_instantiate_prefab = "cmd_instantiate_prefab";
    symbol_ref cmd_create_node        = "cmd_create_node";
    symbol_ref cmd_create_light       = "cmd_create_light";
    symbol_ref cmd_create_model       = "cmd_create_model";
    symbol_ref cmd_select             = "cmd_select";
    symbol_ref cmd_deselect           = "cmd_deselect";
    symbol_ref evt_scene_change       = "evt_scene_change";
    symbol_ref cmd_delete_object      = "cmd_delete_object";
    symbol_ref cmd_focus_selection    = "cmd_focus_selection";
};

} // namespace zabato::editor
