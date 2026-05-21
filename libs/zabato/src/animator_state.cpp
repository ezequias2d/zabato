#include <string.h>
#include <tinyxml2.h>

#include <zabato/animation.hpp>
#include <zabato/animator.hpp>
#include <zabato/animator_state.hpp>
#include <zabato/game_message.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/serializer.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti animator_state::TYPE("zabato.animator_state",
                                &object::TYPE,
                                animator_state::reflect);

void animator_state::reflect(reflection &r) { object::reflect(r); }

void animator_state::save(serializer &s) const
{
    object::save(s);
    s.write(string_view(m_name.c_str()));
    s.write((uint64_t)m_transitions.size());
    for (const auto &t : m_transitions)
    {
        s.write((uint64_t)t.dst_state_index);
        s.write(string_view(t.message_trigger.c_str()));
        s.write((double)t.exit_time_norm);
        s.write((double)t.duration);
        s.write((uint8_t)(t.has_exit_time ? 1 : 0));
        s.write((uint8_t)(t.interruptible ? 1 : 0));
    }
}

void animator_state::load(serializer &s, serializer_link *link)
{
    object::load(s, link);
    string n;
    s.read(n);
    m_name = symbol_ref(n.c_str());

    uint64_t cnt = 0;
    s.read(cnt);
    m_transitions.resize(cnt);
    for (auto &t : m_transitions)
    {
        uint64_t idx = 0;
        s.read(idx);
        t.dst_state_index = (size_t)idx;

        string trig;
        s.read(trig);
        t.message_trigger = symbol_ref(trig.c_str());

        double etn = 1.0;
        s.read(etn);
        t.exit_time_norm = (real)etn;

        double dur = 0.0;
        s.read(dur);
        t.duration = (real)dur;

        uint8_t het = 0;
        s.read(het);
        t.has_exit_time = het != 0;

        uint8_t ipt = 1;
        s.read(ipt);
        t.interruptible = ipt != 0;
    }
}

void animator_state::save_xml(xml_serializer &s, tinyxml2::XMLElement &el) const
{
    object::save_xml(s, el);
    el.SetAttribute("state_name", m_name.c_str());
    for (const auto &t : m_transitions)
    {
        tinyxml2::XMLElement *te = el.GetDocument()->NewElement("transition");
        te->SetAttribute("dst_state", (int64_t)t.dst_state_index);
        if (!t.message_trigger.empty())
            te->SetAttribute("message", t.message_trigger.c_str());
        te->SetAttribute("exit_time_norm", (double)t.exit_time_norm);
        te->SetAttribute("duration", (double)t.duration);
        te->SetAttribute("has_exit_time", t.has_exit_time);
        te->SetAttribute("interruptible", t.interruptible);
        el.InsertEndChild(te);
    }
}

void animator_state::load_xml(xml_serializer &s, tinyxml2::XMLElement &el)
{
    object::load_xml(s, el);
    const char *n = el.Attribute("state_name");
    m_name        = symbol_ref(n ? n : "");
    m_transitions.clear();
    for (tinyxml2::XMLElement *te = el.FirstChildElement("transition"); te;
         te                       = te->NextSiblingElement("transition"))
    {
        animator_transition t;
        int64_t idx = 0;
        te->QueryInt64Attribute("dst_state", &idx);
        t.dst_state_index = (size_t)idx;

        const char *msg   = te->Attribute("message");
        t.message_trigger = symbol_ref(msg ? msg : "");

        double etn = 1.0;
        te->QueryDoubleAttribute("exit_time_norm", &etn);
        t.exit_time_norm = (real)etn;

        double dur = 0.0;
        te->QueryDoubleAttribute("duration", &dur);
        t.duration = (real)dur;

        bool het = false;
        te->QueryBoolAttribute("has_exit_time", &het);
        t.has_exit_time = het;

        bool ipt = true;
        te->QueryBoolAttribute("interruptible", &ipt);
        t.interruptible = ipt;

        m_transitions.push_back(t);
    }
}

// ---------- clip_state ----------

const rtti animator_clip_state::TYPE("zabato.animator_clip_state",
                                     &animator_state::TYPE,
                                     animator_clip_state::reflect);

void animator_clip_state::reflect(reflection &r) { animator_state::reflect(r); }

real animator_clip_state::state_duration(animator &ctx) const
{
    auto anim = m_clip.get<animation>();
    if (!anim)
        return real(0);
    real tps = anim->get_ticks_per_second();
    if (tps <= real(0))
        return real(0);
    real full = anim->get_duration();
    real lo   = m_start_tick > real(0) ? m_start_tick : real(0);
    real hi  = (m_end_tick > real(0) && m_end_tick <= full) ? m_end_tick : full;
    real rng = hi - lo;
    if (rng <= real(0))
        rng = full;
    return rng / tps;
}

static void sample_clip_into(animation *anim,
                             real anim_time_ticks,
                             const vector<animator_bound_bone> &bones,
                             vector<pose_sample> &out)
{
    out.resize(bones.size());
    if (!anim)
    {
        for (auto &p : out)
            p.sampled = false;
        return;
    }
    auto &channels = anim->get_bones();
    for (size_t i = 0; i < bones.size(); ++i)
    {
        pose_sample &ps = out[i];
        ps.sampled      = false;
        const char *bn  = bones[i].name.c_str();
        for (auto &ch : channels)
        {
            if (ch.track.bone_name == bn)
            {
                ps.xform.set_translate(ch.track.get_position(anim_time_ticks));
                ps.xform.set_rotate(ch.track.get_rotation(anim_time_ticks));
                ps.xform.set_scale(ch.track.get_scale(anim_time_ticks));
                ps.sampled = true;
                break;
            }
        }
    }
}

real animator_clip_state::compute_ticks(real local_time) const
{
    auto anim = m_clip.get<animation>();
    if (!anim)
        return real(0);
    real tps  = anim->get_ticks_per_second();
    real full = anim->get_duration();
    real lo   = m_start_tick > real(0) ? m_start_tick : real(0);
    real hi  = (m_end_tick > real(0) && m_end_tick <= full) ? m_end_tick : full;
    real rng = hi - lo;
    if (rng <= real(0))
    {
        lo  = real(0);
        hi  = full;
        rng = full;
    }
    if (tps <= real(0))
        return lo;
    real t = local_time * tps;
    if (m_loop && rng > real(0))
        t = mod(t, rng);
    else if (t > rng)
        t = rng;
    return lo + t;
}

void animator_clip_state::sample(animator &ctx,
                                 real local_time,
                                 vector<pose_sample> &out) const
{
    auto anim = m_clip.get<animation>();
    sample_clip_into(anim.get(), compute_ticks(local_time), ctx.bound_bones(), out);
}

void animator_clip_state::fire_events(animator &ctx,
                                      real prev_local_time,
                                      real cur_local_time) const
{
    auto anim = m_clip.get<animation>();
    if (!anim)
        return;

    object *owner = ctx.get_object();
    if (!owner)
        return;

    real tps = anim->get_ticks_per_second();
    if (tps <= real(0))
        return;
    real full = anim->get_duration();
    real lo   = m_start_tick > real(0) ? m_start_tick : real(0);
    real hi  = (m_end_tick > real(0) && m_end_tick <= full) ? m_end_tick : full;
    real rng = hi - lo;
    if (rng <= real(0))
    {
        lo  = real(0);
        hi  = full;
        rng = full;
    }

    // Convert local_time (seconds) to slice-relative ticks.
    real prev_t = prev_local_time * tps;
    real cur_t  = cur_local_time * tps;

    auto fire_to_siblings = [&](const auto &key)
    {
        game_message msg;
        msg.msg_id    = key.name.c_str();
        msg.sender_id = ctx.id();
        msg.data      = value(string_view(key.args));
        for (const auto &c : owner->get_controllers())
        {
            if (c.get() != &ctx && c)
                c->on_message(msg);
        }
    };

    for (const auto &track : anim->get_event_tracks())
    {
        for (const auto &key : track.keys)
        {
            // key.timestamp is absolute ticks; convert to slice-relative.
            real ts = (real)key.timestamp - lo;
            if (ts < real(0) || ts > rng)
                continue;

            if (m_loop && rng > real(0))
            {
                real prev_mod = mod(prev_t, rng);
                real cur_mod  = mod(cur_t, rng);
                bool wrapped  = cur_mod < prev_mod;
                if (wrapped ? (ts > prev_mod || ts <= cur_mod)
                            : (ts > prev_mod && ts <= cur_mod))
                    fire_to_siblings(key);
            }
            else
            {
                if (ts > prev_t && ts <= cur_t)
                    fire_to_siblings(key);
            }
        }
    }
}

void animator_clip_state::save(serializer &s) const
{
    animator_state::save(s);
    s.write(m_clip);
    s.write((uint8_t)(m_loop ? 1 : 0));
    s.write((double)m_start_tick);
    s.write((double)m_end_tick);
}

void animator_clip_state::load(serializer &s, serializer_link *link)
{
    animator_state::load(s, link);
    s.read(m_clip);
    uint8_t b = 0;
    s.read(b);
    m_loop    = b != 0;
    double lo = 0.0, hi = -1.0;
    s.read(lo);
    s.read(hi);
    m_start_tick = (real)lo;
    m_end_tick   = (real)hi;
}

void animator_clip_state::save_xml(xml_serializer &s,
                                   tinyxml2::XMLElement &el) const
{
    animator_state::save_xml(s, el);
    xml_serializer::write_resource_ref(el, m_clip, "clip");
    el.SetAttribute("loop", m_loop);
    el.SetAttribute("start_tick", (double)m_start_tick);
    el.SetAttribute("end_tick", (double)m_end_tick);
}

void animator_clip_state::load_xml(xml_serializer &s, tinyxml2::XMLElement &el)
{
    animator_state::load_xml(s, el);
    xml_serializer::read_resource_ref(el, m_clip, "clip");
    bool l = true;
    el.QueryBoolAttribute("loop", &l);
    m_loop    = l;
    double lo = 0.0, hi = -1.0;
    el.QueryDoubleAttribute("start_tick", &lo);
    el.QueryDoubleAttribute("end_tick", &hi);
    m_start_tick = (real)lo;
    m_end_tick   = (real)hi;
}

} // namespace zabato
