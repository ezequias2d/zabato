#pragma once

#include <imgui.h>
#include <zabato/hash_map.hpp>

namespace zabato::editor::widgets
{

/**
 * @class node_canvas
 * @brief Minimal reusable node-graph canvas built on ImGui.
 *
 * Provides pan (right-mouse drag), a background hit area (for context menus),
 * node drawing with drag-to-move, and link rendering between previously-drawn
 * nodes by id.
 *
 * Usage within an ImGui window:
 * ```
 * canvas.begin("my_graph");
 * for each node: canvas.draw_node(id, label, size, pos);
 * for each link: auto lr = canvas.draw_link(src_id, dst_id, color, thick);
 *                if (lr.clicked) { ... }
 * canvas.end();
 * ```
 */
class node_canvas
{
public:
    struct node_result
    {
        bool clicked       = false;
        bool right_clicked = false;
        bool selected      = false;
        bool ctrl_held     = false;
    };

    struct link_result
    {
        bool hovered   = false;         ///< Mouse is near the link curve.
        bool clicked   = false;         ///< Left-clicked while hovered.
        ImVec2 midpoint = {0.f, 0.f};  ///< Screen-space midpoint of the curve.
    };

    /** Per-node styling. All colors are ImU32 (IM_COL32). */
    struct node_style
    {
        const char *subtitle = nullptr;
        ImU32 title_color    = IM_COL32(90, 90, 100, 255);
        ImU32 body_color     = IM_COL32(40, 40, 45, 255);
        ImU32 border_color   = IM_COL32(120, 120, 120, 255);
        bool is_current      = false;
    };

    /** Begin a canvas region. Must be inside an ImGui window/child. */
    void begin(const char *id, ImVec2 size = ImVec2(0, 0));
    void end();

    /** Draw a single node with default styling. `pos_inout` is top-left in
     *  canvas space; updated if the user drags the node. */
    node_result draw_node(int id,
                          const char *label,
                          ImVec2 size,
                          ImVec2 &pos_inout,
                          bool highlighted = false);

    /** Styled variant. */
    node_result draw_node(int id,
                          const char *label,
                          ImVec2 size,
                          ImVec2 &pos_inout,
                          const node_style &style);

    /** Draw a link between two previously-drawn nodes. `bend` is a
     *  screen-space offset applied to the curve's control points, letting the
     *  caller route the path around obstacles. Returns hover/click state and
     *  the screen-space midpoint of the drawn curve. */
    link_result draw_link(int src_id,
                          int dst_id,
                          ImU32 color     = IM_COL32(120, 200, 255, 220),
                          float thickness = 2.0f,
                          ImVec2 bend     = {0.f, 0.f});

    /** Draw an in-progress link from a node to a free screen-space point. */
    void draw_pending_link(int src_id,
                           ImVec2 to_screen,
                           ImU32 color     = IM_COL32(255, 200, 100, 220),
                           float thickness = 2.0f);

    bool bg_clicked() const { return m_bg_clicked; }
    bool bg_right_clicked() const { return m_bg_right_clicked; }
    ImVec2 mouse_canvas_pos() const;

    ImVec2 offset() const { return m_offset; }
    void set_offset(ImVec2 o) { m_offset = o; }

    int selected_node() const { return m_selected_node; }
    void clear_selection() { m_selected_node = -1; }
    void set_selected_node(int id) { m_selected_node = id; }

    // Rubber-band selection (LMB drag on background)
    bool   is_rubber_banding() const { return m_rubber_banding; }
    ImVec4 rubber_band_rect_canvas() const { return m_rubber_band_rect; }

    // Multi-drag: which node was dragged this frame and by how much (canvas space)
    int    drag_node_id() const { return m_drag_node_id; }
    ImVec2 drag_delta()   const { return m_drag_delta; }

    // Canvas zoom (Ctrl+scroll). Range [0.15, 4.0].
    float  zoom() const { return m_zoom; }
    void   set_zoom(float z) { m_zoom = z < 0.15f ? 0.15f : (z > 4.f ? 4.f : z); }

    // Minimap overlay (bottom-right corner). On by default.
    bool show_minimap() const { return m_show_minimap; }
    void set_show_minimap(bool v) { m_show_minimap = v; }

private:
    struct node_rec
    {
        ImVec2 tl_screen;
        ImVec2 size;
        ImU32  minimap_color = IM_COL32(60, 100, 160, 200);
    };

    ImVec2 m_canvas_p0      = ImVec2(0, 0);
    ImVec2 m_canvas_sz      = ImVec2(0, 0);
    ImVec2 m_offset         = ImVec2(0, 0);
    ImDrawList *m_dl        = nullptr;
    int m_selected_node     = -1;
    bool m_bg_clicked       = false;
    bool m_bg_right_clicked = false;
    bool m_background_hovered = false;
    bool m_panning          = false;
    bool m_any_node_clicked = false;
    ImVec2 m_right_down_pos = ImVec2(0, 0);

    // Rubber-band
    bool    m_lmb_bg_down         = false;
    ImVec2  m_lmb_bg_down_canvas  = {};
    bool    m_rubber_banding      = false;
    ImVec4  m_rubber_band_rect    = {};

    // Multi-drag (per-frame, reset in begin())
    int     m_drag_node_id        = -1;
    ImVec2  m_drag_delta          = {};

    // Zoom
    float   m_zoom                = 1.0f;

    // Minimap
    bool    m_show_minimap        = true;
    ImVec4  m_mm_rect             = {};  // screen-space rect, recomputed each begin()

    hash_map<int, node_rec> m_frame_nodes;

    ImVec2 canvas_origin() const
    {
        return ImVec2(m_canvas_p0.x + m_offset.x, m_canvas_p0.y + m_offset.y);
    }

    ImVec2 canvas_to_screen(float cx, float cy) const
    {
        ImVec2 o = canvas_origin();
        return ImVec2(o.x + cx * m_zoom, o.y + cy * m_zoom);
    }

    bool is_mouse_over_any_node() const;
    bool is_mouse_in_minimap() const;
    void draw_minimap_overlay();
};

} // namespace zabato::editor::widgets
