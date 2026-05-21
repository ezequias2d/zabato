#include <imgui.h>
#include <stdio.h>
#include <tinyxml2.h>

#include <editor/editor.hpp>
#include <editor/windows/animator_graph_window.hpp>

#include <zabato/animation.hpp>
#include <zabato/animator.hpp>
#include <zabato/animator_graph.hpp>
#include <zabato/animator_state.hpp>
#include <zabato/fs.hpp>
#include <zabato/node.hpp>
#include <zabato/object_resource.hpp>
#include <zabato/spatial.hpp>
#include <zabato/world.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato::editor
{

namespace
{
void copy_into_buf(char *buf, size_t cap, const char *src)
{
    if (cap == 0)
        return;
    size_t i = 0;
    if (src)
    {
        for (; src[i] && i + 1 < cap; ++i)
            buf[i] = src[i];
    }
    buf[i] = 0;
}

void send_select(editor_app &app, const uuid &id)
{
    game_message msg;
    msg.msg_id      = "cmd_select";
    msg.sender_id   = uuid();
    msg.receiver_id = id;
    app.send_message(msg);
}

void send_deselect(editor_app &app)
{
    game_message msg;
    msg.msg_id      = "cmd_deselect";
    msg.sender_id   = uuid();
    msg.receiver_id = uuid::null();
    // Use dispatch_to_windows (synchronous) rather than send_message (queued).
    // process_messages runs at the top of the NEXT frame's update(), so a
    // queued deselect would leave raw object* pointers in
    // scene_view::m_selection alive for an entire render pass after the object
    // is freed.
    app.dispatch_to_windows(msg);
}

// Draw a state name combo for `idx`. `g` may be nullptr (combo disabled).
void draw_state_combo(animator_graph *g, const char *label, size_t &idx)
{
    if (!g)
        return;
    vector<string> names;
    for (const auto &st : g->states())
    {
        if (!st)
            names.push_back(string("<null>"));
        else if (st->state_name().c_str() && st->state_name().c_str()[0])
            names.push_back(string(st->state_name().c_str()));
        else
            names.push_back(string("(unnamed)"));
    }
    int cur = (int)idx;
    if (cur < 0 || cur >= (int)names.size())
        cur = 0;
    const char *preview = names.empty() ? "" : names[cur].c_str();
    if (ImGui::BeginCombo(label, preview))
    {
        for (int i = 0; i < (int)names.size(); ++i)
        {
            bool sel = (i == cur);
            if (ImGui::Selectable(names[i].c_str(), sel))
                idx = (size_t)i;
            if (sel)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

// Returns true if any field changed.
bool draw_transition_editor(animator_graph *g, animator_transition &tr)
{
    bool changed = false;

    size_t idx = tr.dst_state_index;
    draw_state_combo(g, "dst state", idx);
    if (idx != tr.dst_state_index)
    {
        tr.dst_state_index = idx;
        changed            = true;
    }

    float dur = (float)tr.duration;
    if (ImGui::InputFloat("duration (s)", &dur))
    {
        tr.duration = (real)dur;
        changed     = true;
    }

    bool het = tr.has_exit_time;
    if (ImGui::Checkbox("has exit time", &het))
    {
        tr.has_exit_time = het;
        changed          = true;
    }
    if (het)
    {
        float etn = (float)tr.exit_time_norm;
        if (ImGui::InputFloat("exit time (0..1)", &etn))
        {
            tr.exit_time_norm = (real)etn;
            changed           = true;
        }
    }

    bool interruptible = tr.interruptible;
    if (ImGui::Checkbox("interruptible", &interruptible))
    {
        tr.interruptible = interruptible;
        changed          = true;
    }

    char msg_buf[64];
    copy_into_buf(&msg_buf[0], sizeof(msg_buf), tr.message_trigger.c_str());
    if (ImGui::InputText("message trigger", &msg_buf[0], sizeof(msg_buf)))
    {
        tr.message_trigger = symbol_ref(&msg_buf[0]);
        changed            = true;
    }

    return changed;
}
} // namespace

static animator *find_any_animator(spatial *s)
{
    if (!s)
        return nullptr;
    for (const auto &c : s->get_controllers())
    {
        if (c && c->is_derived(animator::TYPE))
            return c_dynamic_cast<animator>(c.get());
    }
    if (auto *n = c_dynamic_cast<node>(s))
    {
        for (int i = 0; i < n->quantity(); ++i)
        {
            if (auto *a = find_any_animator(n->child_at(i).get()))
                return a;
        }
    }
    return nullptr;
}

animator_transition *animator_graph_window::resolve_selected_transition()
{
    if (!m_graph.get() || m_sel_tr_src == LINK_NONE || m_sel_tr_idx < 0)
        return nullptr;

    if (m_sel_tr_src == ANY_STATE_NODE_ID)
    {
        auto &ax = m_graph->any_state_transitions();
        if (m_sel_tr_idx < (int)ax.size())
            return &ax[m_sel_tr_idx];
        return nullptr;
    }

    if (m_sel_tr_src >= 0 && m_sel_tr_src < (int)m_graph->states().size())
    {
        auto &st = m_graph->states()[m_sel_tr_src];
        if (st && m_sel_tr_idx < (int)st->transitions().size())
            return &st->transitions()[m_sel_tr_idx];
    }

    return nullptr;
}

// ---- multi-selection helpers ------------------------------------------------

bool animator_graph_window::is_multi_selected(int idx) const
{
    for (int i : m_multi_sel)
        if (i == idx)
            return true;
    return false;
}

void animator_graph_window::multi_sel_clear() { m_multi_sel.clear(); }

void animator_graph_window::multi_sel_toggle(int idx)
{
    for (int i = 0; i < (int)m_multi_sel.size(); ++i)
    {
        if (m_multi_sel[i] == idx)
        {
            m_multi_sel.remove_at((size_t)i);
            return;
        }
    }
    m_multi_sel.push_back(idx);
}

void animator_graph_window::multi_sel_add(int idx)
{
    if (!is_multi_selected(idx))
        m_multi_sel.push_back(idx);
}

void animator_graph_window::multi_sel_fix_after_delete(int deleted_idx)
{
    for (int i = (int)m_multi_sel.size() - 1; i >= 0; --i)
    {
        if (m_multi_sel[i] == deleted_idx)
            m_multi_sel.remove_at((size_t)i);
        else if (m_multi_sel[i] > deleted_idx)
            m_multi_sel[i]--;
    }
}

// ---- multi-selection operations ---------------------------------------------

void animator_graph_window::delete_multi_selection(editor_app &app)
{
    if (m_multi_sel.empty())
        return;
    // Sort descending so higher indices are deleted first (no index shifting).
    vector<int> sorted = m_multi_sel;
    for (int i = 1; i < (int)sorted.size(); ++i)
        for (int j = i; j > 0 && sorted[j] > sorted[j - 1]; --j)
        {
            int t         = sorted[j];
            sorted[j]     = sorted[j - 1];
            sorted[j - 1] = t;
        }
    multi_sel_clear();
    for (int idx : sorted)
        delete_state_at(app, idx);
}

void animator_graph_window::copy_multi_selection()
{
    if (m_multi_sel.empty() || !m_graph.get())
        return;
    auto &states            = m_graph->states();
    m_multi_clipboard.valid = false;
    m_multi_clipboard.entries.clear();

    ImVec2 origin = m_multi_sel[0] < (int)m_node_positions.size()
                        ? m_node_positions[m_multi_sel[0]]
                        : ImVec2(0.f, 0.f);

    for (int idx : m_multi_sel)
    {
        if (idx < 0 || idx >= (int)states.size() || !states[idx])
            continue;
        multi_clipboard_entry e;
        e.name        = string(states[idx]->state_name().c_str());
        e.transitions = states[idx]->transitions();
        e.rel_pos     = idx < (int)m_node_positions.size()
                            ? ImVec2(m_node_positions[idx].x - origin.x,
                                 m_node_positions[idx].y - origin.y)
                            : ImVec2(0.f, 0.f);
        if (auto *cs = c_dynamic_cast<animator_clip_state>(states[idx].get()))
        {
            e.clip       = cs->clip();
            e.loop       = cs->loop();
            e.start_tick = cs->start_tick();
            e.end_tick   = cs->end_tick();
        }
        m_multi_clipboard.entries.push_back(e);
    }
    m_multi_clipboard.valid = !m_multi_clipboard.entries.empty();
    // Also populate single-state clipboard for "Paste" button compatibility.
    if (m_multi_sel.size() == 1)
        copy_state(m_multi_sel[0]);
}

void animator_graph_window::paste_multi_selection(editor_app &app)
{
    if (!m_multi_clipboard.valid && !m_clipboard.valid)
        return;
    if (!m_graph.get())
        return;
    if (!m_multi_clipboard.valid)
    {
        paste_state(app);
        return;
    }
    ensure_positions();
    ensure_link_bends();
    ImVec2 base      = m_canvas.mouse_canvas_pos();
    size_t first_new = m_graph->states().size();
    multi_sel_clear();
    for (auto &e : m_multi_clipboard.entries)
    {
        auto *s = new animator_clip_state();
        s->set_state_name(symbol_ref(e.name.c_str()));
        s->set_clip(e.clip);
        s->set_loop(e.loop);
        s->set_start_tick(e.start_tick);
        s->set_end_tick(e.end_tick);
        // Remap intra-group transition destinations.
        auto trs = e.transitions;
        for (auto &tr : trs)
            tr.dst_state_index += first_new;
        s->transitions() = trs;
        m_graph->states().push_back(pointer<animator_state>(s));
        m_node_positions.push_back(
            ImVec2(base.x + e.rel_pos.x + 20.f, base.y + e.rel_pos.y + 20.f));
        multi_sel_add((int)(m_graph->states().size() - 1));
    }
    ensure_link_bends();
    m_dirty = true;
}

void animator_graph_window::cut_multi_selection(editor_app &app)
{
    copy_multi_selection();
    delete_multi_selection(app);
}

void animator_graph_window::duplicate_multi_selection(editor_app &app)
{
    copy_multi_selection();
    paste_multi_selection(app);
}

void animator_graph_window::select_all(editor_app &app)
{
    if (!m_graph.get())
        return;
    multi_sel_clear();
    for (size_t i = 0; i < m_graph->states().size(); ++i)
        if (m_graph->states()[i])
            multi_sel_add((int)i);
}

void animator_graph_window::focus_multi_selection()
{
    if (m_multi_sel.empty())
        return;
    float xmin = 1e9f, ymin = 1e9f, xmax = -1e9f, ymax = -1e9f;
    for (int idx : m_multi_sel)
    {
        if (idx < 0 || idx >= (int)m_node_positions.size())
            continue;
        float x = m_node_positions[idx].x, y = m_node_positions[idx].y;
        if (x < xmin)
            xmin = x;
        if (y < ymin)
            ymin = y;
        if (x + 140.f > xmax)
            xmax = x + 140.f;
        if (y + 42.f > ymax)
            ymax = y + 42.f;
    }
    m_canvas.set_offset(ImVec2(-((xmin + xmax) * 0.5f) + 200.f,
                               -((ymin + ymax) * 0.5f) + 150.f));
}

void animator_graph_window::align_selected_h()
{
    if ((int)m_multi_sel.size() < 2)
        return;
    float avg_y = 0.f;
    int count   = 0;
    for (int idx : m_multi_sel)
        if (idx < (int)m_node_positions.size())
        {
            avg_y += m_node_positions[idx].y;
            ++count;
        }
    if (!count)
        return;
    avg_y /= (float)count;
    for (int idx : m_multi_sel)
        if (idx < (int)m_node_positions.size())
            m_node_positions[idx].y = avg_y;
    m_dirty = true;
}

void animator_graph_window::align_selected_v()
{
    if ((int)m_multi_sel.size() < 2)
        return;
    float avg_x = 0.f;
    int count   = 0;
    for (int idx : m_multi_sel)
        if (idx < (int)m_node_positions.size())
        {
            avg_x += m_node_positions[idx].x;
            ++count;
        }
    if (!count)
        return;
    avg_x /= (float)count;
    for (int idx : m_multi_sel)
        if (idx < (int)m_node_positions.size())
            m_node_positions[idx].x = avg_x;
    m_dirty = true;
}

void animator_graph_window::distribute_selected_h()
{
    if ((int)m_multi_sel.size() < 3)
        return;
    vector<int> sorted = m_multi_sel;
    for (int i = 1; i < (int)sorted.size(); ++i)
        for (int j = i; j > 0 && m_node_positions[sorted[j]].x <
                                     m_node_positions[sorted[j - 1]].x;
             --j)
        {
            int t         = sorted[j];
            sorted[j]     = sorted[j - 1];
            sorted[j - 1] = t;
        }
    float x0   = m_node_positions[sorted.front()].x;
    float x1   = m_node_positions[sorted.back()].x;
    float step = (x1 - x0) / (float)((int)sorted.size() - 1);
    for (int k = 0; k < (int)sorted.size(); ++k)
        m_node_positions[sorted[k]].x = x0 + step * (float)k;
    m_dirty = true;
}

void animator_graph_window::distribute_selected_v()
{
    if ((int)m_multi_sel.size() < 3)
        return;
    vector<int> sorted = m_multi_sel;
    for (int i = 1; i < (int)sorted.size(); ++i)
        for (int j = i; j > 0 && m_node_positions[sorted[j]].y <
                                     m_node_positions[sorted[j - 1]].y;
             --j)
        {
            int t         = sorted[j];
            sorted[j]     = sorted[j - 1];
            sorted[j - 1] = t;
        }
    float y0   = m_node_positions[sorted.front()].y;
    float y1   = m_node_positions[sorted.back()].y;
    float step = (y1 - y0) / (float)((int)sorted.size() - 1);
    for (int k = 0; k < (int)sorted.size(); ++k)
        m_node_positions[sorted[k]].y = y0 + step * (float)k;
    m_dirty = true;
}

void animator_graph_window::on_message(const game_message &msg, editor_app &app)
{
    if (msg.msg_id == symbol_ref("cmd_select"))
    {
        object *obj = nullptr;
        if (!object::s_in_use.try_get_value(msg.receiver_id, obj) || !obj)
            return;

        if (auto *a = c_dynamic_cast<animator>(obj))
        {
            m_preview = a;
            return;
        }

        auto ctrl = obj->get_controller(animator::TYPE);
        if (ctrl)
        {
            m_preview = c_dynamic_cast<animator>(ctrl.get());
            return;
        }

        if (obj->is_derived(spatial::TYPE))
        {
            auto *s = static_cast<const spatial *>(obj);
            for (spatial *p = s->parent(); p; p = p->parent())
            {
                auto c = p->get_controller(animator::TYPE);
                if (c)
                {
                    m_preview = c_dynamic_cast<animator>(c.get());
                    return;
                }
            }
        }
    }
    else if (msg.msg_id == symbol_ref("cmd_deselect"))
    {
        if (msg.receiver_id == uuid::null())
        {
            m_preview = nullptr;
        }
        else if (m_preview && m_preview->get_object() &&
                 m_preview->get_object()->id() == msg.receiver_id)
        {
            m_preview = nullptr;
        }
    }
}

void animator_graph_window::open_empty(editor_app &app)
{
    m_graph = pointer<animator_graph>(new animator_graph());
    m_path.clear();
    m_open         = true;
    m_dirty        = false;
    m_linking_from = LINK_NONE;
    m_ctx_node_id  = LINK_NONE;
    m_sel_tr_src   = LINK_NONE;
    m_sel_tr_idx   = -1;
    m_node_positions.clear();
    m_link_bends.clear();
    m_any_bends.clear();
    m_canvas.clear_selection();
    multi_sel_clear();
}

void animator_graph_window::open(editor_app &app, const string &path)
{
    auto *fs = app.get_resource_manager()->get_file_system();
    if (!fs)
        return;

    xml_serializer xs;
    xs.set_manager(app.get_resource_manager());
    object *root = xs.load(*fs, path.c_str());
    auto *g      = root ? c_dynamic_cast<animator_graph>(root) : nullptr;
    if (!g)
    {
        open_empty(app);
        m_path = path;
        return;
    }
    m_graph        = pointer<animator_graph>(g);
    m_path         = path;
    m_open         = true;
    m_dirty        = false;
    m_linking_from = LINK_NONE;
    m_ctx_node_id  = LINK_NONE;
    m_sel_tr_src   = LINK_NONE;
    m_sel_tr_idx   = -1;
    m_node_positions.clear();
    m_link_bends.clear();
    m_any_bends.clear();
    m_canvas.clear_selection();
    multi_sel_clear();
}

void animator_graph_window::save_to_path(editor_app &app, const string &path)
{
    auto *fs = app.get_resource_manager()->get_file_system();
    if (!fs || !m_graph.get())
        return;
    xml_serializer xs;
    xs.set_manager(app.get_resource_manager());
    xs.save(*fs, path.c_str(), m_graph.get());
    m_path  = path;
    m_dirty = false;
}

void animator_graph_window::ensure_positions()
{
    if (!m_graph.get())
        return;
    auto &st = m_graph->states();
    while (m_node_positions.size() < st.size())
    {
        float x = 200.0f + 160.0f * (float)m_node_positions.size();
        m_node_positions.push_back(ImVec2(x, 60.0f));
    }
    while (m_node_positions.size() > st.size())
        m_node_positions.pop_back();
}

void animator_graph_window::ensure_link_bends()
{
    if (!m_graph.get())
        return;
    auto &states = m_graph->states();
    while (m_link_bends.size() < states.size())
        m_link_bends.push_back(vector<ImVec2>());
    while (m_link_bends.size() > states.size())
        m_link_bends.pop_back();
    for (size_t i = 0; i < states.size(); ++i)
    {
        if (!states[i])
            continue;
        auto &trs = states[i]->transitions();
        while (m_link_bends[i].size() < trs.size())
            m_link_bends[i].push_back(ImVec2(0.f, 0.f));
        while (m_link_bends[i].size() > trs.size())
            m_link_bends[i].pop_back();
    }
    auto &ax = m_graph->any_state_transitions();
    while (m_any_bends.size() < ax.size())
        m_any_bends.push_back(ImVec2(0.f, 0.f));
    while (m_any_bends.size() > ax.size())
        m_any_bends.pop_back();
}

void animator_graph_window::delete_state_at(editor_app &app, int idx)
{
    if (!m_graph.get() || idx < 0 || idx >= (int)m_graph->states().size())
        return;
    auto &states = m_graph->states();

    // Fix bend arrays before transition lists change.
    {
        auto &ax = m_graph->any_state_transitions();
        for (int k = (int)ax.size() - 1; k >= 0; --k)
            if ((int)ax[k].dst_state_index == idx &&
                (size_t)k < m_any_bends.size())
                m_any_bends.remove_at((size_t)k);
    }
    for (size_t bi = 0; bi < m_link_bends.size(); ++bi)
    {
        if (bi >= states.size() || !states[bi])
            continue;
        auto &trs = states[bi]->transitions();
        for (int k = (int)trs.size() - 1; k >= 0; --k)
            if ((int)trs[k].dst_state_index == idx &&
                (size_t)k < m_link_bends[bi].size())
                m_link_bends[bi].remove_at((size_t)k);
    }
    if ((size_t)idx < m_link_bends.size())
        m_link_bends.remove_at((size_t)idx);

    // Fix transition destination indices.
    auto fix = [&](vector<animator_transition> &list)
    {
        for (int k = (int)list.size() - 1; k >= 0; --k)
        {
            size_t d = list[k].dst_state_index;
            if ((int)d == idx)
                list.remove_at((size_t)k);
            else if ((int)d > idx)
                list[k].dst_state_index = d - 1;
        }
    };
    fix(m_graph->any_state_transitions());
    for (auto &other : states)
        if (other)
            fix(other->transitions());

    // Fix canvas node selection.
    int sel = m_canvas.selected_node();
    if (sel == idx)
        m_canvas.clear_selection();
    else if (sel > idx)
        m_canvas.set_selected_node(sel - 1);

    // Fix transition selection.
    if (m_sel_tr_src == idx)
    {
        m_sel_tr_src = LINK_NONE;
        m_sel_tr_idx = -1;
    }
    else if (m_sel_tr_src > idx)
        m_sel_tr_src--;

    // Also clear if the selected transition no longer exists (its dst was
    // deleted).
    if (m_sel_tr_src != LINK_NONE && !resolve_selected_transition())
    {
        m_sel_tr_src = LINK_NONE;
        m_sel_tr_idx = -1;
    }

    m_dirty = true;
    // Deselect before freeing the state so scene_view::m_selection raw pointers
    // are cleared while the object is still alive (dispatch_to_windows is
    // synchronous).
    send_deselect(app);

    states.remove_at((size_t)idx);
    if ((size_t)idx < m_node_positions.size())
        m_node_positions.remove_at((size_t)idx);

    // Fix entry index — reads states.size() post-removal, must stay last.
    if (!states.empty())
    {
        size_t e = m_graph->entry_state_index();
        if ((int)e >= (int)states.size())
            e = states.size() - 1;
        else if ((int)e > idx && e > 0)
            e -= 1;
        m_graph->set_entry_state_index(e);
    }

    multi_sel_fix_after_delete(idx);
}

void animator_graph_window::delete_selected_transition()
{
    if (!m_graph.get() || m_sel_tr_src == LINK_NONE || m_sel_tr_idx < 0)
        return;
    if (m_sel_tr_src == ANY_STATE_NODE_ID)
    {
        auto &ax = m_graph->any_state_transitions();
        if (m_sel_tr_idx < (int)ax.size())
        {
            ax.remove_at((size_t)m_sel_tr_idx);
            if (m_sel_tr_idx < (int)m_any_bends.size())
                m_any_bends.remove_at((size_t)m_sel_tr_idx);
        }
    }
    else if (m_sel_tr_src >= 0 &&
             m_sel_tr_src < (int)m_graph->states().size() &&
             m_graph->states()[m_sel_tr_src])
    {
        auto &trs = m_graph->states()[m_sel_tr_src]->transitions();
        if (m_sel_tr_idx < (int)trs.size())
        {
            trs.remove_at((size_t)m_sel_tr_idx);
            if (m_sel_tr_src < (int)m_link_bends.size() &&
                m_sel_tr_idx < (int)m_link_bends[m_sel_tr_src].size())
                m_link_bends[m_sel_tr_src].remove_at((size_t)m_sel_tr_idx);
        }
    }
    m_sel_tr_src = LINK_NONE;
    m_sel_tr_idx = -1;
    m_dirty      = true;
}

void animator_graph_window::copy_state(int idx)
{
    if (!m_graph.get() || idx < 0 || idx >= (int)m_graph->states().size())
        return;
    auto &st = m_graph->states()[idx];
    if (!st)
        return;

    m_clipboard.name        = string(st->state_name().c_str());
    m_clipboard.transitions = st->transitions();
    m_clipboard.src_pos     = (idx < (int)m_node_positions.size())
                                  ? m_node_positions[idx]
                                  : ImVec2(200.f, 60.f);
    if (auto *cs = c_dynamic_cast<animator_clip_state>(st.get()))
    {
        m_clipboard.clip       = cs->clip();
        m_clipboard.loop       = cs->loop();
        m_clipboard.start_tick = cs->start_tick();
        m_clipboard.end_tick   = cs->end_tick();
    }
    m_clipboard.valid = true;
}

void animator_graph_window::paste_state(editor_app &app)
{
    if (!m_clipboard.valid || !m_graph.get())
        return;
    auto *s = new animator_clip_state();
    s->set_state_name(symbol_ref(m_clipboard.name.c_str()));
    s->set_clip(m_clipboard.clip);
    s->set_loop(m_clipboard.loop);
    s->set_start_tick(m_clipboard.start_tick);
    s->set_end_tick(m_clipboard.end_tick);
    s->transitions() = m_clipboard.transitions;
    m_graph->states().push_back(pointer<animator_state>(s));
    ImVec2 pos(m_clipboard.src_pos.x + 20.f, m_clipboard.src_pos.y + 20.f);
    m_node_positions.push_back(pos);
    m_clipboard.src_pos = pos; // stagger subsequent pastes
    m_dirty             = true;
}

void animator_graph_window::duplicate_state(editor_app &app, int idx)
{
    if (!m_graph.get() || idx < 0 || idx >= (int)m_graph->states().size())
        return;
    clipboard_state saved = m_clipboard;
    copy_state(idx);
    paste_state(app);
    m_clipboard = saved; // don't pollute copy clipboard
    int new_idx = (int)m_graph->states().size() - 1;
    m_canvas.set_selected_node(new_idx);
    select_state(app, new_idx);
}

void animator_graph_window::focus_state(int idx)
{
    if (idx < 0 || idx >= (int)m_node_positions.size())
        return;
    ImVec2 pos = m_node_positions[idx];
    m_canvas.set_offset(ImVec2(-pos.x + 200.f, -pos.y + 150.f));
}

void animator_graph_window::handle_shortcuts(editor_app &app)
{
    if (!m_graph.get())
        return;
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
        return;
    if (ImGui::GetIO().WantTextInput)
        return;

    // Sync auxiliary arrays before any modification so remove_at indices
    // are always in bounds (ensure_* are normally called in render_canvas_panel
    // which runs after this function).
    ensure_positions();
    ensure_link_bends();

    const bool ctrl      = ImGui::GetIO().KeyCtrl;
    const int sel        = m_canvas.selected_node();
    const bool has_multi = (int)m_multi_sel.size() > 1;
    const bool has_st    = m_multi_sel.size() == 1 && sel >= 0 &&
                        sel < (int)m_graph->states().size() &&
                        m_graph->states()[sel];
    const bool has_tr = m_sel_tr_src != LINK_NONE && m_sel_tr_idx >= 0;

    if (ImGui::IsKeyPressed(ImGuiKey_Delete) ||
        ImGui::IsKeyPressed(ImGuiKey_Backspace))
    {
        if (has_tr)
            delete_selected_transition();
        else if (has_multi)
            delete_multi_selection(app);
        else if (has_st)
            delete_state_at(app, sel);
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_A))
        select_all(app);
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C) && !m_multi_sel.empty())
        copy_multi_selection();
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_X) && !m_multi_sel.empty())
        cut_multi_selection(app);
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V))
        paste_multi_selection(app);
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_D) && !m_multi_sel.empty())
        duplicate_multi_selection(app);
    if (ImGui::IsKeyPressed(ImGuiKey_F) && !m_multi_sel.empty())
        focus_multi_selection();
}

void animator_graph_window::select_state(editor_app &app, int state_index)
{
    if (!m_graph.get())
        return;
    if (state_index < 0 || state_index >= (int)m_graph->states().size())
        return;
    auto &st = m_graph->states()[state_index];
    if (st)
        send_select(app, st->id());
}

void animator_graph_window::select_any_state(editor_app &app)
{
    if (!m_graph.get())
        return;
    send_select(app, m_graph->id());
}

void animator_graph_window::render_toolbar(editor_app &app)
{
    if (ImGui::Button("New"))
        open_empty(app);
    ImGui::SameLine();
    if (ImGui::Button("Save") && !m_path.empty())
        save_to_path(app, m_path);
    ImGui::SameLine();
    ImGui::Text("%s%s",
                m_path.empty() ? "<unsaved>" : m_path.c_str(),
                m_dirty ? " *" : "");

    ImGui::SameLine();
    ImGui::Text("  |");
    ImGui::SameLine();
    if (m_preview)
    {
        const char *owner_name = "<anonymous>";
        if (m_preview->get_object())
        {
            const char *n = m_preview->get_object()->name();
            if (n && n[0])
                owner_name = n;
        }
        ImGui::TextColored(
            ImVec4(0.5f, 0.95f, 0.6f, 1), "Preview: %s", owner_name);
        ImGui::SameLine();
        if (ImGui::SmallButton(m_preview_playing ? "pause" : "play"))
            m_preview_playing = !m_preview_playing;
        ImGui::SameLine();
        if (ImGui::SmallButton("rewind") && m_graph.get())
        {
            spatial *root = nullptr;
            if (m_preview->get_object() &&
                m_preview->get_object()->is_derived(spatial::TYPE))
                root = static_cast<spatial *>(m_preview->get_object());
            m_preview->set_graph_direct(m_graph, root);
            m_preview_bound_graph = m_graph.get();
            m_preview_bound_skel  = root;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("clear"))
            m_preview = nullptr;

        const size_t bone_count = m_preview->bound_bones().size();
        const char *state_name  = "<none>";
        bool has_clip           = false;
        animation *clip_anim    = nullptr;
        if (m_preview->graph() && m_preview->current_state_index() <
                                      m_preview->graph()->states().size())
        {
            auto st =
                m_preview->graph()->states()[m_preview->current_state_index()];
            if (st)
            {
                const char *n = st->state_name().c_str();
                if (n && n[0])
                    state_name = n;
                if (auto *cs = c_dynamic_cast<animator_clip_state>(st.get()))
                {
                    has_clip = !cs->clip().path().empty();
                    if (has_clip)
                    {
                        auto ap   = cs->clip().get<animation>();
                        clip_anim = ap.get();
                    }
                }
                else
                {
                    has_clip = true;
                }
            }
        }

        size_t channels = 0;
        size_t matched  = 0;
        if (clip_anim)
        {
            channels = clip_anim->get_bones().size();
            for (const auto &ch : clip_anim->get_bones())
            {
                const char *cn = ch.track.bone_name.c_str();
                for (const auto &bb : m_preview->bound_bones())
                {
                    if (bb.name == cn)
                    {
                        ++matched;
                        break;
                    }
                }
            }
        }

        const char *skel_hint = "skeleton: auto(owner)";
        ImGui::TextDisabled("  state=%s  bones=%zu  %s%s",
                            state_name,
                            bone_count,
                            skel_hint,
                            has_clip ? "" : "  [!] NO clip");
        if (clip_anim)
        {
            ImVec4 col = (matched > 0) ? ImVec4(0.6f, 0.8f, 0.6f, 1.0f)
                                       : ImVec4(1.0f, 0.6f, 0.4f, 1.0f);
            ImGui::TextColored(
                col,
                "  clip channels=%zu  matched bones=%zu%s",
                channels,
                matched,
                matched == 0 ? "  [!] NAME MISMATCH (rig differs)" : "");
            if (ImGui::IsItemHovered() && channels > 0)
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Unmatched animation channels:");
                size_t shown = 0;
                for (const auto &ch : clip_anim->get_bones())
                {
                    const char *cn = ch.track.bone_name.c_str();
                    bool hit       = false;
                    for (const auto &bb : m_preview->bound_bones())
                    {
                        if (bb.name == cn)
                        {
                            hit = true;
                            break;
                        }
                    }
                    if (!hit)
                    {
                        ImGui::BulletText("%s", cn);
                        if (++shown >= 20)
                        {
                            ImGui::Text("  ... (%zu more)", channels - shown);
                            break;
                        }
                    }
                }
                ImGui::EndTooltip();
            }
        }
    }
    else
    {
        ImGui::TextDisabled("Preview: none");
        ImGui::SameLine();
        if (ImGui::SmallButton("bind first in scene"))
        {
            if (auto w = app.get_world())
            {
                if (auto root = w->get_scene_root())
                    m_preview = find_any_animator(root.get());
            }
        }
    }

    if (m_linking_from != LINK_NONE)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0.8f, 0.3f, 1),
                           "linking... click target (Esc to cancel)");
    }
}

void animator_graph_window::render_left_panel(editor_app &app)
{
    if (!m_graph.get())
        return;

    // If a transition is selected, show its inline editor.
    animator_transition *sel_tr = resolve_selected_transition();
    if (sel_tr)
    {
        ImGui::TextDisabled("Transition");
        ImGui::Separator();

        auto &states = m_graph->states();
        if (m_sel_tr_src == ANY_STATE_NODE_ID)
        {
            ImGui::TextDisabled("from: Any State");
        }
        else if (m_sel_tr_src >= 0 && m_sel_tr_src < (int)states.size() &&
                 states[m_sel_tr_src])
        {
            const char *sn = states[m_sel_tr_src]->state_name().c_str();
            ImGui::TextDisabled("from: %s", (sn && sn[0]) ? sn : "(unnamed)");
        }

        if (draw_transition_editor(m_graph.get(), *sel_tr))
            m_dirty = true;

        ImGui::Separator();
        if (ImGui::Button("Remove transition"))
            delete_selected_transition();
        return;
    }

    // Default view: list all unique message triggers used in this graph.
    ImGui::TextDisabled("Message triggers");
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 26.0f);
        ImGui::TextUnformatted(
            "All message triggers used in transitions.\n\n"
            "Scripts call animator:send_message(name) (or the object's "
            "message dispatch) to fire a trigger. The 'send' button "
            "injects one immediately for live preview.");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    ImGui::Separator();

    vector<symbol_ref> msgs;
    auto collect = [&](const vector<animator_transition> &trs)
    {
        for (const auto &t : trs)
        {
            if (t.message_trigger.empty())
                continue;
            bool found = false;
            for (const auto &m : msgs)
                if (m == t.message_trigger)
                {
                    found = true;
                    break;
                }
            if (!found)
                msgs.push_back(t.message_trigger);
        }
    };
    collect(m_graph->any_state_transitions());
    for (const auto &st : m_graph->states())
        if (st)
            collect(st->transitions());

    if (msgs.empty())
    {
        ImGui::TextDisabled("(none)");
        ImGui::TextDisabled("Set message_trigger on a transition.");
    }
    else
    {
        for (const auto &m : msgs)
        {
            ImGui::BulletText("%s", m.c_str());
            if (m_preview)
            {
                ImGui::SameLine();
                ImGui::PushID(m.c_str());
                if (ImGui::SmallButton("send"))
                {
                    game_message fire_msg;
                    fire_msg.msg_id    = m;
                    fire_msg.sender_id = uuid();
                    m_preview->on_message(fire_msg);
                }
                ImGui::PopID();
            }
        }
    }
}

void animator_graph_window::render_canvas_panel(editor_app &app)
{
    if (!m_graph.get())
        return;
    ensure_positions();
    ensure_link_bends();
    auto &states = m_graph->states();

    ImGui::Text("States");
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
        ImGui::TextUnformatted(
            "Left-click node       - select state\n"
            "Ctrl+click node       - add/remove from selection\n"
            "Left-drag background  - rubber-band select\n"
            "Left-click link       - select transition\n"
            "Left-drag node        - move state(s)\n"
            "Right-click node      - state context menu\n"
            "Right-click bg        - add state menu\n"
            "Right/Middle-drag     - pan the canvas\n"
            "Ctrl+scroll           - zoom in / out\n"
            "Esc                   - cancel link gesture\n"
            "\n"
            "Keyboard shortcuts:\n"
            "Del / Backspace    - delete selection\n"
            "Ctrl+A             - select all\n"
            "Ctrl+C / X         - copy / cut\n"
            "Ctrl+V             - paste\n"
            "Ctrl+D             - duplicate\n"
            "F                  - focus selection\n"
            "\n"
            "Multi-selection toolbar:\n"
            "Align Row / Col    - align nodes\n"
            "Space H / Space V  - distribute evenly\n"
            "\n"
            "Bend handle (on selected link):\n"
            "Drag dot           - reshape curve\n"
            "Middle-click dot   - reset bend");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    // ---- Context-sensitive action bar ----
    {
        const int primary_sel = m_canvas.selected_node();
        const bool has_multi  = (int)m_multi_sel.size() > 1;
        const bool has_st     = m_multi_sel.size() == 1 && primary_sel >= 0 &&
                            primary_sel < (int)states.size() &&
                            states[primary_sel];
        const bool has_tr = m_sel_tr_src != LINK_NONE && m_sel_tr_idx >= 0;

        if (has_multi)
        {
            ImGui::TextDisabled("%d selected", (int)m_multi_sel.size());
            ImGui::SameLine();
            if (ImGui::SmallButton("Copy"))
                copy_multi_selection();
            ImGui::SameLine();
            if (ImGui::SmallButton("Cut"))
                cut_multi_selection(app);
            ImGui::SameLine();
            if (ImGui::SmallButton("Duplicate"))
                duplicate_multi_selection(app);
            ImGui::SameLine();
            if (ImGui::SmallButton("Delete"))
                delete_multi_selection(app);
            ImGui::SameLine();
            if (ImGui::SmallButton("Focus"))
                focus_multi_selection();
            ImGui::SameLine();
            if (ImGui::SmallButton("Align Row"))
                align_selected_h();
            ImGui::SameLine();
            if (ImGui::SmallButton("Align Col"))
                align_selected_v();
            ImGui::SameLine();
            if (ImGui::SmallButton("Space H"))
                distribute_selected_h();
            ImGui::SameLine();
            if (ImGui::SmallButton("Space V"))
                distribute_selected_v();
        }
        else if (has_st)
        {
            if (ImGui::SmallButton("Copy"))
                copy_multi_selection();
            ImGui::SameLine();
            if (ImGui::SmallButton("Cut"))
                cut_multi_selection(app);
            ImGui::SameLine();
            if (ImGui::SmallButton("Duplicate"))
                duplicate_state(app, primary_sel);
            ImGui::SameLine();
            if (ImGui::SmallButton("Delete"))
                delete_state_at(app, primary_sel);
            ImGui::SameLine();
            if (ImGui::SmallButton("Focus"))
                focus_state(primary_sel);
            ImGui::SameLine();
            if (ImGui::SmallButton("Link From"))
                m_linking_from = primary_sel;
        }
        else if (has_tr)
        {
            if (ImGui::SmallButton("Delete Transition"))
                delete_selected_transition();
            ImGui::SameLine();
            ImVec2 *bend_ptr = nullptr;
            if (m_sel_tr_src == ANY_STATE_NODE_ID &&
                m_sel_tr_idx < (int)m_any_bends.size())
                bend_ptr = &m_any_bends[m_sel_tr_idx];
            else if (m_sel_tr_src >= 0 &&
                     m_sel_tr_src < (int)m_link_bends.size() &&
                     m_sel_tr_idx < (int)m_link_bends[m_sel_tr_src].size())
                bend_ptr = &m_link_bends[m_sel_tr_src][m_sel_tr_idx];
            const bool has_bend =
                bend_ptr && (bend_ptr->x != 0.f || bend_ptr->y != 0.f);
            if (!has_bend)
                ImGui::BeginDisabled();
            if (ImGui::SmallButton("Reset Bend") && bend_ptr)
                *bend_ptr = ImVec2(0.f, 0.f);
            if (!has_bend)
                ImGui::EndDisabled();
        }
        else
        {
            ImGui::TextDisabled("Select a node or link  |");
            if (m_clipboard.valid || m_multi_clipboard.valid)
            {
                ImGui::SameLine();
                if (ImGui::SmallButton("Paste"))
                    paste_multi_selection(app);
            }
        }
    }
    ImGui::Separator();

    m_canvas.begin("graph_canvas");

    const ImVec2 node_sz(140, 42);
    ImVec2 menu_canvas_pos = m_canvas.mouse_canvas_pos();

    // ---- Any-State pseudo-node ----
    {
        widgets::node_canvas::node_style any_style;
        any_style.subtitle     = "from any";
        any_style.title_color  = IM_COL32(50, 170, 130, 255);
        any_style.body_color   = IM_COL32(36, 52, 46, 255);
        any_style.border_color = IM_COL32(70, 200, 155, 255);

        const char *label = "Any State";
        auto res          = m_canvas.draw_node(
            ANY_STATE_NODE_ID, label, node_sz, m_any_state_pos, any_style);
        if (res.clicked)
        {
            if (m_linking_from != LINK_NONE &&
                m_linking_from != ANY_STATE_NODE_ID)
            {
                m_linking_from = LINK_NONE;
            }
            else
            {
                m_sel_tr_src = LINK_NONE;
                m_sel_tr_idx = -1;
                select_any_state(app);
            }
        }
        if (res.right_clicked)
        {
            m_ctx_node_id = ANY_STATE_NODE_ID;
            ImGui::OpenPopup("agw_node_ctx");
        }
    }

    // ---- Regular state nodes ----
    for (size_t i = 0; i < states.size(); ++i)
    {
        auto &st = states[i];
        if (!st)
            continue;
        const char *sname = st->state_name().c_str();
        bool live     = (m_preview && m_preview->current_state_index() == i);
        bool is_entry = ((int)m_graph->entry_state_index() == (int)i);

        widgets::node_canvas::node_style st_style;
        st_style.subtitle = "clip";
        if (is_entry)
        {
            st_style.title_color  = IM_COL32(232, 135, 10, 255);
            st_style.body_color   = IM_COL32(55, 40, 22, 255);
            st_style.border_color = IM_COL32(255, 170, 60, 255);
        }
        else
        {
            st_style.title_color  = IM_COL32(78, 110, 150, 255);
            st_style.body_color   = IM_COL32(40, 44, 52, 255);
            st_style.border_color = IM_COL32(110, 130, 160, 255);
        }
        st_style.is_current = live;
        // Yellow border for multi-selected nodes (overrides entry color too).
        if (is_multi_selected((int)i))
            st_style.border_color = IM_COL32(255, 240, 80, 255);

        const char *display = (sname && sname[0]) ? sname : "(unnamed)";
        auto res            = m_canvas.draw_node(
            (int)i, display, node_sz, m_node_positions[i], st_style);
        if (res.clicked)
        {
            if (m_linking_from != LINK_NONE)
            {
                // Complete the link: linking_from -> this state.
                animator_transition tr;
                tr.dst_state_index = i;
                tr.duration        = real(0.25);
                if (m_linking_from == ANY_STATE_NODE_ID)
                {
                    m_graph->any_state_transitions().push_back(tr);
                    m_sel_tr_src = ANY_STATE_NODE_ID;
                    m_sel_tr_idx =
                        (int)m_graph->any_state_transitions().size() - 1;
                }
                else if (m_linking_from >= 0 &&
                         m_linking_from < (int)states.size() &&
                         states[m_linking_from])
                {
                    states[m_linking_from]->transitions().push_back(tr);
                    m_sel_tr_src = m_linking_from;
                    m_sel_tr_idx =
                        (int)states[m_linking_from]->transitions().size() - 1;
                }
                m_dirty        = true;
                m_linking_from = LINK_NONE;
            }
            else if (res.ctrl_held)
            {
                // Ctrl+click: toggle this node in the multi-selection.
                multi_sel_toggle((int)i);
            }
            else
            {
                // Normal click: single-select.
                multi_sel_clear();
                multi_sel_add((int)i);
                m_sel_tr_src = LINK_NONE;
                m_sel_tr_idx = -1;
                select_state(app, (int)i);
            }
        }
        if (res.right_clicked)
        {
            // If right-clicking a node NOT in the multi-selection, replace
            // selection with just this node so context ops act on it.
            if (!is_multi_selected((int)i))
            {
                multi_sel_clear();
                multi_sel_add((int)i);
            }
            m_ctx_node_id = (int)i;
            ImGui::OpenPopup("agw_node_ctx");
        }
    }

    // ---- Rubber-band hit test
    // ------------------------------------------------
    if (m_canvas.is_rubber_banding())
    {
        ImVec4 rr = m_canvas.rubber_band_rect_canvas();
        if (!ImGui::GetIO().KeyCtrl)
            multi_sel_clear();
        for (size_t i = 0; i < states.size(); ++i)
        {
            if (!states[i] || i >= m_node_positions.size())
                continue;
            ImVec2 &p = m_node_positions[i];
            if (p.x + node_sz.x >= rr.x && p.x <= rr.z &&
                p.y + node_sz.y >= rr.y && p.y <= rr.w)
                multi_sel_add((int)i);
        }
    }

    // ---- Multi-drag: propagate drag delta to other selected nodes
    // ------------
    {
        int dragged = m_canvas.drag_node_id();
        if (dragged >= 0 && (int)m_multi_sel.size() > 1)
        {
            ImVec2 d = m_canvas.drag_delta();
            for (int sel : m_multi_sel)
            {
                if (sel == dragged)
                    continue;
                if (sel >= 0 && sel < (int)m_node_positions.size())
                {
                    m_node_positions[sel].x += d.x;
                    m_node_positions[sel].y += d.y;
                }
            }
        }
    }

    // ---- Links: regular transitions ----
    // Bends are stored in canvas space; scale to screen space for drawing.
    const float z            = m_canvas.zoom();
    bool link_was_clicked    = false;
    ImVec2 sel_link_midpoint = {};
    bool sel_link_found      = false;
    for (size_t i = 0; i < states.size(); ++i)
    {
        auto &st = states[i];
        if (!st)
            continue;
        auto &trs = st->transitions();
        for (size_t j = 0; j < trs.size(); ++j)
        {
            size_t dst = trs[j].dst_state_index;
            if (dst >= states.size())
                continue;
            bool is_sel = (m_sel_tr_src == (int)i && m_sel_tr_idx == (int)j);
            ImU32 color = is_sel ? IM_COL32(255, 220, 60, 255)
                                 : IM_COL32(120, 200, 255, 220);
            float thick = is_sel ? 2.5f : 2.0f;
            ImVec2 bend_cs =
                (i < m_link_bends.size() && j < m_link_bends[i].size())
                    ? m_link_bends[i][j]
                    : ImVec2(0.f, 0.f);
            ImVec2 bend(bend_cs.x * z, bend_cs.y * z);
            auto lr = m_canvas.draw_link((int)i, (int)dst, color, thick, bend);
            if (is_sel)
            {
                sel_link_midpoint = lr.midpoint;
                sel_link_found    = true;
            }
            if (lr.clicked && m_linking_from == LINK_NONE)
            {
                m_sel_tr_src     = (int)i;
                m_sel_tr_idx     = (int)j;
                link_was_clicked = true;
            }
        }
    }

    // ---- Links: any-state transitions ----
    auto &ax = m_graph->any_state_transitions();
    for (size_t j = 0; j < ax.size(); ++j)
    {
        size_t dst = ax[j].dst_state_index;
        if (dst >= states.size())
            continue;
        bool is_sel =
            (m_sel_tr_src == ANY_STATE_NODE_ID && m_sel_tr_idx == (int)j);
        ImU32 color =
            is_sel ? IM_COL32(255, 220, 60, 255) : IM_COL32(255, 160, 80, 220);
        float thick = is_sel ? 2.5f : 2.0f;
        ImVec2 bend_cs =
            (j < m_any_bends.size()) ? m_any_bends[j] : ImVec2(0.f, 0.f);
        ImVec2 bend(bend_cs.x * z, bend_cs.y * z);
        auto lr =
            m_canvas.draw_link(ANY_STATE_NODE_ID, (int)dst, color, thick, bend);
        if (is_sel)
        {
            sel_link_midpoint = lr.midpoint;
            sel_link_found    = true;
        }
        if (lr.clicked && m_linking_from == LINK_NONE)
        {
            m_sel_tr_src     = ANY_STATE_NODE_ID;
            m_sel_tr_idx     = (int)j;
            link_was_clicked = true;
        }
    }

    // ---- Bend handle for the selected link ----------------------------------
    m_bend_handle_active = false;
    if (sel_link_found && m_sel_tr_src != LINK_NONE && m_sel_tr_idx >= 0)
    {
        // Resolve pointer to the active bend entry.
        ImVec2 *bend_ptr = nullptr;
        if (m_sel_tr_src == ANY_STATE_NODE_ID &&
            m_sel_tr_idx < (int)m_any_bends.size())
            bend_ptr = &m_any_bends[m_sel_tr_idx];
        else if (m_sel_tr_src >= 0 && m_sel_tr_src < (int)m_link_bends.size() &&
                 m_sel_tr_idx < (int)m_link_bends[m_sel_tr_src].size())
            bend_ptr = &m_link_bends[m_sel_tr_src][m_sel_tr_idx];

        const bool has_bend =
            bend_ptr && (bend_ptr->x != 0.f || bend_ptr->y != 0.f);

        // Hit area: larger when bent (diamond), smaller when straight (circle).
        const float hs = has_bend ? 8.f : 5.f;
        ImVec2 mp      = sel_link_midpoint;

        ImGui::SetNextItemAllowOverlap();
        ImGui::SetCursorScreenPos(ImVec2(mp.x - hs, mp.y - hs));
        ImGui::PushID("##bh");
        ImGui::InvisibleButton("bh", ImVec2(hs * 2.f, hs * 2.f));

        const bool hovered   = ImGui::IsItemHovered();
        const bool active    = ImGui::IsItemActive();
        m_bend_handle_active = active || hovered;

        const ImU32 fill_col =
            hovered ? IM_COL32(255, 245, 120, 240)
                    : IM_COL32(255, 220, 60, has_bend ? 200 : 140);
        const ImU32 border_col = IM_COL32(255, 255, 255, hovered ? 220 : 130);

        ImDrawList *dl = ImGui::GetWindowDrawList();
        if (has_bend)
        {
            dl->AddQuadFilled(ImVec2(mp.x - hs, mp.y),
                              ImVec2(mp.x, mp.y - hs),
                              ImVec2(mp.x + hs, mp.y),
                              ImVec2(mp.x, mp.y + hs),
                              fill_col);
            dl->AddQuad(ImVec2(mp.x - hs, mp.y),
                        ImVec2(mp.x, mp.y - hs),
                        ImVec2(mp.x + hs, mp.y),
                        ImVec2(mp.x, mp.y + hs),
                        border_col,
                        1.5f);
        }
        else
        {
            dl->AddCircleFilled(mp, hs, fill_col);
            dl->AddCircle(mp, hs, border_col, 0, 1.5f);
        }

        // Middle-click resets the bend.
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle) &&
            bend_ptr)
            *bend_ptr = ImVec2(0.f, 0.f);

        // Left-drag reshapes the curve (accumulate in canvas space so bends
        // look consistent at all zoom levels).
        if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && bend_ptr)
        {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            float z      = m_canvas.zoom();
            bend_ptr->x += delta.x / z;
            bend_ptr->y += delta.y / z;
        }

        if (hovered)
            ImGui::SetTooltip(has_bend
                                  ? "Drag to reshape  |  Middle-click to reset"
                                  : "Drag to reshape path");

        ImGui::PopID();
    }

    // ---- Pending link while in linking mode ----
    if (m_linking_from != LINK_NONE)
        m_canvas.draw_pending_link(m_linking_from, ImGui::GetMousePos());

    // ---- Node context menu ----
    if (ImGui::BeginPopup("agw_node_ctx"))
    {
        if (m_ctx_node_id == ANY_STATE_NODE_ID)
        {
            if (ImGui::MenuItem("Make Transition from Any"))
                m_linking_from = ANY_STATE_NODE_ID;
            ImGui::Separator();
            // List any-state transitions for selection.
            auto &alist = m_graph->any_state_transitions();
            for (int ti = 0; ti < (int)alist.size(); ++ti)
            {
                const auto &tr    = alist[ti];
                const char *dname = "?";
                if (tr.dst_state_index < states.size() &&
                    states[tr.dst_state_index])
                {
                    const char *n =
                        states[tr.dst_state_index]->state_name().c_str();
                    if (n && n[0])
                        dname = n;
                }
                char item_lbl[96];
                snprintf(
                    item_lbl, sizeof(item_lbl), "Edit tr -> %s##%d", dname, ti);
                if (ImGui::MenuItem(item_lbl))
                {
                    m_sel_tr_src = ANY_STATE_NODE_ID;
                    m_sel_tr_idx = ti;
                }
            }
        }
        else if (m_ctx_node_id >= 0 && m_ctx_node_id < (int)states.size())
        {
            if ((int)m_multi_sel.size() > 1)
            {
                // Multi-selection context menu
                char lbl[64];
                snprintf(lbl,
                         sizeof(lbl),
                         "Delete %d states",
                         (int)m_multi_sel.size());
                if (ImGui::MenuItem(lbl))
                    delete_multi_selection(app);
                snprintf(lbl,
                         sizeof(lbl),
                         "Copy %d states",
                         (int)m_multi_sel.size());
                if (ImGui::MenuItem(lbl))
                    copy_multi_selection();
                snprintf(lbl,
                         sizeof(lbl),
                         "Duplicate %d states",
                         (int)m_multi_sel.size());
                if (ImGui::MenuItem(lbl))
                    duplicate_multi_selection(app);
                ImGui::Separator();
                if (ImGui::MenuItem("Align Row"))
                    align_selected_h();
                if (ImGui::MenuItem("Align Column"))
                    align_selected_v();
                if (ImGui::MenuItem("Space Horizontally"))
                    distribute_selected_h();
                if (ImGui::MenuItem("Space Vertically"))
                    distribute_selected_v();
            }
            else
            {
                // Single-node context menu
                if (ImGui::MenuItem("Make Transition"))
                    m_linking_from = m_ctx_node_id;
                if (ImGui::MenuItem("Set as entry"))
                {
                    m_graph->set_entry_state_index((size_t)m_ctx_node_id);
                    m_dirty = true;
                }
                if (ImGui::MenuItem("Inspect"))
                    select_state(app, m_ctx_node_id);
                ImGui::Separator();
                // List outgoing transitions for selection.
                auto &src = states[m_ctx_node_id];
                if (src && !src->transitions().empty())
                {
                    for (int ti = 0; ti < (int)src->transitions().size(); ++ti)
                    {
                        const auto &tr    = src->transitions()[ti];
                        const char *dname = "?";
                        if (tr.dst_state_index < states.size() &&
                            states[tr.dst_state_index])
                        {
                            const char *n = states[tr.dst_state_index]
                                                ->state_name()
                                                .c_str();
                            if (n && n[0])
                                dname = n;
                        }
                        char item_lbl[96];
                        snprintf(item_lbl,
                                 sizeof(item_lbl),
                                 "Edit tr -> %s##%d",
                                 dname,
                                 ti);
                        if (ImGui::MenuItem(item_lbl))
                        {
                            m_sel_tr_src = m_ctx_node_id;
                            m_sel_tr_idx = ti;
                        }
                    }
                    ImGui::Separator();
                }
                if (ImGui::MenuItem("Delete state"))
                    delete_state_at(app, m_ctx_node_id);
            }
        }
        ImGui::EndPopup();
    }

    // ---- Background context menu (add state) ----
    if (m_canvas.bg_right_clicked())
        ImGui::OpenPopup("agw_canvas_ctx");
    if (ImGui::BeginPopup("agw_canvas_ctx"))
    {
        if (ImGui::MenuItem("Add clip state"))
        {
            auto *s = new animator_clip_state();
            s->set_state_name(symbol_ref("new_clip_state"));
            m_graph->states().push_back(pointer<animator_state>(s));
            m_node_positions.push_back(menu_canvas_pos);
            m_dirty = true;
        }
        ImGui::EndPopup();
    }

    // Cancel link gesture on Esc or bg left-click.
    if (m_linking_from != LINK_NONE &&
        (ImGui::IsKeyPressed(ImGuiKey_Escape) || m_canvas.bg_clicked()))
        m_linking_from = LINK_NONE;

    // Deselect on background click (not in link mode, no link/bend clicked).
    if (m_linking_from == LINK_NONE && m_canvas.bg_clicked() &&
        !link_was_clicked && !m_bend_handle_active && !ImGui::GetIO().KeyCtrl)
    {
        m_sel_tr_src = LINK_NONE;
        m_sel_tr_idx = -1;
        multi_sel_clear();
        send_deselect(app);
    }

    m_canvas.end();
}

void animator_graph_window::render(editor_app &app, real dtime)
{
    if (!m_open)
        return;

    if (m_preview && m_graph.get())
    {
        spatial *root = nullptr;
        if (m_preview->get_object() &&
            m_preview->get_object()->is_derived(spatial::TYPE))
            root = static_cast<spatial *>(m_preview->get_object());

        if (m_preview_bound_graph != m_graph.get() ||
            m_preview_bound_skel != root)
        {
            m_preview->set_graph_direct(m_graph, root);
            m_preview_bound_graph = m_graph.get();
            m_preview_bound_skel  = root;
        }
        if (m_preview_playing)
            m_preview->update(dtime);
    }
    else if (!m_preview)
    {
        m_preview_bound_graph = nullptr;
    }

    if (ImGui::Begin("Animator Graph", &m_open))
    {
        handle_shortcuts(app);
        render_toolbar(app);
        ImGui::Separator();
        if (ImGui::BeginTable("agw_layout",
                              2,
                              ImGuiTableFlags_BordersInnerV |
                                  ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn(
                "Left", ImGuiTableColumnFlags_WidthStretch, 0.25f);
            ImGui::TableSetupColumn(
                "Graph", ImGuiTableColumnFlags_WidthStretch, 0.75f);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::BeginChild("agw_left", ImVec2(0, 0), true);
            render_left_panel(app);
            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("agw_canvas", ImVec2(0, 0), true);
            render_canvas_panel(app);
            ImGui::EndChild();

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

} // namespace zabato::editor
