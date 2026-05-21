#include <zabato/animator.hpp>
#include <zabato/animator_graph.hpp>
#include <zabato/animator_state.hpp>
#include <zabato/game_message.hpp>
#include <zabato/node.hpp>
#include <zabato/object_resource.hpp>
#include <zabato/resource.hpp>
#include <zabato/script.hpp>
#include <zabato/spatial.hpp>
#include <zabato/symbol.hpp>

namespace zabato
{

const rtti
    animator::TYPE("zabato.animator", &controller::TYPE, animator::reflect);

static void
w_bind_property(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 4)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(animator::TYPE))
        return;
    auto *ctrl = static_cast<animator *>(self.get());

    string track = string(args->get_value(1).as_string());

    controller *target = nullptr;
    value v_target     = args->get_value(2);
    if (v_target.is_object())
        target = c_dynamic_cast<controller>(v_target.as_object().get());

    string prop = string(args->get_value(3).as_string());

    if (target)
        ctrl->bind_property(track.c_str(), target, prop.c_str());
}

static void w_play_graph(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(animator::TYPE))
        return;
    auto *a = static_cast<animator *>(self.get());
    resource_ref ref;
    if (args->get_value(1).is_string())
        ref = resource_ref(args->get_value(1).as_string(),
                           a->get_graph().manager());
    spatial *root = nullptr;
    if (a->get_object() && a->get_object()->is_derived(spatial::TYPE))
        root = static_cast<spatial *>(a->get_object());
    a->play_graph(ref, root);
}

void animator::reflect(reflection &r)
{
    controller::reflect(r);
    r.add_method("bind_property", w_bind_property);
    r.add_method("play_graph", w_play_graph);
}

void animator::bind_property(const char *track_name,
                             controller *target,
                             const char *prop_name)
{
    if (!target)
        return;
    m_bound_properties.push_back(
        {symbol_ref(track_name), pointer<controller>(target), prop_name});
}

void animator::apply_bound_properties(const animator_clip_state &cs,
                                      real prev_st,
                                      real cur_st)
{
    auto anim_res = cs.clip().get<animation>();
    if (!anim_res)
        return;
    animation &anim = *anim_res;
    real prev_ticks = cs.compute_ticks(prev_st);
    real cur_ticks  = cs.compute_ticks(cur_st);
    bool wrapped    = cs.loop() && cur_ticks < prev_ticks;

    for (const auto &b : m_bound_properties)
    {
        if (!b.target)
            continue;
        const char *tn = b.track_name.c_str();
        const char *pn = b.property_name.c_str();

        for (auto &t : anim.get_real_tracks())
            if (t.property_name == tn)
                b.target->set_property(pn, t.get_value(cur_ticks));

        for (auto &t : anim.get_int_tracks())
            if (t.property_name == tn)
                b.target->set_property(pn, t.get_value(cur_ticks));

        for (auto &t : anim.get_bool_tracks())
            if (t.property_name == tn)
                b.target->set_property(pn, t.get_value(cur_ticks));

        for (auto &t : anim.get_string_tracks())
            if (t.property_name == tn)
                b.target->set_property(pn, t.get_value(cur_ticks));

        for (const auto &t : anim.get_event_tracks())
        {
            if (t.property_name != tn)
                continue;
            for (const auto &key : t.keys)
            {
                real ts    = (real)key.timestamp;
                bool fired = wrapped ? (ts > prev_ticks || ts <= cur_ticks)
                                     : (ts > prev_ticks && ts <= cur_ticks);
                if (fired)
                {
                    game_message msg;
                    msg.msg_id = key.name.c_str();
                    msg.data   = value(string_view(key.args));
                    b.target->on_message(msg);
                }
            }
        }
    }
}

void animator::ensure_graph_loaded()
{
    if (m_graph.get() || m_graph_ref.path().empty())
        return;

    auto res = m_graph_ref.get<object_resource>();
    if (!res)
        return;
    auto root = res->get_object();
    if (!root)
        return;
    auto *g = c_dynamic_cast<animator_graph>(root.get());
    if (!g)
        return;
    m_graph       = pointer<animator_graph>(g);
    m_graph_dirty = false;

    if (get_object() && get_object()->is_derived(spatial::TYPE))
        rebind_skeleton_bones(static_cast<spatial *>(get_object()));

    m_current_state   = g->entry_state_index();
    m_state_time      = real(0);
    m_prev_state_time = real(0);
    m_transitioning   = false;
    m_next_state      = (size_t)-1;
    m_blend_time      = real(0);
    m_blend_duration  = real(0);
}

void animator::rebind_skeleton_bones(spatial *root)
{
    m_bound_bones.clear();
    if (!root)
        return;
    auto walk = [&](auto &&self, spatial *node) -> void
    {
        if (!node)
            return;
        animator_bound_bone b;
        b.name = symbol_ref(node->name());
        b.node = pointer<spatial>(node);
        m_bound_bones.push_back(b);
        auto n = c_dynamic_cast<class node>(node);
        if (n)
        {
            for (int i = 0; i < n->quantity(); ++i)
                self(self, n->child_at(i));
        }
    };
    walk(walk, root);
}

void animator::play_graph(const resource_ref &graph_ref, spatial *root)
{
    m_graph_ref   = graph_ref;
    m_graph       = nullptr;
    m_graph_dirty = true;
    if (root)
        rebind_skeleton_bones(root);
    ensure_graph_loaded();
}

void animator::set_graph_direct(pointer<animator_graph> g, spatial *root)
{
    m_graph_ref   = resource_ref();
    m_graph       = g;
    m_graph_dirty = false;

    if (root)
        rebind_skeleton_bones(root);

    m_current_state = m_graph.get() ? m_graph->entry_state_index() : (size_t)-1;
    m_state_time    = real(0);
    m_prev_state_time = real(0);
    m_transitioning   = false;
    m_next_state      = (size_t)-1;
    m_blend_time      = real(0);
    m_blend_duration  = real(0);
}

void animator::on_message(const game_message &msg)
{
    if (msg.msg_id.empty())
        return;
    m_pending_messages.push_back(msg.msg_id);
}

bool animator::eval_transition(animator_state *state,
                               const animator_transition &tr)
{
    if (tr.has_exit_time && state)
    {
        real dur  = state->state_duration(*this);
        real need = dur * tr.exit_time_norm;
        if (m_state_time < need)
            return false;
    }

    if (!tr.message_trigger.empty())
    {
        bool found = false;
        for (const auto &m : m_pending_messages)
        {
            if (m == tr.message_trigger)
            {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }

    return true;
}

void animator::begin_transition(size_t next_idx, real duration)
{
    m_transitioning  = true;
    m_next_state     = next_idx;
    m_blend_time     = real(0);
    m_blend_duration = duration;
}

void animator::apply_pose(const vector<pose_sample> &pose)
{
    for (size_t i = 0; i < pose.size() && i < m_bound_bones.size(); ++i)
    {
        if (!pose[i].sampled)
            continue;
        spatial *node = m_bound_bones[i].node.get();
        if (!node)
            continue;
        transformation t = node->get_local();
        t.set_translate(pose[i].xform.translate());
        t.set_rotate(pose[i].xform.rotate());
        t.set_scale(pose[i].xform.scale());
        node->set_local(t);
    }
}

void animator::blend_apply_pose(const vector<pose_sample> &a,
                                const vector<pose_sample> &b,
                                real t)
{
    const size_t n = m_bound_bones.size();
    for (size_t i = 0; i < n; ++i)
    {
        const bool sa = i < a.size() && a[i].sampled;
        const bool sb = i < b.size() && b[i].sampled;
        if (!sa && !sb)
            continue;
        spatial *node = m_bound_bones[i].node.get();
        if (!node)
            continue;
        transformation out = node->get_local();
        if (sa && sb)
        {
            out.set_translate(
                lerp(a[i].xform.translate(), b[i].xform.translate(), t));
            out.set_rotate(slerp(a[i].xform.rotate(), b[i].xform.rotate(), t));
            out.set_scale(lerp(a[i].xform.scale(), b[i].xform.scale(), t));
        }
        else if (sa)
        {
            out.set_translate(a[i].xform.translate());
            out.set_rotate(a[i].xform.rotate());
            out.set_scale(a[i].xform.scale());
        }
        else
        {
            out.set_translate(b[i].xform.translate());
            out.set_rotate(b[i].xform.rotate());
            out.set_scale(b[i].xform.scale());
        }
        node->set_local(out);
    }
}

void animator::update(real delta_time)
{
    ensure_graph_loaded();

    if (!m_graph || m_graph->states().empty() ||
        m_current_state >= m_graph->states().size())
        return;

    real prev_st = m_state_time;
    m_state_time += delta_time;
    if (m_transitioning)
        m_blend_time += delta_time;

    animator_state *cur = m_graph->states()[m_current_state].get();
    animator_state *nxt =
        m_transitioning && m_next_state < m_graph->states().size()
            ? m_graph->states()[m_next_state].get()
            : nullptr;

    // Sample and apply pose.
    if (cur && !m_bound_bones.empty())
    {
        vector<pose_sample> a_pose, b_pose;
        cur->sample(*this, m_state_time, a_pose);
        if (nxt && m_blend_duration > real(0))
        {
            real t = m_blend_time / m_blend_duration;
            if (t > real(1))
                t = real(1);
            nxt->sample(*this, m_blend_time, b_pose);
            blend_apply_pose(a_pose, b_pose, t);
        }
        else if (nxt)
        {
            vector<pose_sample> npose;
            nxt->sample(*this, m_blend_time, npose);
            apply_pose(npose);
        }
        else
        {
            apply_pose(a_pose);
        }
    }

    // Commit transition when complete.
    if (m_transitioning && m_blend_time >= m_blend_duration)
    {
        m_current_state  = m_next_state;
        m_state_time     = m_blend_time;
        prev_st          = real(0);
        m_transitioning  = false;
        m_next_state     = (size_t)-1;
        m_blend_time     = real(0);
        m_blend_duration = real(0);
        cur              = m_graph->states()[m_current_state].get();
    }

    if (!m_transitioning && cur)
    {
        // Fire animation events to sibling controllers.
        cur->fire_events(*this, prev_st, m_state_time);

        // Drive bound property tracks.
        if (!m_bound_properties.empty())
        {
            auto *cs = c_dynamic_cast<animator_clip_state>(cur);
            if (cs)
                apply_bound_properties(*cs, prev_st, m_state_time);
        }
    }

    // Evaluate outgoing transitions for the current state.
    if (!m_transitioning && cur)
    {
        for (const auto &tr : cur->transitions())
        {
            if (eval_transition(cur, tr))
            {
                begin_transition(tr.dst_state_index, tr.duration);
                break;
            }
        }
    }

    // "From Any State" transitions.
    if (!m_transitioning)
    {
        for (const auto &tr : m_graph->any_state_transitions())
        {
            if (tr.dst_state_index == m_current_state)
                continue;
            if (eval_transition(nullptr, tr))
            {
                begin_transition(tr.dst_state_index, tr.duration);
                break;
            }
        }
    }

    m_pending_messages.clear();
}

} // namespace zabato
