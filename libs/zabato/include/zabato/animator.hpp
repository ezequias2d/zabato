#pragma once

#include <stddef.h>
#include <zabato/controller.hpp>
#include <zabato/error.hpp>
#include <zabato/ice.hpp>
#include <zabato/math.hpp>
#include <zabato/mesh.hpp>
#include <zabato/node.hpp>
#include <zabato/spatial.hpp>
#include <zabato/transformation.hpp>

namespace zabato
{
class animation;
class mesh;
class spatial;
class transformation;
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
class animation
{
public:
    static constexpr chunk_id CHUNK_ID = chunk_id("ANIM");

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

/**
 * @class animator
 * @brief A state machine that applies an animation to a model's skeleton over
 * time.
 */
class animator : public controller
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }

    /** @brief Constructs a new animator instance. */
    animator() : controller() {}
    /** @brief Destroys the animator. */
    virtual ~animator() {}

    /**
     * @brief Starts playing an animation clip on a scene graph hierarchy.
     * @param anim The animation clip to play.
     * @param root The root node of the scene graph to animate.
     * @param loop If true, the animation will loop when it reaches the end.
     */
    void play_animation(animation *anim, spatial *root, bool loop);

    /**
     * @brief Manually binds a specific bone name to a scene graph node.
     * @param bone_name The name of the bone in the animation.
     * @param node The scene graph node to control.
     */
    void bind_node(const char *bone_name, spatial *node);

    /**
     * @brief Binds an animation track to a property on a target controller.
     * @param track_name The name of the track in the animation.
     * @param target The target controller to modify.
     * @param prop_name The property name on the target controller.
     */
    void bind_property(const char *track_name,
                       controller *target,
                       const char *prop_name);

    void update(real delta_time) override;

private:
    struct bound_node
    {
        uint16_t channel_index;
        pointer<spatial> node;
    };

    struct bound_property
    {
        uint16_t track_index;
        pointer<controller> target;
        fixed_string<32> property_name;
    };

    vector<bound_node> m_bound_nodes = {};

    vector<bound_property> m_bound_reals   = {};
    vector<bound_property> m_bound_ints    = {};
    vector<bound_property> m_bound_bools   = {};
    vector<bound_property> m_bound_strings = {};
    vector<bound_property> m_bound_events  = {};

    animation *m_current_animation = nullptr;
    real m_current_time            = real(0);
    bool m_loop                    = false;
    animation_node m_root_node;
};

#pragma pack(push, 1)
struct ICE_ANIMATION_HEADER
{
    ice_real duration;
    ICE_R16 ticks_per_second;
    ice_uint16_t channel_count;
    ice_uint16_t node_count;
    ICE_MAT4X4_R16 global_inverse_transform;
};

struct ICE_ANIMATION_CHANNEL
{
    ice_int16_t node_id;
    ice_uint16_t position_count;
    ice_uint16_t rotation_count;
    ice_uint16_t scale_count;
};

struct ICE_NODE_HEADER
{
    ice_uint16_t name_len;
    ice_uint16_t children_count;
    ICE_VEC3_R16 position;
    ICE_VEC3_R16 scale;
    ICE_QUAT_R16 rotation;
};
#pragma pack(pop)

static inline void calculate_nodes_recursive(const animation_node &node,
                                             size_t &count,
                                             size_t &bytes)
{
    count += 1;
    size_t name_len = node.name.size() + 1;
    bytes += name_len += sizeof(ICE_NODE_HEADER);
    for (const auto &c : node.children)
        calculate_nodes_recursive(node, count, bytes);
}

static inline result<void> write_nodes_recursive(ice_writer &writer,
                                                 const animation_node &node)
{
    vec3<real> pos = {}, scale = {};
    quat<real> rot = {};
    mat4_decompose<real>(node.transform, pos, scale, rot);

    size_t name_len = node.name.size() + 1;
    assert(name_len <= UINT16_MAX);

    size_t childen_count = node.children.size();
    assert(childen_count <= UINT16_MAX);

    ICE_NODE_HEADER hdr = {
        .name_len       = (uint16_t)name_len,
        .children_count = (uint16_t)childen_count,
        .position       = pos,
        .scale          = scale,
        .rotation       = rot,
    };

    auto hres = writer.write(&hdr, sizeof(hdr));
    if (hres.has_error())
        return hres.error;

    const char *name = node.name.c_str();
    auto nres        = writer.write(name, hdr.name_len);
    if (nres.has_error())
        return nres.error;

    for (const auto &child : node.children)
    {
        auto rres = write_nodes_recursive(writer, child);
        if (rres.has_error())
            return rres.error;
    }

    return error_code::ok;
}

template <>
inline result<void> serialize(ice_writer &writer, const animation &a)
{
    TRACE_FUNCTION;

    size_t node_count = 0, node_bytes = 0;
    calculate_nodes_recursive(a.m_root_node, node_count, node_bytes);
    assert(node_count <= UINT16_MAX);

    size_t channel_count = a.m_channels.size();
    assert(channel_count <= UINT16_MAX);

    size_t channel_bytes = channel_count * sizeof(ICE_ANIMATION_CHANNEL);
    for (const auto &b : a.m_channels)
    {
        channel_bytes += b.track.positions.size() * sizeof(key_position);
        channel_bytes += b.track.rotations.size() * sizeof(key_rotation);
        channel_bytes += b.track.scales.size() * sizeof(key_scale);
    }

    const size_t total_size =
        sizeof(ICE_ANIMATION_HEADER) + node_bytes + channel_bytes;

    writer.write_chunk_header(animation::CHUNK_ID, total_size);

    // Header
    const ICE_ANIMATION_HEADER hdr = {
        .duration                 = a.m_duration,
        .ticks_per_second         = a.m_ticks_per_second,
        .channel_count            = (uint16_t)channel_count,
        .node_count               = (uint16_t)node_count,
        .global_inverse_transform = a.m_global_inverse_transform,
    };

    auto hres = writer.write(&hdr, sizeof(hdr));
    if (hres.has_error())
        return hres.error;

    // Nodes (preorder)
    auto nres = write_nodes_recursive(writer, a.m_root_node);
    if (nres.has_error())
        return nres.error;

    // Channels
    for (const auto &b : a.m_channels)
    {
        size_t position_count = b.track.positions.size();
        assert(position_count <= UINT16_MAX);

        size_t rotation_count = b.track.rotations.size();
        assert(rotation_count <= UINT16_MAX);

        size_t scale_count = b.track.scales.size();
        assert(scale_count <= UINT16_MAX);

        const ICE_ANIMATION_CHANNEL chdr = {
            .node_id        = b.bone_id,
            .position_count = (uint16_t)position_count,
            .rotation_count = (uint16_t)rotation_count,
            .scale_count    = (uint16_t)scale_count,
        };

        auto chres = writer.write(&chdr, sizeof(chdr));
        if (chres.has_error())
            return chres.error;

        if (chdr.position_count)
        {
            auto pres =
                writer.write(b.track.positions.data(),
                             chdr.position_count * sizeof(key_position));
            if (pres.has_error())
                return pres.error;
        }

        if (chdr.rotation_count)
        {
            auto pres =
                writer.write(b.track.rotations.data(),
                             chdr.rotation_count * sizeof(key_rotation));
            if (pres.has_error())
                return pres.error;
        }

        if (chdr.scale_count)
        {
            auto pres = writer.write(b.track.scales.data(),
                                     chdr.scale_count * sizeof(key_scale));
            if (pres.has_error())
                return pres.error;
        }
    }

    return error_code::ok;
}

static inline void import_nodes_recursive(ice_reader &reader,
                                          animation_node &parent_node,
                                          size_t &nodes_left)
{
    if (nodes_left == 0)
        return;

    ICE_NODE_HEADER header = {};
    size_t hsize           = reader.read(&header, sizeof(header));
    assert(hsize == sizeof(header));

    size_t name_len = min((uint32_t)header.name_len, (uint32_t)32);
    reader.read(&parent_node.name, name_len);

    nodes_left--;

    parent_node.bone      = nullptr;
    parent_node.transform = mat4_translation<real>(header.position) *
                            mat4_translation<real>(header.rotation) *
                            mat4_scaling<real>(header.scale);
    parent_node.children.resize(header.children_count);
    for (size_t i = 0; i < header.children_count; i++)
        import_nodes_recursive(reader, parent_node.children[i], nodes_left);
}

template <> inline result<void> deserialize(ice_reader &reader, animation &a)
{
    TRACE_FUNCTION;

    ICE_ANIMATION_HEADER header = {};
    size_t hsize                = reader.read(&header, sizeof(header));
    if (hsize != sizeof(header))
    {
        return report_error(error_code::chunk_broken,
                            animation::CHUNK_ID.to_string().c_str(),
                            (uint32_t)animation::CHUNK_ID);
    }

    a.set_duration(header.duration);
    a.set_ticks_per_second(header.ticks_per_second);
    a.set_global_inverse_transform(header.global_inverse_transform);

    // Import node hierarchy
    size_t nodes_to_process = header.node_count;
    import_nodes_recursive(reader, a.m_root_node, nodes_to_process);

    // Import channels
    a.m_channels.resize(header.channel_count);
    for (size_t i = 0; i < header.channel_count; ++i)
    {
        ICE_ANIMATION_CHANNEL chdr = {};
        reader.read(&chdr, sizeof(chdr));

        anim_bone &b = a.m_channels[i];

        b.track.positions.resize(chdr.position_count);
        reader.read(b.track.positions.data(),
                    chdr.position_count * sizeof(key_position));

        b.track.rotations.resize(chdr.rotation_count);
        reader.read(b.track.rotations.data(),
                    chdr.rotation_count * sizeof(key_rotation));

        b.track.scales.resize(chdr.scale_count);
        reader.read(b.track.scales.data(),
                    chdr.scale_count * sizeof(key_scale));
    }

    return error_code::ok;
}
} // namespace zabato