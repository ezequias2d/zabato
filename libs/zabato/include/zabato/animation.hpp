#pragma once

#include <zabato/ice.hpp>
#include <zabato/reflection.hpp>
#include <zabato/resource.hpp>

namespace zabato
{
struct bone_info;

struct key_position
{
    ICE_VEC3_R16 position;
    ICE_R16 timestamp;
};

struct key_rotation
{
    ICE_QUAT_R16 rotation;
    ICE_R16 timestamp;
};

struct key_scale
{
    ICE_VEC3_R16 scale;
    ICE_R16 timestamp;
};

struct animation_node
{
    ICE_MAT4X4_R16 transform;
    fixed_string<32> name;
    vector<animation_node> children;
    const bone_info *bone;
};

struct animation_track
{
    vector<key_position> positions;
    vector<key_rotation> rotations;
    vector<key_scale> scales;
    fixed_string<32> bone_name;

    static real get_scale_factor(real last_timestamp,
                                 real next_timestamp,
                                 real animation_time)
    {
        real midway_length = animation_time - last_timestamp;
        real frames_diff   = next_timestamp - last_timestamp;
        return frames_diff == real(0) ? real(0) : midway_length / frames_diff;
    }

    vec3<real> get_position(real animation_time)
    {
        if (positions.size() <= 1)
            return positions.empty() ? vec3<real>(0)
                                     : (vec3<real>)positions[0].position;

        size_t p0_index = 0;
        for (size_t i = 0; i < positions.size() - 1; ++i)
        {
            if (animation_time < positions[i + 1].timestamp)
            {
                p0_index = i;
                break;
            }
        }

        size_t p1_index = p0_index + 1;
        real factor     = get_scale_factor(positions[p0_index].timestamp,
                                       positions[p1_index].timestamp,
                                       animation_time);
        return lerp<vec3<real>>(
            positions[p0_index].position, positions[p1_index].position, factor);
    }

    mat4<real> interpolate_position(real animation_time)
    {
        return mat4_translation(get_position(animation_time));
    }

    quat<real> get_rotation(real animation_time)
    {
        if (rotations.size() <= 1)
            return normalize(rotations.empty()
                                 ? quat<real>()
                                 : (quat<real>)rotations[0].rotation);

        size_t r0_index = 0;
        for (size_t i = 0; i < rotations.size() - 1; ++i)
        {
            if (animation_time < rotations[i + 1].timestamp)
            {
                r0_index = i;
                break;
            }
        }

        size_t r1_index = r0_index + 1;
        real factor     = get_scale_factor(rotations[r0_index].timestamp,
                                       rotations[r1_index].timestamp,
                                       animation_time);
        return slerp<real>(
            rotations[r0_index].rotation, rotations[r1_index].rotation, factor);
    }

    mat4<real> interpolate_rotation(real animation_time)
    {
        return mat4_from_quat(get_rotation(animation_time));
    }

    vec3<real> get_scale(real animation_time)
    {
        if (scales.size() <= 1)
            return scales.empty() ? vec3<real>(1) : (vec3<real>)scales[0].scale;

        size_t s0_index = 0;
        for (size_t i = 0; i < scales.size() - 1; ++i)
        {
            if (animation_time < scales[i + 1].timestamp)
            {
                s0_index = i;
                break;
            }
        }
        size_t s1_index = s0_index + 1;
        real factor     = get_scale_factor(scales[s0_index].timestamp,
                                       scales[s1_index].timestamp,
                                       animation_time);
        return lerp<vec3<real>>(
            scales[s0_index].scale, scales[s1_index].scale, factor);
    }

    mat4<real> interpolate_scaling(real animation_time)
    {
        return mat4_scaling(get_scale(animation_time));
    }
};

struct key_real
{
    ICE_R16 value;
    ICE_R16 timestamp;
};

struct key_int
{
    ice_int64_t value;
    ICE_R16 timestamp;
};

struct key_bool
{
    ice_int8_t value;
    ICE_R16 timestamp;
};

struct key_string
{
    fixed_string<32> value;
    ICE_R16 timestamp;
};

struct key_event
{
    fixed_string<32> name;
    fixed_string<32> args;
    ICE_R16 timestamp;
};

struct real_track
{
    vector<key_real> keys;
    fixed_string<32> property_name;

    real get_value(real animation_time)
    {
        if (keys.empty())
            return real(0);
        if (keys.size() == 1)
            return (real)keys[0].value;

        size_t p0_index = 0;
        for (size_t i = 0; i < keys.size() - 1; ++i)
        {
            if (animation_time < keys[i + 1].timestamp)
            {
                p0_index = i;
                break;
            }
        }

        size_t p1_index = p0_index + 1;
        real factor     = animation_track::get_scale_factor(
            keys[p0_index].timestamp, keys[p1_index].timestamp, animation_time);
        return lerp<real>(keys[p0_index].value, keys[p1_index].value, factor);
    }
};

struct int_track
{
    vector<key_int> keys;
    fixed_string<32> property_name;

    int64_t get_value(real animation_time)
    {
        if (keys.empty())
            return 0;

        for (size_t i = 0; i < keys.size() - 1; ++i)
        {
            if (animation_time < keys[i + 1].timestamp)
                return keys[i].value;
        }
        return keys.back().value;
    }
};

struct bool_track
{
    vector<key_bool> keys;
    fixed_string<32> property_name;

    bool get_value(real animation_time)
    {
        if (keys.empty())
            return false;

        for (size_t i = 0; i < keys.size() - 1; ++i)
        {
            if (animation_time < keys[i + 1].timestamp)
                return keys[i].value != 0;
        }
        return keys.back().value != 0;
    }
};

struct string_track
{
    vector<key_string> keys;
    fixed_string<32> property_name;

    const char *get_value(real animation_time)
    {
        if (keys.empty())
            return "";

        for (size_t i = 0; i < keys.size() - 1; ++i)
        {
            if (animation_time < keys[i + 1].timestamp)
                return keys[i].value.c_str();
        }
        return keys.back().value.c_str();
    }
};

struct event_track
{
    vector<key_event> keys;
    fixed_string<32> property_name;
};

struct anim_bone
{
    animation_track track;
    int16_t bone_id;
    mat4<real> local_transform;
    mat4<real> offset_transform;

    void update(real animation_time)
    {
        mat4<real> translation = track.interpolate_position(animation_time);
        mat4<real> rotation    = track.interpolate_rotation(animation_time);
        mat4<real> scale       = track.interpolate_scaling(animation_time);
        local_transform        = translation * rotation * scale;
    }
};

/**
 * @class animation
 * @brief An animation clip resource, containing keyframe data for a skeleton.
 */
class animation : public resource
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }

    static constexpr chunk_id CHUNK_ID = chunk_id("ANIM");

    static void reflect(reflection &r);

    /** @brief Constructs a new, empty animation object. */
    animation() {}
    /** @brief Destroys the animation and releases all its keyframe data. */
    ~animation() {}

    /** @return The total duration of the animation in ticks. */
    real get_duration() const { return m_duration; }

    /** @return The number of ticks that occur per second. */
    real get_ticks_per_second() const { return m_ticks_per_second; }

    void set_duration(real d) { m_duration = d; }

    void set_ticks_per_second(real tps) { m_ticks_per_second = tps; }

    void set_tracks(const vector<animation_track> &tracks)
    {
        m_channels.resize(tracks.size());
        for (size_t i = 0; i < tracks.size(); ++i)
        {
            auto &bone  = m_channels[i];
            auto &track = tracks[i];
            bone.track  = track;
        }
    }

    void set_global_inverse_transform(const mat4<real> &transform)
    {
        m_global_inverse_transform = transform;
    }

    vector<anim_bone> &get_bones() { return m_channels; }
    const vector<anim_bone> &get_bones() const { return m_channels; }

    vector<real_track> &get_real_tracks() { return m_real_tracks; }
    vector<int_track> &get_int_tracks() { return m_int_tracks; }
    vector<bool_track> &get_bool_tracks() { return m_bool_tracks; }
    vector<string_track> &get_string_tracks() { return m_string_tracks; }
    vector<event_track> &get_event_tracks() { return m_event_tracks; }

    const mat4<real> &get_global_inverse_transform() const
    {
        return m_global_inverse_transform;
    }

    anim_bone *find_bone(const char *name)
    {
        for (size_t i = 0; i < m_channels.size(); ++i)
        {
            auto &b = m_channels[i];
            if (b.track.bone_name == name)
                return &b;
        }
        return nullptr;
    }

    anim_bone *find_bone_by_id(int16_t id)
    {
        for (auto &b : m_channels)
            if (b.bone_id == id)
                return &b;
        return nullptr;
    }

    ptrdiff_t find_bone_index(const char *name)
    {
        for (size_t i = 0; i < m_channels.size(); ++i)
        {
            auto &b = m_channels[i];
            if (b.track.bone_name == name)
                return i;
        }
        return -1;
    }

private:
    template <typename T>
    friend result<void> serialize(ice_writer &writer, const T &a);

    template <typename T>
    friend result<void> deserialize(ice_reader &reader, T &m);

    real m_duration         = real(0);
    real m_ticks_per_second = real(25);
    animation_node m_root_node;
    vector<anim_bone> m_channels;

    vector<real_track> m_real_tracks;
    vector<int_track> m_int_tracks;
    vector<bool_track> m_bool_tracks;
    vector<string_track> m_string_tracks;
    vector<event_track> m_event_tracks;

    mat4<real> m_global_inverse_transform = mat4<real>::identity();
};
} // namespace zabato