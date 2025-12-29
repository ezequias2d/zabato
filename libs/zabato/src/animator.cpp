#include <zabato/animator.hpp>
#include <zabato/game_message.hpp>
#include <zabato/symbol.hpp>

namespace zabato
{

static void recursive_bind(animator *animator, spatial *root)
{
    if (!root)
        return;

    animator->bind_node(root->name(), root);

    node *n = c_dynamic_cast<node>(root);
    if (n)
    {
        for (int i = 0; i < n->quantity(); ++i)
            recursive_bind(animator, n->child_at(i));
    }
}

void animator::play_animation(animation *anim, spatial *root, bool loop)
{
    m_current_animation = anim;
    m_loop              = loop;
    m_current_time      = real(0);
    m_bound_nodes.clear();

    if (!anim || !root)
        return;

    recursive_bind(this, root);
}

void animator::bind_node(const char *bone_name, spatial *node)
{
    if (!m_current_animation || !node)
        return;

    auto index = m_current_animation->find_bone_index(bone_name);
    if (index >= 0)
    {
        m_bound_nodes.push_back({(uint16_t)index, node});
    }
}
const rtti animator::TYPE = rtti("animator", &controller::TYPE);

void animator::bind_property(const char *track_name,
                             controller *target,
                             const char *prop_name)
{
    if (!m_current_animation || !target)
        return;

    // Real tracks
    auto &reals = m_current_animation->get_real_tracks();
    for (size_t i = 0; i < reals.size(); ++i)
    {
        if (reals[i].property_name == track_name)
        {
            m_bound_reals.push_back({(uint16_t)i, target, prop_name});
            return;
        }
    }

    // Int tracks
    auto &ints = m_current_animation->get_int_tracks();
    for (size_t i = 0; i < ints.size(); ++i)
    {
        if (ints[i].property_name == track_name)
        {
            m_bound_ints.push_back({(uint16_t)i, target, prop_name});
            return;
        }
    }

    // Bool tracks
    auto &bools = m_current_animation->get_bool_tracks();
    for (size_t i = 0; i < bools.size(); ++i)
    {
        if (bools[i].property_name == track_name)
        {
            m_bound_bools.push_back({(uint16_t)i, target, prop_name});
            return;
        }
    }

    // String tracks
    auto &strings = m_current_animation->get_string_tracks();
    for (size_t i = 0; i < strings.size(); ++i)
    {
        if (strings[i].property_name == track_name)
        {
            m_bound_strings.push_back({(uint16_t)i, target, prop_name});
            return;
        }
    }

    // Event tracks
    auto &events = m_current_animation->get_event_tracks();
    for (size_t i = 0; i < events.size(); ++i)
    {
        if (events[i].property_name == track_name)
        {
            m_bound_events.push_back({(uint16_t)i, target, prop_name});
            return;
        }
    }
}

void animator::update(real delta_time)
{
    if (!m_current_animation)
        return;

    real last_time = m_current_time;
    m_current_time += m_current_animation->get_ticks_per_second() * delta_time;

    if (m_loop)
    {
        m_current_time =
            mod(m_current_time, m_current_animation->get_duration());
    }
    else
    {
        if (m_current_time > m_current_animation->get_duration())
            m_current_time = m_current_animation->get_duration();
    }

    // Update bound scene graph nodes
    if (!m_bound_nodes.empty())
    {
        auto &bones = m_current_animation->get_bones();
        for (const auto &bound : m_bound_nodes)
        {
            if (bound.channel_index < bones.size() && bound.node)
            {
                auto &track = bones[bound.channel_index].track;
                transformation t;
                t.set_translate(track.get_position(m_current_time));
                t.set_rotate(track.get_rotation(m_current_time));
                t.set_scale(track.get_scale(m_current_time));
                bound.node->set_local(t);
            }
        }
    }

    // Real
    auto &reals = m_current_animation->get_real_tracks();
    for (const auto &b : m_bound_reals)
    {
        if (b.target && b.track_index < reals.size())
        {
            real val = reals[b.track_index].get_value(m_current_time);
            b.target->set_property(b.property_name.c_str(), val);
        }
    }

    // Int
    auto &ints = m_current_animation->get_int_tracks();
    for (const auto &b : m_bound_ints)
    {
        if (b.target && b.track_index < ints.size())
        {
            int64_t val = ints[b.track_index].get_value(m_current_time);
            b.target->set_property(b.property_name.c_str(), val);
        }
    }

    // Bool
    auto &bools = m_current_animation->get_bool_tracks();
    for (const auto &b : m_bound_bools)
    {
        if (b.target && b.track_index < bools.size())
        {
            bool val = bools[b.track_index].get_value(m_current_time);
            b.target->set_property(b.property_name.c_str(), val);
        }
    }

    // String
    auto &strings = m_current_animation->get_string_tracks();
    for (const auto &b : m_bound_strings)
    {
        if (b.target && b.track_index < strings.size())
        {
            const char *val = strings[b.track_index].get_value(m_current_time);
            b.target->set_property(b.property_name.c_str(), val);
        }
    }

    // Events
    auto &events = m_current_animation->get_event_tracks();
    for (const auto &b : m_bound_events)
    {
        if (b.target && b.track_index < events.size())
        {
            const auto &keys = events[b.track_index].keys;
            for (const auto &key : keys)
            {
                bool fired = false;
                if (m_loop && m_current_time < last_time)
                {
                    // wrapped around
                    if (key.timestamp > last_time ||
                        key.timestamp <= m_current_time)
                        fired = true;
                }
                else
                {
                    if (key.timestamp > last_time &&
                        key.timestamp <= m_current_time)
                        fired = true;
                }

                if (fired)
                {
                    game_message msg;
                    msg.msg_id      = get_symbol(key.name.c_str());
                    msg.sender_id   = 0;
                    msg.receiver_id = 0;

                    string_view arg_view(key.args.c_str(), key.args.size());
                    value arg_view_val(arg_view);
                    msg.data = arg_view_val;

                    b.target->on_message(msg);
                }
            }
        }
    }
}
} // namespace zabato