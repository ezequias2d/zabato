#pragma once

#include <zabato/allocator.hpp>
#include <zabato/spatial.hpp>

#include <zabato/color.hpp>
#include <zabato/error.hpp>
#include <zabato/gpu.hpp>
#include <zabato/ice.hpp>
#include <zabato/math.hpp>
#include <zabato/real.hpp>
#include <zabato/resource.hpp>
#include <zabato/vector.hpp>

#include <assert.h>

namespace zabato
{
class mesh;

/**
 * @enum mesh_flags
 * @brief Bitflags describing the data present in a model's vertices.
 */
enum class mesh_flags : uint8_t
{
    none   = 0,      ///< No flags set.
    normal = 1 << 0, ///< Vertex has a normal vector.
    color  = 1 << 1, ///< Vertex has a color attribute.
    tex    = 1 << 2, ///< Vertex has a texture coordinate.
    bone   = 1 << 3, ///< Vertex has bone weights and indices.
};

inline mesh_flags operator|(mesh_flags a, mesh_flags b)
{
    return static_cast<mesh_flags>(static_cast<uint8_t>(a) |
                                   static_cast<uint8_t>(b));
}
inline mesh_flags operator&(mesh_flags a, mesh_flags b)
{
    return static_cast<mesh_flags>(static_cast<uint8_t>(a) &
                                   static_cast<uint8_t>(b));
}

struct quad_primitive
{
    union
    {
        struct
        {
            uint16_t v0, v1, v2, v3;
        };
        uint16_t v[4];
    };
};

struct triangle_primitive
{
    union
    {
        struct
        {
            uint16_t v0, v1, v2;
        };
        uint16_t v[3];
    };
};

struct point_primitive
{
    uint16_t v;
};

struct line_primitive
{
    union
    {
        struct
        {
            uint16_t v0, v1;
        };
        uint16_t v[2];
    };
};

/**
 * @struct bone_info
 * @brief Contains mapping information for a single bone in a model's skeleton.
 */
struct bone_info
{
    fixed_string<32> name;
    int16_t bone_id;
    ICE_MAT4X4_R16 offset_transform;
};

struct bone_weight
{
    int16_t bone_id = -1;
    ICE_R16 weight  = real(0);
};

using position_t   = vec3<real>;
using normal_t     = vec3<real>;
using color_t      = color;
using texcoord_t   = vec2<real>;
using boneweight_t = bone_weight[4];

class mesh : public resource
{
public:
    static constexpr chunk_id CHUNK_ID = chunk_id("MESH");

    inline mesh() : m_vertex_count(0) {};
    inline ~mesh() {};

    void init(mesh_flags flags, primitive_type type)
    {
        m_flags = flags;
        m_type  = type;
        calculate_offsets();
        resize();
    }

    constexpr size_t get_index_count_per_primitive() const
    {
        switch (m_type)
        {
        case primitive_type::quads:
            return 4;
        case primitive_type::triangles:
            return 3;
        case primitive_type::lines:
            return 2;
        case primitive_type::points:
            return 1;
        default:
            return 0;
        }
    }

    constexpr primitive_type get_primitive_type() const { return m_type; }
    constexpr mesh_flags get_flags() const { return m_flags; }

    /** @return The total number of vertices in the model. */
    uint16_t get_vertex_count() const { return m_vertex_count; }

    uint16_t set_vertex_count(uint16_t count)
    {
        m_vertex_count = count;
        m_data.resize(count * m_vertex_size);
        return m_vertex_count;
    }

    uint16_t get_primitive_count() const { return m_primitive_count; }
    uint16_t set_primitive_count(uint16_t count)
    {
        m_primitive_count = count;
        m_indices.resize(count * get_index_count_per_primitive());
        return m_primitive_count;
    }

    void get_primitive(uint16_t index, point_primitive &prim) const
    {
        assert(m_type == primitive_type::points);
        assert(index < m_primitive_count);
        if (index >= m_primitive_count)
            return;

        const uint16_t *index_ptr =
            m_indices.data() + index * get_index_count_per_primitive();
        memcpy(&prim, index_ptr, sizeof(point_primitive));
    }

    void set_primitive(uint16_t index, const point_primitive &prim)
    {
        assert(m_type == primitive_type::points);
        assert(index < m_primitive_count);
        if (index >= m_primitive_count)
            return;

        uint16_t *index_ptr =
            m_indices.data() + index * get_index_count_per_primitive();
        memcpy(index_ptr, &prim, sizeof(point_primitive));
    }

    void get_primitive(uint16_t index, line_primitive &prim) const
    {
        assert(m_type == primitive_type::lines);
        assert(index < m_primitive_count);
        if (index >= m_primitive_count)
            return;

        const uint16_t *index_ptr =
            m_indices.data() + index * get_index_count_per_primitive();
        memcpy(&prim, index_ptr, sizeof(line_primitive));
    }

    void set_primitive(uint16_t index, const line_primitive &prim)
    {
        assert(m_type == primitive_type::lines);
        assert(index < m_primitive_count);
        if (index >= m_primitive_count)
            return;

        uint16_t *index_ptr =
            m_indices.data() + index * get_index_count_per_primitive();
        memcpy(index_ptr, &prim, sizeof(line_primitive));
    }

    void get_primitive(uint16_t index, triangle_primitive &prim) const
    {
        assert(m_type == primitive_type::triangles);
        assert(index < m_primitive_count);
        if (index >= m_primitive_count)
            return;

        const uint16_t *index_ptr =
            m_indices.data() + index * get_index_count_per_primitive();
        prim.v0 = index_ptr[0];
        prim.v1 = index_ptr[1];
        prim.v2 = index_ptr[2];
    }

    void set_primitive(uint16_t index, const triangle_primitive &prim)
    {
        assert(m_type == primitive_type::triangles);
        assert(index < m_primitive_count);
        if (index >= m_primitive_count)
            return;

        uint16_t *index_ptr =
            m_indices.data() + index * get_index_count_per_primitive();
        index_ptr[0] = prim.v0;
        index_ptr[1] = prim.v1;
        index_ptr[2] = prim.v2;
    }

    void get_primitive(uint16_t index, quad_primitive &prim) const
    {
        assert(m_type == primitive_type::quads);
        assert(index < m_primitive_count);
        if (index >= m_primitive_count)
            return;

        const uint16_t *index_ptr =
            m_indices.data() + index * get_index_count_per_primitive();
        prim.v0 = index_ptr[0];
        prim.v1 = index_ptr[1];
        prim.v2 = index_ptr[2];
        prim.v3 = index_ptr[3];
    }

    void set_primitive(uint16_t index, const quad_primitive &prim)
    {
        assert(m_type == primitive_type::quads);
        assert(index < m_primitive_count);
        if (index >= m_primitive_count)
            return;

        uint16_t *index_ptr =
            m_indices.data() + index * get_index_count_per_primitive();
        index_ptr[0] = prim.v0;
        index_ptr[1] = prim.v1;
        index_ptr[2] = prim.v2;
        index_ptr[3] = prim.v3;
    }

    uint16_t get_bone_count() const { return m_bone_infos.size(); }
    uint16_t set_bone_count(uint16_t count)
    {
        m_bone_infos.resize(count);
        return m_bone_infos.size();
    }

    void set_bone(uint16_t index, const bone_info &bone)
    {
        assert(index < m_bone_infos.size());
        if (index >= m_bone_infos.size())
            return;
        m_bone_infos[index] = bone;
    }

    void get_bone(uint16_t index, bone_info &bone) const
    {
        assert(index < m_bone_infos.size());
        if (index >= m_bone_infos.size())
            return;
        bone = m_bone_infos[index];
    }

    /**
     * @brief Sets the color for a specific vertex. This requires the model to
     * have the `color` flag.
     * @param index The index of the vertex to modify.
     * @param c The new color for the vertex.
     */
    void set_color(uint16_t index, const color &c)
    {
        assert((m_flags & mesh_flags::color) != mesh_flags::none);
        uint8_t *vertex_ptr = get_vertex_ptr(index);
        if (!vertex_ptr)
            return;
        vertex_ptr += m_color_offset;
        memcpy(vertex_ptr, &c, sizeof(color_t));
    }

    void get_color(uint16_t index, color &c) const
    {
        assert((m_flags & mesh_flags::color) != mesh_flags::none);
        const uint8_t *vertex_ptr = get_vertex_ptr(index);
        if (!vertex_ptr)
            return;
        vertex_ptr += m_color_offset;
        memcpy(&c, vertex_ptr, sizeof(color_t));
    }

    void set_position(uint16_t index, const vec3<real> &pos)
    {
        uint8_t *vertex_ptr = get_vertex_ptr(index);
        if (!vertex_ptr)
            return;
        memcpy(vertex_ptr, &pos, sizeof(position_t));
    }

    void get_position(uint16_t index, vec3<real> &pos) const
    {
        const uint8_t *vertex_ptr = get_vertex_ptr(index);
        if (!vertex_ptr)
            return;
        memcpy(&pos, vertex_ptr, sizeof(position_t));
    }

    void set_normal(uint16_t index, const vec3<real> &norm)
    {
        assert((m_flags & mesh_flags::normal) != mesh_flags::none);
        uint8_t *vertex_ptr = get_vertex_ptr(index);
        if (!vertex_ptr)
            return;
        vertex_ptr += m_normal_offset;
        memcpy(vertex_ptr, &norm, sizeof(normal_t));
    }

    void get_normal(uint16_t index, vec3<real> &norm) const
    {
        assert((m_flags & mesh_flags::normal) != mesh_flags::none);
        const uint8_t *vertex_ptr = get_vertex_ptr(index);
        if (!vertex_ptr)
            return;
        vertex_ptr += m_normal_offset;
        memcpy(&norm, vertex_ptr, sizeof(normal_t));
    }

    void set_texcoord(uint16_t index, const texcoord_t &tex)
    {
        assert((m_flags & mesh_flags::tex) != mesh_flags::none);
        uint8_t *vertex_ptr = get_vertex_ptr(index);
        if (!vertex_ptr)
            return;
        vertex_ptr += m_texcoord_offset;
        memcpy(vertex_ptr, &tex, sizeof(texcoord_t));
    }

    void get_texcoord(uint16_t index, texcoord_t &tex) const
    {
        assert((m_flags & mesh_flags::tex) != mesh_flags::none);
        const uint8_t *vertex_ptr = get_vertex_ptr(index);
        if (!vertex_ptr)
            return;
        vertex_ptr += m_texcoord_offset;
        memcpy(&tex, vertex_ptr, sizeof(texcoord_t));
    }

    void set_boneweight(uint16_t index, const boneweight_t &bones)
    {
        assert((m_flags & mesh_flags::bone) != mesh_flags::none);
        uint8_t *vertex_ptr = get_vertex_ptr(index);
        if (!vertex_ptr)
            return;
        vertex_ptr += m_boneweight_offset;
        memcpy(vertex_ptr, &bones, sizeof(boneweight_t));
    }

    void get_boneweight(uint16_t index, boneweight_t &bones) const
    {
        assert((m_flags & mesh_flags::bone) != mesh_flags::none);
        const uint8_t *vertex_ptr = get_vertex_ptr(index);
        if (!vertex_ptr)
            return;
        vertex_ptr += m_boneweight_offset;
        memcpy(&bones, vertex_ptr, sizeof(boneweight_t));
    }

    const bone_info *find_bone_info(const char *name) const
    {
        for (const auto &bone : m_bone_infos)
            if (bone.name == name)
                return &bone;
        return nullptr;
    }

    const vector<bone_info> &get_bones() { return m_bone_infos; }

    /**
     * @brief Renders the model using a given GPU context.
     * @param gpu The GPU interface to use for drawing commands.
     * @param bones Optional list of bone nodes for skeletal animation.
     *              If provided, it must match the mesh's bone count and order.
     */
    void render(gpu &gpu, const vector<spatial *> &bones = {}) const;

private:
    vector<uint8_t> m_data;
    vector<uint16_t> m_indices;
    vector<bone_info> m_bone_infos;

    mesh_flags m_flags;
    primitive_type m_type;

    uint16_t m_vertex_count;
    uint16_t m_primitive_count;

    size_t m_vertex_size;
    size_t m_normal_offset;
    size_t m_color_offset;
    size_t m_texcoord_offset;
    size_t m_boneweight_offset;

    uint8_t *get_vertex_ptr(uint16_t index)
    {
        assert(index < m_vertex_count);
        assert(m_data.size() >= (index + 1) * m_vertex_size);

        if (index >= m_vertex_count)
            return nullptr;
        return m_data.data() + index * m_vertex_size;
    }

    const uint8_t *get_vertex_ptr(uint16_t index) const
    {
        assert(index < m_vertex_count);
        assert(m_data.size() >= (index + 1) * m_vertex_size);

        if (index >= m_vertex_count)
            return nullptr;
        return m_data.data() + index * m_vertex_size;
    }

    inline void calculate_offsets()
    {
        const mesh_flags flags = m_flags;
        const bool has_normal =
            (flags & mesh_flags::normal) != mesh_flags::none;
        const bool has_color = (flags & mesh_flags::color) != mesh_flags::none;
        const bool has_tex   = (flags & mesh_flags::tex) != mesh_flags::none;
        const bool has_bone  = (flags & mesh_flags::bone) != mesh_flags::none;

        m_vertex_size = sizeof(position_t) + (has_normal * sizeof(normal_t)) +
                        (has_color * sizeof(color_t)) +
                        (has_tex * sizeof(texcoord_t)) +
                        (has_bone * sizeof(boneweight_t));
        m_normal_offset   = 0 + has_normal * sizeof(normal_t);
        m_color_offset    = m_normal_offset + has_color * sizeof(color_t);
        m_texcoord_offset = m_color_offset + has_color * sizeof(texcoord_t);
        m_boneweight_offset =
            m_texcoord_offset + has_bone * sizeof(boneweight_t);
    }

    inline void resize()
    {
        m_data.resize(m_vertex_count * m_vertex_size);
        m_indices.resize(m_primitive_count * get_index_count_per_primitive());
    }

    inline void vertex(bool has_normal,
                       bool has_color,
                       bool has_tex,
                       bool has_bone,
                       gpu &gpu,
                       size_t index,
                       const vector<mat4<real>> *final_bone_matrices) const
    {
        if (has_normal)
        {
            vec3<real> normal = {};
            get_normal(index, normal);
            gpu.normal(normal);
        }

        if (has_color)
        {
            color c = {};
            get_color(index, c);
            gpu.color(c);
        }

        if (has_tex)
        {
            vec2<real> uv = {};
            get_texcoord(index, uv);
            gpu.tex_coord(uv);
        }

        vec3<real> pos = {};
        get_position(index, pos);

        if (has_bone && final_bone_matrices)
        {
            boneweight_t bone_weights;
            get_boneweight(index, bone_weights);

            size_t bone_count = 0;

            vec3<real> final_position = {0};
            for (auto j = 0; j < 4; ++j)
            {
                const auto &bw = bone_weights[j];
                if (bw.bone_id >= 0 && bw.bone_id < bone_count &&
                    bw.weight > real(0))
                {
                    vec4<real> pos4(pos, 1);
                    vec4<real> transformed_pos =
                        final_bone_matrices->operator[](bw.bone_id) * pos4;

                    final_position += transformed_pos.xyz() * bw.weight;
                }
            }
        }
        else
            gpu.vertex(pos);
    }

    template <typename T>
    friend result<void> serialize(ice_writer &writer, const T &m);

    template <typename T>
    friend result<void> deserialize(ice_reader &reader, T &m);
};

struct ICE_MESH_HEADER
{
    ice_uint8_t flags_and_type;
    ice_uint16_t boneCount;
    ice_uint16_t vertexCount;
    ice_uint16_t primitiveCount;

    mesh_flags flags() const
    {
        return static_cast<mesh_flags>((uint8_t)flags_and_type & 0x0F);
    }

    primitive_type type() const
    {
        return static_cast<primitive_type>(((uint8_t)flags_and_type >> 6) &
                                           0x03);
    }

    bool is_index_8bit() const { return ((uint8_t)flags_and_type & 0x10) != 0; }
};

struct ICE_BONE_INFO_HEADER
{
    ICE_MAT4X4_R16 offset;
    ice_uint8_t nameLen;
};

template <> inline result<void> serialize(ice_writer &writer, const mesh &m)
{
    TRACE_FUNCTION;

    const bool use_8bit_indices             = m.get_vertex_count() <= 256;
    const uint8_t index_count_per_primitive = m.get_index_count_per_primitive();

    size_t total_size = sizeof(ICE_MESH_HEADER);
    total_size += m.m_vertex_size * m.get_vertex_count();
    total_size += m.get_primitive_count() * index_count_per_primitive *
                  (use_8bit_indices ? 1 : 2);

    writer.write_chunk_header(mesh::CHUNK_ID, total_size);

    ICE_MESH_HEADER header = {
        .flags_and_type = static_cast<uint8_t>(
            static_cast<uint8_t>(m.m_flags) | (use_8bit_indices ? 0x10 : 0) |
            (static_cast<uint8_t>(m.m_type) << 6)),
        .boneCount      = m.get_bone_count(),
        .vertexCount    = m.get_vertex_count(),
        .primitiveCount = m.get_primitive_count(),
    };

    writer.write(&header, sizeof(header));

    // Write bone info
    for (const auto &bone_info : m.m_bone_infos)
    {
        ICE_BONE_INFO_HEADER bone_header = {};
        bone_header.offset               = bone_info.offset_transform;
        bone_header.nameLen              = bone_info.name.size();

        writer.write(&bone_header, sizeof(bone_header));
        writer.write(bone_info.name.c_str(), bone_header.nameLen);
    }

    const bool has_normal =
        (m.m_flags & mesh_flags::normal) != mesh_flags::none;
    const bool has_color = (m.m_flags & mesh_flags::color) != mesh_flags::none;
    const bool has_tex   = (m.m_flags & mesh_flags::tex) != mesh_flags::none;
    const bool has_bone  = (m.m_flags & mesh_flags::bone) != mesh_flags::none;

    // Write vertices
    for (uint16_t i = 0; i < m.get_vertex_count(); ++i)
    {
        position_t pos;
        m.get_position(i, pos);
        ICE_VEC3_R16 p = pos;
        writer.write(&p, sizeof(p));

        if (has_normal)
        {
            normal_t norm;
            m.get_normal(i, norm);
            ICE_NORM3 n = norm;
            writer.write(&n, sizeof(n));
        }

        if (has_color)
        {
            color_t c;
            m.get_color(i, c);
            color5551 ic{c};
            writer.write(&ic.value, sizeof(ic.value));
        }

        if (has_tex)
        {
            texcoord_t uv;
            m.get_texcoord(i, uv);
            ICE_VEC2_U16 iuv = ICE_VEC2_U16(uv);
            writer.write(&iuv, sizeof(iuv));
        }

        if (has_bone)
        {
            boneweight_t bones = {0};
            // Get actual bone weights
            m.get_boneweight(i, bones);

            ICE_VEC2<int16_t, ICE_R16> ibones[4];
            for (int j = 0; j < 4; ++j)
            {
                ibones[j].x = bones[j].bone_id;
                ibones[j].y = bones[j].weight;
            }
            writer.write(ibones, sizeof(ibones));
        }
    }

    const size_t primitive_size  = m.get_index_count_per_primitive();
    const size_t primitive_count = m.get_primitive_count();

    // Write indices
    if (use_8bit_indices)
    {
        uint8_t primitive[4];
        for (uint16_t i = 0; i < primitive_count; ++i)
        {
            for (size_t j = 0; j < primitive_size; ++j)
            {
                primitive[j] =
                    static_cast<uint8_t>(m.m_indices[i * primitive_size + j]);
            }
            writer.write(&primitive[0], primitive_size);
        }
    }
    else
    {
        writer.write(m.m_indices.data(),
                     m.get_primitive_count() * primitive_size *
                         sizeof(uint16_t));
    }

    return error_code::ok;
}

template <> inline result<void> deserialize(ice_reader &reader, mesh &m)
{
    TRACE_FUNCTION;

    ICE_MESH_HEADER header;
    reader.read(&header, sizeof(header));

    const bool use_8bit_indices = header.is_index_8bit();

    m.set_vertex_count(header.vertexCount);
    m.set_primitive_count(header.primitiveCount);
    m.set_bone_count(header.boneCount);

    m.init(header.flags(), header.type());

    // Read bone info
    for (uint16_t i = 0; i < header.boneCount; ++i)
    {
        ICE_BONE_INFO_HEADER bone_header;
        reader.read(&bone_header, sizeof(bone_header));

        bone_info bone;
        bone.offset_transform = bone_header.offset;

        char name_buf[33] = {};
        reader.read(name_buf, bone_header.nameLen);
        bone.name    = name_buf;
        bone.bone_id = i;

        m.set_bone(i, bone);
    }

    const bool has_normal =
        (header.flags() & mesh_flags::normal) != mesh_flags::none;
    const bool has_color =
        (header.flags() & mesh_flags::color) != mesh_flags::none;
    const bool has_tex = (header.flags() & mesh_flags::tex) != mesh_flags::none;
    const bool has_bone =
        (header.flags() & mesh_flags::bone) != mesh_flags::none;

    // Read vertices
    for (uint16_t i = 0; i < header.vertexCount; ++i)
    {
        position_t pos;
        ICE_VEC3_R16 p;
        reader.read(&p, sizeof(p));
        pos = p;
        m.set_position(i, pos);

        if (has_normal)
        {
            normal_t norm;
            ICE_NORM3 n;
            reader.read(&n, sizeof(n));
            norm = n;
            m.set_normal(i, norm);
        }

        if (has_color)
        {
            color_t c;
            color5551 ic;
            reader.read(&ic.value, sizeof(ic.value));
            c = ic.as_color();
            m.set_color(i, c);
        }

        if (has_tex)
        {
            texcoord_t uv;
            ICE_VEC2_U16 iuv;
            reader.read(&iuv, sizeof(iuv));
            uv = iuv;
            m.set_texcoord(i, uv);
        }

        if (has_bone)
        {
            boneweight_t bones;
            ICE_VEC2<int16_t, ICE_R16> ibones[4];
            reader.read(ibones, sizeof(ibones));
            for (int j = 0; j < 4; ++j)
            {
                bones[j].bone_id = ibones[j].x;
                bones[j].weight  = ibones[j].y;
            }
            m.set_boneweight(i, bones);
        }
    }

    const size_t primitive_size = m.get_index_count_per_primitive();

    // Read indices
    if (header.is_index_8bit())
    {
        uint8_t primitive[4];
        for (uint16_t i = 0; i < header.primitiveCount; ++i)
        {
            reader.read(&primitive[0], primitive_size);
            for (size_t j = 0; j < primitive_size; ++j)
                m.m_indices[i * primitive_size + j] = primitive[j];
        }
    }
    else
    {
        reader.read(m.m_indices.data(),
                    header.primitiveCount * primitive_size * sizeof(uint16_t));
    }

    return error_code::ok;
}

} // namespace zabato
