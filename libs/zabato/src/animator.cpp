#include <zabato/animator.hpp>
#include <zabato/game_message.hpp>
#include <zabato/node.hpp>
#include <zabato/resource.hpp>
#include <zabato/script.hpp>
#include <zabato/serializer.hpp>
#include <zabato/spatial.hpp>
#include <zabato/symbol.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti
    animator::TYPE("zabato.animator", &controller::TYPE, animator::reflect);

static void
w_play_animation(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 2)
        return;

    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(animator::TYPE))
        return;

    auto *ctrl = static_cast<animator *>(self.get());

    value v_anim = args->get_value(1);
    resource_ref anim_ref;

    if (v_anim.is_string())
    {
        anim_ref =
            resource_ref(v_anim.as_string(), ctrl->animation_ref().manager());
    }

    // Arg 2: Loop boolean
    bool loop = false;
    if (args->count() >= 3)
        loop = args->get_value(2).as_bool();

    // Root: Use the node attached to the animator
    spatial *root = nullptr;
    if (ctrl->get_object() && ctrl->get_object()->is_derived(spatial::TYPE))
        root = static_cast<spatial *>(ctrl->get_object());

    ctrl->play_animation(anim_ref, root, loop);
}

static void
w_bind_node(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 2)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(animator::TYPE))
        return;
    auto *ctrl = static_cast<animator *>(self.get());

    string bone_name = string(args->get_value(1).as_string());

    spatial *node = nullptr;
    if (args->count() >= 3)
    {
        value v_node = args->get_value(2);
        if (v_node.is_object())
            node = c_dynamic_cast<spatial>(v_node.as_object().get());
    }

    ctrl->bind_node(bone_name.c_str(), node);
}

static void
w_get_bone_count(script_system *sys, script_instance *inst, script_args *args)
{
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(animator::TYPE))
        return;
    auto *ctrl = static_cast<animator *>(self.get());
    args->push_return((int64_t)ctrl->get_bone_count());
}

static void
w_get_bone_name(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 2)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(animator::TYPE))
        return;
    auto *ctrl = static_cast<animator *>(self.get());
    int index  = args->get_value(1).as_int();

    const char *name = ctrl->get_bone_name(index);
    if (name)
        args->push_return(name);
    else
        args->push_return(value());
}

static void
w_bind_property(script_system *sys, script_instance *inst, script_args *args)
{
    if (args->count() < 4)
        return;
    auto self = args->get_value(0).as_object();
    if (!self || !self->is_derived(animator::TYPE))
        return;
    auto *ctrl = static_cast<animator *>(self.get());

    // Arg 1: Track name
    string track = string(args->get_value(1).as_string());

    // Arg 2: Target controller
    controller *target = nullptr;
    value v_target     = args->get_value(2);
    if (v_target.is_object())
        target = c_dynamic_cast<controller>(v_target.as_object().get());

    // Arg 3: Property name
    string prop = string(args->get_value(3).as_string());

    if (target)
        ctrl->bind_property(track.c_str(), target, prop.c_str());
}

static void
animator_anim_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    auto v      = args->get_value(0);
    auto o      = v.as_object();
    animator *m = c_dynamic_cast<animator>(o.get());
    if (m)
    {
        args->push_return(string(m->get_animation().path()).c_str());
    }
}

static void
animator_anim_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    auto v1     = args->get_value(0);
    auto o      = v1.as_object();
    animator *m = c_dynamic_cast<animator>(o.get());
    auto v2     = args->get_value(1);
    if (m && v2.is_string())
    {
        resource_ref anim_ref(v2.as_string(), m->get_animation().manager());

        spatial *root = nullptr;
        if (m->get_object() && m->get_object()->is_derived(spatial::TYPE))
            root = static_cast<spatial *>(m->get_object());

        m->play_animation(anim_ref, root, m->get_loop());
    }
}

static void
animator_loop_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    auto v      = args->get_value(0);
    auto o      = v.as_object();
    animator *m = c_dynamic_cast<animator>(o.get());
    if (m)
    {
        args->push_return(m->get_loop());
    }
}

static void
animator_loop_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    auto v1     = args->get_value(0);
    auto o      = v1.as_object();
    animator *m = c_dynamic_cast<animator>(o.get());
    auto v2     = args->get_value(1);
    if (m)
    {
        m->set_loop(v2.as_bool());
    }
}

static void
animator_skeleton_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    auto v      = args->get_value(0);
    auto o      = v.as_object();
    animator *m = c_dynamic_cast<animator>(o.get());
    if (m)
        args->push_return((object *)m->get_skeleton_root().get());
}

static void
animator_skeleton_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    auto v1     = args->get_value(0);
    auto o      = v1.as_object();
    animator *m = c_dynamic_cast<animator>(o.get());

    if (!m)
    {
        args->error("Invalid animator");
        return;
    }

    auto v2 = args->get_value(1);
    if (v2.is_nil())
    {
        m->bind_skeleton(nullptr);
        return;
    }
    auto obj = v2.as_object();
    if (!obj)
    {
        args->error("Invalid skeleton root is not an object");
        return;
    }

    auto root = c_dynamic_cast<spatial>(obj.get());
    if (!root)
    {
        args->error("Invalid skeleton root is not a spatial");
        return;
    }
    m->bind_skeleton(root);
}

void animator::reflect(reflection &r)
{
    controller::reflect(r);
    r.add_method("play_animation", w_play_animation);
    r.add_method("bind_node", w_bind_node);
    r.add_method("bind_property", w_bind_property);
    r.add_method("get_bone_count", w_get_bone_count);
    r.add_method("get_bone_name", w_get_bone_name);

    value anim_attrs = value::make_map();
    anim_attrs.set_field("asset_type", "animation");

    r.add_property(
        "animation", animator_anim_getter, animator_anim_setter, anim_attrs);
    r.add_property("loop", animator_loop_getter, animator_loop_setter);

    value skeleton_attrs = value::make_map();
    r.add_property("skeleton",
                   animator_skeleton_getter,
                   animator_skeleton_setter,
                   skeleton_attrs);
}

static void recursive_bind(animator *animator, spatial *root)
{
    if (!root)
        return;

    animator->bind_node(root->name(), root);

    pointer<node> n = c_dynamic_cast<node>(root);
    if (n)
    {
        for (int i = 0; i < n->quantity(); ++i)
            recursive_bind(animator, n->child_at(i));
    }
}

void animator::play_animation(const resource_ref &anim_ref,
                              spatial *root,
                              bool loop)
{
    m_current_animation_ref = anim_ref;
    m_loop                  = loop;
    m_current_time          = real(0);
    m_bound_nodes.clear();

    if (m_current_animation_ref.path().empty())
    {
        m_current_animation = nullptr;
        return;
    }

    auto res = m_current_animation_ref.get<animation>();
    if (res)
        m_current_animation = res.get();
    else
        m_current_animation = nullptr;

    if (!root)
        return;

    recursive_bind(this, root);
}

void animator::bind_node(const char *bone_name, spatial *node)
{
    if (!m_current_animation)
        return;

    auto index = m_current_animation->find_bone_index(bone_name);
    if (index >= 0)
    {
        for (auto it = m_bound_nodes.begin(); it != m_bound_nodes.end(); ++it)
        {
            if (it->channel_index == index)
            {
                if (node)
                    it->node = node;
                else
                    m_bound_nodes.erase(it);
                return;
            }
        }

        if (node)
            m_bound_nodes.push_back({(uint16_t)index, node});
    }
}

size_t animator::get_bone_count() const
{
    if (m_current_animation)
        return m_current_animation->get_bones().size();
    return 0;
}

const char *animator::get_bone_name(size_t index) const
{
    if (m_current_animation && index < m_current_animation->get_bones().size())
        return m_current_animation->get_bones()[index].track.bone_name.c_str();
    return nullptr;
}

void animator::bind_skeleton(pointer<spatial> root)
{
    m_skeleton_root = root;
    m_bound_nodes.clear();

    if (root && m_current_animation)
    {
        const auto &bones = m_current_animation->get_bones();
        for (size_t i = 0; i < bones.size(); ++i)
        {
            const auto &info        = bones[i];
            const char *target_name = info.track.bone_name.c_str();

            object *obj   = root->get_object_by_name(target_name);
            spatial *bone = c_dynamic_cast<spatial>(obj);
            if (bone)
            {
                m_bound_nodes.push_back({(uint16_t)i, bone});
            }
        }
    }
}

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
    if (!m_current_animation && !m_current_animation_ref.path().empty())
    {
        auto res = m_current_animation_ref.get<animation>();
        if (res)
        {
            m_current_animation = res.get();
            // Rebind nodes if root is spatial
            if (get_object() && get_object()->is_derived(spatial::TYPE))
            {
                m_bound_nodes.clear();
                recursive_bind(this, static_cast<spatial *>(get_object()));
            }
        }
    }

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
                auto &track      = bones[bound.channel_index].track;
                transformation t = bound.node->get_local();
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
                    msg.msg_id      = key.name.c_str();
                    msg.sender_id   = 0;
                    msg.receiver_id = 0;

                    value arg_view_val(string_view(key.args));
                    msg.data = arg_view_val;

                    b.target->on_message(msg);
                }
            }
        }
    }
}

} // namespace zabato