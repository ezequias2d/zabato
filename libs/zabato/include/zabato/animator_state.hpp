#pragma once

#include <stdint.h>
#include <zabato/animator.hpp>
#include <zabato/math.hpp>
#include <zabato/object.hpp>
#include <zabato/real.hpp>
#include <zabato/resource.hpp>
#include <zabato/symbol.hpp>
#include <zabato/transformation.hpp>
#include <zabato/vector.hpp>

namespace zabato
{

class animator;

struct pose_sample
{
    transformation xform;
    bool sampled = false;
};

/**
 * @struct animator_transition
 * @brief A directed edge between two animator states.
 *
 * Fires when (if has_exit_time) the source state has reached exit_time_norm
 * fraction of its duration AND (if message_trigger is set) that message was
 * received this tick. If neither gate is set, fires every frame.
 * Blend duration controls the cross-fade length in seconds.
 */
struct animator_transition
{
    size_t dst_state_index = 0;
    symbol_ref message_trigger;
    real exit_time_norm = real(1);
    real duration       = real(0);
    bool has_exit_time  = false;
    bool interruptible  = true;
};

/**
 * @class animator_state
 * @brief Abstract node in an animator state graph.
 *
 * Carries a name, outgoing transitions, and a virtual sample() that populates a
 * pose buffer aligned with the owning animator's bound node list.
 */
class animator_state : public object
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    animator_state()           = default;
    ~animator_state() override = default;

    const symbol_ref &state_name() const { return m_name; }
    void set_state_name(const symbol_ref &n) { m_name = n; }

    vector<animator_transition> &transitions() { return m_transitions; }
    const vector<animator_transition> &transitions() const
    {
        return m_transitions;
    }

    /** @return Duration of the state in seconds (not animation ticks). 0 if
     *  unknown / untimed. */
    virtual real state_duration(animator &ctx) const = 0;

    /** @brief Fill pose buffer with bone transformations for the given local
     *  time (seconds since entering this state). Unsampled bones are left
     *  untouched (sampled=false). */
    virtual void
    sample(animator &ctx, real local_time, vector<pose_sample> &out) const = 0;

    /** @brief Fire game_message events whose timestamps fall in the window
     *  (prev_local_time, cur_local_time]. Events are dispatched to all
     *  sibling controllers on the owner object. */
    virtual void
    fire_events(animator &ctx, real prev_local_time, real cur_local_time) const
    {
    }

    void save(class serializer &s) const override;
    void load(class serializer &s, class serializer_link *link) override;
    void save_xml(class xml_serializer &s,
                  tinyxml2::XMLElement &el) const override;
    void load_xml(class xml_serializer &s, tinyxml2::XMLElement &el) override;

protected:
    symbol_ref m_name;
    vector<animator_transition> m_transitions;
};

/** Single-clip state. Samples one animation resource. */
class animator_clip_state : public animator_state
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    animator_clip_state()           = default;
    ~animator_clip_state() override = default;

    const resource_ref &clip() const { return m_clip; }
    void set_clip(const resource_ref &r) { m_clip = r; }

    bool loop() const { return m_loop; }
    void set_loop(bool v) { m_loop = v; }

    /** @return Start of the active slice, in animation ticks. */
    real start_tick() const { return m_start_tick; }
    void set_start_tick(real t) { m_start_tick = t; }

    /** @return End of the active slice, in animation ticks. A negative value
     *  means "use the clip's full duration". */
    real end_tick() const { return m_end_tick; }
    void set_end_tick(real t) { m_end_tick = t; }

    real state_duration(animator &ctx) const override;
    void sample(animator &ctx,
                real local_time,
                vector<pose_sample> &out) const override;
    void fire_events(animator &ctx,
                     real prev_local_time,
                     real cur_local_time) const override;

    /** @brief Convert local time (seconds) to absolute animation ticks,
     *  accounting for start/end slice and loop wrapping. */
    real compute_ticks(real local_time) const;

    void save(class serializer &s) const override;
    void load(class serializer &s, class serializer_link *link) override;
    void save_xml(class xml_serializer &s,
                  tinyxml2::XMLElement &el) const override;
    void load_xml(class xml_serializer &s, tinyxml2::XMLElement &el) override;

private:
    resource_ref m_clip;
    bool m_loop       = true;
    real m_start_tick = real(0);
    real m_end_tick   = real(-1);
};

} // namespace zabato
