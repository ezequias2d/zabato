#include <imgui.h>

#include <editor/asset_database.hpp>
#include <editor/core/editor_registry.hpp>
#include <editor/editor.hpp>

#include <zabato/animation.hpp>
#include <zabato/animator_graph.hpp>
#include <zabato/animator_state.hpp>

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

animator_graph *find_owning_graph(const animator_state *s)
{
    if (!s)
        return nullptr;
    for (auto &pair : object::s_in_use)
    {
        auto *obj = pair.value;
        if (!obj)
            continue;
        auto *g = c_dynamic_cast<animator_graph>(obj);
        if (!g)
            continue;
        for (const auto &st : g->states())
            if (st.get() == s)
                return g;
    }
    return nullptr;
}

void draw_state_common(animator_state *st)
{
    char name_buf[64];
    copy_into_buf(&name_buf[0], sizeof(name_buf), st->state_name().c_str());
    if (ImGui::InputText("name", &name_buf[0], sizeof(name_buf)))
        st->set_state_name(symbol_ref(&name_buf[0]));
}

void clip_state_editor(void *obj, editor_app &app, real dtime)
{
    auto *s = (animator_clip_state *)obj;
    if (!s)
        return;
    ImGui::TextDisabled("Clip State");
    draw_state_common(s);

    string path(s->clip().c_path());
    if (draw_asset_selector(
            "clip", path, asset_type::animation, app.get_asset_database()))
    {
        s->set_clip(resource_ref(string_view(path),
                                 s->clip().manager()
                                     ? s->clip().manager()
                                     : app.get_resource_manager()));
    }
    bool loop = s->loop();
    if (ImGui::Checkbox("loop", &loop))
        s->set_loop(loop);

    auto anim = s->clip().get<animation>();
    if (anim)
    {
        float full  = (float)anim->get_duration();
        float tps   = (float)anim->get_ticks_per_second();
        float start = (float)s->start_tick();
        float end   = (s->end_tick() < real(0)) ? full : (float)s->end_tick();
        if (tps <= 0.0f)
            tps = 1.0f;
        float start_s = start / tps;
        float end_s   = end / tps;
        float full_s  = full / tps;
        bool changed  = false;
        if (ImGui::DragFloatRange2("slice (s)",
                                   &start_s,
                                   &end_s,
                                   0.01f,
                                   0.0f,
                                   full_s,
                                   "%.3f",
                                   "%.3f"))
            changed = true;
        if (changed)
        {
            if (start_s < 0.0f)
                start_s = 0.0f;
            if (end_s > full_s)
                end_s = full_s;
            if (end_s < start_s)
                end_s = start_s;
            s->set_start_tick((real)(start_s * tps));
            s->set_end_tick(end_s >= full_s ? real(-1) : (real)(end_s * tps));
        }
        ImGui::TextDisabled(
            "clip: %.2fs @ %.1f tps  |  slice: %.2f..%.2fs  |  length: %.2fs",
            full_s,
            tps,
            start_s,
            end_s,
            end_s - start_s);
        if (ImGui::SmallButton("reset slice"))
        {
            s->set_start_tick(real(0));
            s->set_end_tick(real(-1));
        }
    }
    else
    {
        float start = (float)s->start_tick();
        float end   = (float)s->end_tick();
        if (ImGui::InputFloat("start (ticks)", &start))
            s->set_start_tick((real)start);
        if (ImGui::InputFloat("end (ticks, -1=full)", &end))
            s->set_end_tick((real)end);
    }
    // Transitions are edited in the Animator Graph window (left panel).
    ImGui::TextDisabled("(transitions: right-click state in Animator Graph)");
}

void graph_editor(void *obj, editor_app &app, real dtime)
{
    auto *g = (animator_graph *)obj;
    if (!g)
        return;
    ImGui::TextDisabled("Animator Graph");

    int entry = (int)g->entry_state_index();
    if (ImGui::InputInt("entry state", &entry))
    {
        if (entry < 0)
            entry = 0;
        if (!g->states().empty() && entry >= (int)g->states().size())
            entry = (int)g->states().size() - 1;
        g->set_entry_state_index((size_t)entry);
    }
    ImGui::TextDisabled(
        "(transitions and triggers: use the Animator Graph window)");
}

} // namespace

void register_animator_inspectors()
{
    editor_registry::register_editor(animator_clip_state::TYPE,
                                     clip_state_editor);
    editor_registry::register_editor(animator_graph::TYPE, graph_editor);
}

} // namespace zabato::editor
