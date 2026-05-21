#pragma once

#include <zabato/animation.hpp>
#include <zabato/animator_graph.hpp>
#include <zabato/controller.hpp>
#include <zabato/error.hpp>
#include <zabato/ice.hpp>
#include <zabato/math.hpp>
#include <zabato/node.hpp>
#include <zabato/reflection.hpp>
#include <zabato/resource.hpp>
#include <zabato/spatial.hpp>
#include <zabato/symbol.hpp>
#include <zabato/transformation.hpp>

namespace zabato
{
class animation;
class spatial;
class transformation;
class animator_graph;
class animator_state;
class animator_transition;
class animator_clip_state;
struct pose_sample;

/** A bone bound by the animator into the scene graph by name. */
struct animator_bound_bone
{
    symbol_ref name;
    pointer<spatial> node;
};

/**
 * @class animator
 * @brief Graph-based state machine that applies skeletal animation and drives
 * non-spatial controller properties from animation curves.
 */
class animator : public controller
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    animator() : controller() {}
    virtual ~animator() {}

    void start() override {}

    /**
     * @brief Bind an animation track by name to a property on a target
     * controller.
     *
     * Resolved lazily each frame against the active graph clip state.
     * Works for real, int, bool, and string value tracks (sampled continuously)
     * and event tracks (fired via on_message when the timestamp is crossed).
     */
    void bind_property(const char *track_name,
                       controller *target,
                       const char *prop_name);

    void update(real delta_time) override;
    void on_message(const game_message &msg) override;

    /** @brief Load and begin playing an animator_graph from a resource. */
    void play_graph(const resource_ref &graph_ref, spatial *root);

    /** @brief Bind an in-memory graph directly (programmatic setup / editor
     *  preview; bypasses resource loading). */
    void set_graph_direct(pointer<animator_graph> g, spatial *root);

    /** @brief Set the graph resource reference without forcing an immediate
     *  reload (reload happens on next update). */
    void set_graph(const resource_ref &graph_ref)
    {
        m_graph_ref   = graph_ref;
        m_graph_dirty = true;
    }

    const resource_ref &get_graph() const { return m_graph_ref; }
    size_t current_state_index() const { return m_current_state; }
    const vector<animator_bound_bone> &bound_bones() const
    {
        return m_bound_bones;
    }
    animator_graph *graph() const { return m_graph; }

private:
    struct bound_property
    {
        symbol_ref track_name;
        pointer<controller> target;
        fixed_string<32> property_name;
    };

    vector<bound_property> m_bound_properties = {};

    resource_ref m_graph_ref        = {};
    pointer<animator_graph> m_graph = nullptr;
    bool m_graph_dirty              = false;

    vector<animator_bound_bone> m_bound_bones = {};
    vector<symbol_ref> m_pending_messages     = {};

    size_t m_current_state = (size_t)-1;
    real m_state_time      = real(0);
    real m_prev_state_time = real(0);

    bool m_transitioning  = false;
    size_t m_next_state   = (size_t)-1;
    real m_blend_time     = real(0);
    real m_blend_duration = real(0);

    void ensure_graph_loaded();
    void apply_pose(const vector<pose_sample> &pose);
    void blend_apply_pose(const vector<pose_sample> &a,
                          const vector<pose_sample> &b,
                          real t);
    void rebind_skeleton_bones(spatial *root);
    void apply_bound_properties(const animator_clip_state &cs,
                                real prev_st,
                                real cur_st);
    bool eval_transition(animator_state *state, const animator_transition &tr);
    void begin_transition(size_t next_idx, real duration);
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
        calculate_nodes_recursive(c, count, bytes);
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
                            mat4_from_quat<real>(header.rotation) *
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
