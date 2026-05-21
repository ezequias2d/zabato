#pragma once

#include <imgui.h>

#include <editor/widgets/node_canvas.hpp>
#include <zabato/animator_graph.hpp>
#include <zabato/game_message.hpp>
#include <zabato/pointer.hpp>
#include <zabato/string.hpp>

namespace zabato::editor
{
class editor_app;

/**
 * @class animator_graph_window
 * @brief Editor panel for authoring animator_graph assets.
 *
 * Two-region layout: a left panel and a node canvas on the right.
 *
 * Left panel — shows the transition editor when a transition is selected
 * (m_sel_tr_src / m_sel_tr_idx), otherwise shows the message trigger
 * reference list collected from all transitions in the graph.
 *
 * Transition selection is index-pair based (no UUID / object::s_in_use):
 *   m_sel_tr_src  state index (or ANY_STATE_NODE_ID for any-state list)
 *   m_sel_tr_idx  transition index within that list
 *
 * Newly created transitions are auto-selected. Existing transitions can be
 * selected via right-click → "Edit tr -> X" on their source state node.
 */
class animator_graph_window
{
public:
    animator_graph_window() = default;

    void render(editor_app &app, real dtime);

    /** @brief Open an animator_graph asset by path for editing. */
    void open(editor_app &app, const string &path);

    /** @brief Open a new (empty) in-memory graph. */
    void open_empty(editor_app &app);

    /** @brief Hook an existing in-scene animator for live preview. */
    void set_preview_target(class zabato::animator *target)
    {
        m_preview = target;
    }

    void on_message(const game_message &msg, editor_app &app);

    bool is_open() const { return m_open; }

    /** Sentinel canvas id used for the permanent "Any State" pseudo-node. */
    static constexpr int ANY_STATE_NODE_ID = -1000;

    /** Sentinel for "no active link gesture" / "no selection". */
    static constexpr int LINK_NONE = -2;

private:
    bool m_open = false;
    string m_path;
    pointer<animator_graph> m_graph = nullptr;

    bool m_dirty = false;

    vector<ImVec2> m_node_positions;
    ImVec2 m_any_state_pos = ImVec2(30, 30);
    widgets::node_canvas m_canvas;

    vector<vector<ImVec2>> m_link_bends;
    vector<ImVec2> m_any_bends;
    bool m_bend_handle_active = false;

    // Copy/paste clipboard — stores a single cloned clip state.
    struct clipboard_state
    {
        bool valid = false;
        string name;
        resource_ref clip;
        bool loop       = true;
        real start_tick = real(0);
        real end_tick   = real(-1);
        vector<animator_transition> transitions;
        ImVec2 src_pos = {0.f, 0.f};
    };
    clipboard_state m_clipboard;
    vector<int> m_multi_sel;

    struct multi_clipboard_entry
    {
        string name;
        resource_ref clip;
        bool loop       = true;
        real start_tick = real(0);
        real end_tick   = real(-1);
        vector<animator_transition> transitions;
        ImVec2 rel_pos = {0.f, 0.f};
    };
    struct multi_clipboard_state
    {
        bool valid = false;
        vector<multi_clipboard_entry> entries;
    };
    multi_clipboard_state m_multi_clipboard;

    int m_linking_from = LINK_NONE;
    int m_ctx_node_id  = LINK_NONE;
    int m_sel_tr_src   = LINK_NONE;
    int m_sel_tr_idx   = -1;

    class animator *m_preview = nullptr;

    class animator_graph *m_preview_bound_graph = nullptr;
    void *m_preview_bound_skel                  = nullptr;

    // Whether to advance the preview animator each frame in edit mode.
    bool m_preview_playing = true;

    void render_toolbar(editor_app &app);
    void render_left_panel(editor_app &app);
    void render_canvas_panel(editor_app &app);

    void save_to_path(editor_app &app, const string &path);

    void ensure_positions();
    void ensure_link_bends();

    void delete_state_at(editor_app &app, int idx);
    void delete_selected_transition();
    void copy_state(int idx);
    void paste_state(editor_app &app);
    void duplicate_state(editor_app &app, int idx);
    void focus_state(int idx);
    void handle_shortcuts(editor_app &app);

    void select_state(editor_app &app, int state_index);
    void select_any_state(editor_app &app);

    /** @return Pointer to the currently selected transition, or nullptr. */
    animator_transition *resolve_selected_transition();

    // Multi-selection helpers
    bool is_multi_selected(int idx) const;
    void multi_sel_clear();
    void multi_sel_toggle(int idx);
    void multi_sel_add(int idx);
    void multi_sel_fix_after_delete(int deleted_idx);

    // Multi-selection operations
    void delete_multi_selection(editor_app &app);
    void copy_multi_selection();
    void cut_multi_selection(editor_app &app);
    void paste_multi_selection(editor_app &app);
    void duplicate_multi_selection(editor_app &app);
    void select_all(editor_app &app);
    void focus_multi_selection();
    void align_selected_h();
    void align_selected_v();
    void distribute_selected_h();
    void distribute_selected_v();
};

} // namespace zabato::editor
