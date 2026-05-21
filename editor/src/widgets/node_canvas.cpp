#include <editor/widgets/node_canvas.hpp>
#include <math.h>

namespace zabato::editor::widgets
{

// ---- geometry helpers -------------------------------------------------------

static ImVec2 node_border_pt(ImVec2 center, ImVec2 tl, ImVec2 sz, ImVec2 target)
{
    float dx = target.x - center.x;
    float dy = target.y - center.y;
    float t  = 1e9f;
    if (fabsf(dx) > 1e-3f)
    {
        float t0 = (tl.x - center.x) / dx;
        float t1 = (tl.x + sz.x - center.x) / dx;
        if (t0 > 1e-4f) t = fminf(t, t0);
        if (t1 > 1e-4f) t = fminf(t, t1);
    }
    if (fabsf(dy) > 1e-3f)
    {
        float t0 = (tl.y - center.y) / dy;
        float t1 = (tl.y + sz.y - center.y) / dy;
        if (t0 > 1e-4f) t = fminf(t, t0);
        if (t1 > 1e-4f) t = fminf(t, t1);
    }
    if (t > 1e8f)
        return ImVec2(center.x + sz.x * 0.5f, center.y);
    return ImVec2(center.x + t * dx, center.y + t * dy);
}

static float point_bezier_dist(ImVec2 p,
                                ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3)
{
    float min_d = 1e9f;
    for (int i = 0; i <= 20; ++i)
    {
        float t  = i / 20.f;
        float u  = 1.f - t;
        float x  = u*u*u*p0.x + 3*u*u*t*p1.x + 3*u*t*t*p2.x + t*t*t*p3.x;
        float y  = u*u*u*p0.y + 3*u*u*t*p1.y + 3*u*t*t*p2.y + t*t*t*p3.y;
        float dx = p.x - x, dy = p.y - y;
        float d  = sqrtf(dx * dx + dy * dy);
        if (d < min_d) min_d = d;
    }
    return min_d;
}

// ---- canvas -----------------------------------------------------------------

void node_canvas::begin(const char *id, ImVec2 size)
{
    m_frame_nodes.clear();
    m_bg_clicked        = false;
    m_bg_right_clicked  = false;
    m_any_node_clicked  = false;
    m_drag_node_id      = -1;
    m_drag_delta        = {};

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (size.x <= 0) size.x = avail.x;
    if (size.y <= 0) size.y = avail.y;
    if (size.y < 100) size.y = 100;
    m_canvas_sz = size;

    // A scrollbar-free child window isolates node InvisibleButtons (which may
    // land far outside the visible area when panning) from the parent window's
    // content-size tracking, preventing unwanted scrollbars on the parent.
    ImGui::BeginChild(id, m_canvas_sz, false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    m_canvas_p0 = ImGui::GetCursorScreenPos();

    // Pre-compute minimap rect so begin() can suppress background events when
    // the mouse is over the minimap overlay (drawn later in end()).
    if (m_show_minimap)
    {
        const float mm_w = 180.f, mm_h = 120.f, mm_mg = 8.f;
        m_mm_rect = ImVec4(m_canvas_p0.x + m_canvas_sz.x - mm_w - mm_mg,
                           m_canvas_p0.y + m_canvas_sz.y - mm_h - mm_mg,
                           m_canvas_p0.x + m_canvas_sz.x - mm_mg,
                           m_canvas_p0.y + m_canvas_sz.y - mm_mg);
    }

    m_dl = ImGui::GetWindowDrawList();
    ImVec2 p1(m_canvas_p0.x + m_canvas_sz.x, m_canvas_p0.y + m_canvas_sz.y);
    m_dl->AddRectFilled(m_canvas_p0, p1, IM_COL32(30, 30, 30, 255));
    // Clip all subsequent draw calls (grid, nodes, links) to the canvas region.
    // The border is drawn in end() after the pop so it always renders on top.
    m_dl->PushClipRect(m_canvas_p0, p1, true);

    const float grid_cs = 32.0f;
    const float grid_ss = grid_cs * m_zoom;
    for (float x = fmodf(m_offset.x, grid_ss); x < m_canvas_sz.x; x += grid_ss)
        m_dl->AddLine(ImVec2(m_canvas_p0.x + x, m_canvas_p0.y),
                      ImVec2(m_canvas_p0.x + x, p1.y),
                      IM_COL32(60, 60, 60, 120));
    for (float y = fmodf(m_offset.y, grid_ss); y < m_canvas_sz.y; y += grid_ss)
        m_dl->AddLine(ImVec2(m_canvas_p0.x, m_canvas_p0.y + y),
                      ImVec2(p1.x, m_canvas_p0.y + y),
                      IM_COL32(60, 60, 60, 120));

    ImGui::SetCursorScreenPos(m_canvas_p0);
    ImGui::SetNextItemAllowOverlap();
    const ImGuiButtonFlags bg_flags = ImGuiButtonFlags_MouseButtonLeft |
                                      ImGuiButtonFlags_MouseButtonRight |
                                      ImGuiButtonFlags_MouseButtonMiddle;
    ImGui::InvisibleButton("##bg", m_canvas_sz, bg_flags);
    m_background_hovered = ImGui::IsItemHovered();

    const bool bg_held = ImGui::IsItemActive();
    const ImVec2 delta = ImGui::GetIO().MouseDelta;

    // ---- zoom via Ctrl+scroll ------------------------------------------------
    if (m_background_hovered && ImGui::GetIO().KeyCtrl)
    {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.f)
        {
            ImVec2 mpos  = ImGui::GetMousePos();
            float old_z  = m_zoom;
            float new_z  = m_zoom * (1.f + wheel * 0.12f);
            if (new_z < 0.15f) new_z = 0.15f;
            if (new_z > 4.0f)  new_z = 4.0f;
            m_zoom = new_z;
            // Keep the point under the cursor fixed in canvas space.
            float rx = mpos.x - m_canvas_p0.x;
            float ry = mpos.y - m_canvas_p0.y;
            m_offset.x = rx - (rx - m_offset.x) * (new_z / old_z);
            m_offset.y = ry - (ry - m_offset.y) * (new_z / old_z);
        }
    }

    // ---- panning (right/middle mouse drag) -----------------------------------
    if (bg_held && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        m_right_down_pos = ImGui::GetMousePos();
        m_panning        = false;
    }
    if (bg_held &&
        (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 4.0f) ||
         ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 4.0f)))
        m_panning = true;
    if (m_panning)
    {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Right) &&
            !ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            m_panning = false;
        else
        {
            m_offset.x += delta.x;
            m_offset.y += delta.y;
        }
    }

    // ---- right-click on background -------------------------------------------
    if (m_background_hovered &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !m_panning)
    {
        ImVec2 release = ImGui::GetMousePos();
        float dx       = release.x - m_right_down_pos.x;
        float dy       = release.y - m_right_down_pos.y;
        if (dx * dx + dy * dy < 16.0f)
            m_bg_right_clicked = true;
    }

    // ---- LMB on background: fire bg_clicked on press (same frame as link
    //      clicks so !link_was_clicked guards work) and start rubber-band ------
    if (!is_mouse_in_minimap() && m_background_hovered &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        m_bg_clicked          = true;
        m_selected_node       = -1;
        m_lmb_bg_down         = true;
        m_lmb_bg_down_canvas  = mouse_canvas_pos();
    }

    if (m_lmb_bg_down && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.f))
        m_rubber_banding = true;

    if (m_rubber_banding)
    {
        ImVec2 cur = mouse_canvas_pos();
        float rxmin = m_lmb_bg_down_canvas.x < cur.x ? m_lmb_bg_down_canvas.x : cur.x;
        float rymin = m_lmb_bg_down_canvas.y < cur.y ? m_lmb_bg_down_canvas.y : cur.y;
        float rxmax = m_lmb_bg_down_canvas.x > cur.x ? m_lmb_bg_down_canvas.x : cur.x;
        float rymax = m_lmb_bg_down_canvas.y > cur.y ? m_lmb_bg_down_canvas.y : cur.y;
        m_rubber_band_rect = ImVec4(rxmin, rymin, rxmax, rymax);

        ImVec2 a_s = canvas_to_screen(rxmin, rymin);
        ImVec2 b_s = canvas_to_screen(rxmax, rymax);
        m_dl->AddRectFilled(a_s, b_s, IM_COL32(100, 160, 255, 25));
        m_dl->AddRect(a_s, b_s, IM_COL32(100, 160, 255, 180), 0.f, 0, 1.5f);
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        m_lmb_bg_down    = false;
        m_rubber_banding = false;
    }
}

void node_canvas::end()
{
    if (m_show_minimap)
        draw_minimap_overlay();

    m_dl->PopClipRect();
    // Draw border on top of all clipped content.
    ImVec2 p1(m_canvas_p0.x + m_canvas_sz.x, m_canvas_p0.y + m_canvas_sz.y);
    m_dl->AddRect(m_canvas_p0, p1, IM_COL32(80, 80, 80, 255));

    ImGui::EndChild();
}

node_canvas::node_result node_canvas::draw_node(int id,
                                                const char *label,
                                                ImVec2 size,
                                                ImVec2 &pos_inout,
                                                bool highlighted)
{
    node_style style;
    if (highlighted)
    {
        style.title_color = IM_COL32(70, 130, 200, 255);
        style.is_current  = true;
    }
    return draw_node(id, label, size, pos_inout, style);
}

node_canvas::node_result node_canvas::draw_node(int id,
                                                const char *label,
                                                ImVec2 size,
                                                ImVec2 &pos_inout,
                                                const node_style &style)
{
    node_result res;

    // Screen-space positions (applying zoom)
    ImVec2 tl = canvas_to_screen(pos_inout.x, pos_inout.y);
    ImVec2 sz_s(size.x * m_zoom, size.y * m_zoom);
    ImVec2 br(tl.x + sz_s.x, tl.y + sz_s.y);

    const float rounding = 6.0f * m_zoom;
    const float title_h  = 18.0f * m_zoom;
    const bool is_sel    = (m_selected_node == id);

    m_dl->AddRectFilled(ImVec2(tl.x + 2, tl.y + 4),
                        ImVec2(br.x + 2, br.y + 4),
                        IM_COL32(0, 0, 0, 80), rounding);
    m_dl->AddRectFilled(tl, br, style.body_color, rounding);
    ImVec2 tb_br(br.x, tl.y + title_h);
    m_dl->AddRectFilled(tl, tb_br, style.title_color, rounding,
                        ImDrawFlags_RoundCornersTop);
    m_dl->AddLine(ImVec2(tl.x + 1, tb_br.y), ImVec2(br.x - 1, tb_br.y),
                  IM_COL32(0, 0, 0, 100));

    ImU32 border  = style.border_color;
    float border_w = 1.5f;
    if (is_sel)                 { border = IM_COL32(255, 210, 0, 255);   border_w = 2.5f; }
    else if (style.is_current)  { border = IM_COL32(255, 255, 255, 220); border_w = 2.0f; }
    m_dl->AddRect(tl, br, border, rounding, 0, border_w);

    if (m_zoom >= 0.3f)
    {
        float font_sz = ImGui::GetFontSize() * m_zoom;
        if (font_sz < 7.f) font_sz = 7.f;

        ImVec2 ttl_sz  = ImGui::CalcTextSize(label);
        ImVec2 ttl_pos(tl.x + (sz_s.x - ttl_sz.x * m_zoom) * 0.5f,
                       tl.y + (title_h - ttl_sz.y * m_zoom) * 0.5f);
        m_dl->AddText(ImGui::GetFont(), font_sz, ttl_pos,
                      IM_COL32(250, 250, 250, 255), label);

        if (m_zoom >= 0.5f && style.subtitle && style.subtitle[0])
        {
            ImVec2 sub_sz  = ImGui::CalcTextSize(style.subtitle);
            ImVec2 sub_pos(tl.x + (sz_s.x - sub_sz.x * m_zoom) * 0.5f,
                           tb_br.y + (sz_s.y - title_h - sub_sz.y * m_zoom) * 0.5f);
            m_dl->AddText(ImGui::GetFont(), font_sz, sub_pos,
                          IM_COL32(190, 190, 200, 200), style.subtitle);
        }
    }

    ImGui::SetCursorScreenPos(tl);
    ImGui::PushID(id);
    const ImGuiButtonFlags n_flags =
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight;
    ImGui::InvisibleButton("##n", sz_s, n_flags);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        m_selected_node    = id;
        m_any_node_clicked = true;
        res.clicked        = true;
        res.ctrl_held      = ImGui::GetIO().KeyCtrl;
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        res.right_clicked = true;
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        ImVec2 d = ImGui::GetIO().MouseDelta;
        pos_inout.x += d.x / m_zoom;
        pos_inout.y += d.y / m_zoom;
        m_drag_delta   = ImVec2(d.x / m_zoom, d.y / m_zoom);
        m_drag_node_id = id;
    }
    ImGui::PopID();
    res.selected = is_sel;

    node_rec rec;
    rec.tl_screen     = tl;
    rec.size          = sz_s;
    rec.minimap_color = style.title_color;
    m_frame_nodes.add_or_set(id, rec);
    return res;
}

bool node_canvas::is_mouse_over_any_node() const
{
    ImVec2 m = ImGui::GetMousePos();
    for (const auto &pair : m_frame_nodes)
    {
        const node_rec &r = pair.value;
        if (m.x >= r.tl_screen.x && m.x < r.tl_screen.x + r.size.x &&
            m.y >= r.tl_screen.y && m.y < r.tl_screen.y + r.size.y)
            return true;
    }
    return false;
}

node_canvas::link_result node_canvas::draw_link(int src_id,
                                                 int dst_id,
                                                 ImU32 color,
                                                 float thickness,
                                                 ImVec2 bend)
{
    node_rec s, d;
    if (!m_frame_nodes.try_get_value(src_id, s)) return {};
    if (!m_frame_nodes.try_get_value(dst_id, d)) return {};

    ImVec2 sc(s.tl_screen.x + s.size.x * 0.5f,
              s.tl_screen.y + s.size.y * 0.5f);
    ImVec2 dc(d.tl_screen.x + d.size.x * 0.5f,
              d.tl_screen.y + d.size.y * 0.5f);

    ImVec2 mid((sc.x + dc.x) * 0.5f + bend.x, (sc.y + dc.y) * 0.5f + bend.y);
    ImVec2 p0 = node_border_pt(sc, s.tl_screen, s.size, mid);
    ImVec2 p3 = node_border_pt(dc, d.tl_screen, d.size, mid);

    ImVec2 dir0(p0.x - sc.x, p0.y - sc.y);
    ImVec2 dir3(p3.x - dc.x, p3.y - dc.y);
    {
        float l = sqrtf(dir0.x * dir0.x + dir0.y * dir0.y);
        if (l > 1e-3f) { dir0.x /= l; dir0.y /= l; }
        else              dir0 = {1.f, 0.f};
    }
    {
        float l = sqrtf(dir3.x * dir3.x + dir3.y * dir3.y);
        if (l > 1e-3f) { dir3.x /= l; dir3.y /= l; }
        else              dir3 = {-1.f, 0.f};
    }
    // off is in screen space; baseline (40) and cap (180) scale with zoom so
    // the curve shape stays constant across all zoom levels.
    float off = fminf((fabsf(dc.x - sc.x) + fabsf(dc.y - sc.y)) * 0.4f + 40.f * m_zoom,
                      180.f * m_zoom);
    ImVec2 p1(p0.x + dir0.x * off + bend.x, p0.y + dir0.y * off + bend.y);
    ImVec2 p2(p3.x + dir3.x * off + bend.x, p3.y + dir3.y * off + bend.y);
    ImVec2 midpoint((p0.x + 3.f * p1.x + 3.f * p2.x + p3.x) * 0.125f,
                    (p0.y + 3.f * p1.y + 3.f * p2.y + p3.y) * 0.125f);

    ImVec2 mouse = ImGui::GetMousePos();
    float dist   = point_bezier_dist(mouse, p0, p1, p2, p3);

    bool hovered = dist < 8.f && !is_mouse_over_any_node() && !is_mouse_in_minimap();
    bool clicked = hovered && !m_any_node_clicked &&
                   ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    ImU32 draw_color = color;
    float draw_thick = thickness * m_zoom;
    if (hovered)
    {
        ImVec4 cv = ImGui::ColorConvertU32ToFloat4(color);
        cv.w      = fminf(1.f, cv.w + 0.3f);
        draw_color = ImGui::ColorConvertFloat4ToU32(cv);
        draw_thick += 1.f;
    }

    auto draw_arrow = [&](ImVec2 tip, ImVec2 tan_dir)
    {
        float len = sqrtf(tan_dir.x * tan_dir.x + tan_dir.y * tan_dir.y);
        if (len < 0.001f) return;
        tan_dir.x /= len; tan_dir.y /= len;
        ImVec2 base(tip.x - tan_dir.x * 12.f * m_zoom, tip.y - tan_dir.y * 12.f * m_zoom);
        ImVec2 perp(-tan_dir.y * 6.f * m_zoom, tan_dir.x * 6.f * m_zoom);
        m_dl->AddTriangleFilled(tip,
                                ImVec2(base.x + perp.x, base.y + perp.y),
                                ImVec2(base.x - perp.x, base.y - perp.y),
                                draw_color);
    };

    m_dl->AddBezierCubic(p0, p1, p2, p3, draw_color, draw_thick, 32);
    draw_arrow(p3, ImVec2(p3.x - p2.x, p3.y - p2.y));

    link_result res;
    res.hovered  = hovered;
    res.clicked  = clicked;
    res.midpoint = midpoint;
    return res;
}

void node_canvas::draw_pending_link(int src_id,
                                    ImVec2 to_screen,
                                    ImU32 color,
                                    float thickness)
{
    node_rec s;
    if (!m_frame_nodes.try_get_value(src_id, s))
        return;
    ImVec2 sc(s.tl_screen.x + s.size.x * 0.5f,
              s.tl_screen.y + s.size.y * 0.5f);
    ImVec2 p0  = node_border_pt(sc, s.tl_screen, s.size, to_screen);
    ImVec2 p3  = to_screen;
    ImVec2 dir0(p0.x - sc.x, p0.y - sc.y);
    {
        float l = sqrtf(dir0.x * dir0.x + dir0.y * dir0.y);
        if (l > 1e-3f) { dir0.x /= l; dir0.y /= l; }
        else              dir0 = {1.f, 0.f};
    }
    float off = fminf((fabsf(to_screen.x - sc.x) + fabsf(to_screen.y - sc.y)) * 0.4f + 40.f * m_zoom,
                      180.f * m_zoom);
    ImVec2 dir3(sc.x - to_screen.x, sc.y - to_screen.y);
    {
        float l = sqrtf(dir3.x * dir3.x + dir3.y * dir3.y);
        if (l > 1e-3f) { dir3.x /= l; dir3.y /= l; }
        else              dir3 = {-1.f, 0.f};
    }
    ImVec2 p1(p0.x + dir0.x * off, p0.y + dir0.y * off);
    ImVec2 p2(p3.x + dir3.x * off, p3.y + dir3.y * off);
    m_dl->AddBezierCubic(p0, p1, p2, p3, color, thickness, 32);
}

ImVec2 node_canvas::mouse_canvas_pos() const
{
    ImVec2 m = ImGui::GetMousePos();
    return ImVec2((m.x - m_canvas_p0.x - m_offset.x) / m_zoom,
                  (m.y - m_canvas_p0.y - m_offset.y) / m_zoom);
}

bool node_canvas::is_mouse_in_minimap() const
{
    if (!m_show_minimap) return false;
    ImVec2 m = ImGui::GetMousePos();
    return m.x >= m_mm_rect.x && m.x <= m_mm_rect.z &&
           m.y >= m_mm_rect.y && m.y <= m_mm_rect.w;
}

void node_canvas::draw_minimap_overlay()
{
    const ImVec2 mm_tl(m_mm_rect.x, m_mm_rect.y);
    const ImVec2 mm_br(m_mm_rect.z, m_mm_rect.w);
    const float  mm_w = mm_br.x - mm_tl.x;
    const float  mm_h = mm_br.y - mm_tl.y;
    const float  inv  = 1.0f / m_zoom;

    // Viewport corners in canvas space (always computed, viewport is always known)
    const float vl = -m_offset.x * inv;
    const float vt = -m_offset.y * inv;
    const float vr = vl + m_canvas_sz.x * inv;
    const float vb = vt + m_canvas_sz.y * inv;

    // World = union(node bounding box, viewport) so the viewport indicator is
    // always fully visible inside the minimap and never overflows it.
    float wx0 = vl, wy0 = vt, wx1 = vr, wy1 = vb;
    for (const auto &pair : m_frame_nodes)
    {
        const node_rec &r = pair.value;
        float l  = (r.tl_screen.x - m_canvas_p0.x - m_offset.x) * inv;
        float t  = (r.tl_screen.y - m_canvas_p0.y - m_offset.y) * inv;
        float r2 = l + r.size.x * inv;
        float b  = t + r.size.y * inv;
        if (l  < wx0) wx0 = l;
        if (t  < wy0) wy0 = t;
        if (r2 > wx1) wx1 = r2;
        if (b  > wy1) wy1 = b;
    }

    // 10 % padding around the combined world
    const float epx = (wx1 - wx0) * 0.10f;
    const float epy = (wy1 - wy0) * 0.10f;
    wx0 -= epx; wx1 += epx;
    wy0 -= epy; wy1 += epy;

    const float ww = wx1 - wx0;
    const float wh = wy1 - wy0;
    if (ww < 1.f || wh < 1.f) return;

    // minimap pixels per canvas unit
    const float sx = mm_w / ww;
    const float sy = mm_h / wh;

    auto to_mm = [&](float cx, float cy) -> ImVec2 {
        return { mm_tl.x + (cx - wx0) * sx,
                 mm_tl.y + (cy - wy0) * sy };
    };

    // draw
    m_dl->AddRectFilled(mm_tl, mm_br, IM_COL32(20, 20, 25, 210), 4.f);
    m_dl->PushClipRect(mm_tl, mm_br, true);

    for (const auto &pair : m_frame_nodes)
    {
        const node_rec &r = pair.value;
        float l  = (r.tl_screen.x - m_canvas_p0.x - m_offset.x) * inv;
        float t  = (r.tl_screen.y - m_canvas_p0.y - m_offset.y) * inv;
        float r2 = l + r.size.x * inv;
        float b  = t + r.size.y * inv;
        ImVec2 a = to_mm(l, t);
        ImVec2 c = to_mm(r2, b);
        if (c.x < a.x + 2.f) c.x = a.x + 2.f;
        if (c.y < a.y + 2.f) c.y = a.y + 2.f;
        m_dl->AddRectFilled(a, c, r.minimap_color, 1.f);
    }

    // viewport indicator — always fits since world >= viewport
    ImVec2 vp_a = to_mm(vl, vt);
    ImVec2 vp_b = to_mm(vr, vb);
    m_dl->AddRectFilled(vp_a, vp_b, IM_COL32(255, 255, 255, 20));
    m_dl->AddRect      (vp_a, vp_b, IM_COL32(255, 255, 255, 180), 0.f, 0, 1.f);

    m_dl->PopClipRect();
    m_dl->AddRect(mm_tl, mm_br, IM_COL32(100, 100, 110, 220), 4.f, 0, 1.f);

    // navigation: click or drag to center viewport on that canvas point
    ImVec2 mouse = ImGui::GetMousePos();
    const bool in_mm = mouse.x >= mm_tl.x && mouse.x <= mm_br.x &&
                       mouse.y >= mm_tl.y && mouse.y <= mm_br.y;
    if (in_mm && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        float cx = wx0 + (mouse.x - mm_tl.x) / sx;
        float cy = wy0 + (mouse.y - mm_tl.y) / sy;
        m_offset.x = m_canvas_sz.x * 0.5f - cx * m_zoom;
        m_offset.y = m_canvas_sz.y * 0.5f - cy * m_zoom;
    }
}

} // namespace zabato::editor::widgets
