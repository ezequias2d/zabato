#pragma once

#include <zabato/gpu.hpp>
#include <zabato/math.hpp>
#include <zabato/resource.hpp>
#include <zabato/string.hpp>

namespace zabato
{

enum class param_type
{
    float_val,
    int_val,
    vec2_val,
    vec3_val,
    vec4_val,
    color_val,
    bool_val
};

/**
 * @struct texture_slot
 * @brief Configuration for a single texture unit.
 */
struct texture_slot
{
    resource_ref tex;
    string uniform_name;
};

struct material_parameter
{
    string name;
    param_type type = param_type::float_val;

    union
    {
        real r;
        int i;
        vec2<real> v2;
        vec3<real> v3;
        vec4<real> v4;
    };

    material_parameter() : r(0) {}
};

#pragma pack(push, 1)
struct ICE_MAT_PARAM_HEADER
{
    uint8_t type;
};
#pragma pack(pop)

template <> inline result<void> serialize(ice_writer &writer, const string &s)
{
    ice_uint32_t len = (ice_uint32_t)s.size();
    auto r           = writer.write(&len, sizeof(len));
    if (r.has_error())
        return r.error;
    if (len > 0)
    {
        auto r2 = writer.write(s.data(), len);
        if (r2.has_error())
            return r2.error;
    }
    return {};
}

template <> inline result<void> deserialize(ice_reader &reader, string &s)
{
    ice_uint32_t len;
    if (reader.read(&len, sizeof(len)) != sizeof(len))
        return report_error(error_code::unable_to_read);

    s.resize((size_t)len);
    if (len > 0)
    {
        if (reader.read(s.data(), (size_t)len) != (size_t)len)
            return report_error(error_code::unable_to_read);
    }
    return {};
}

template <>
inline result<void> serialize(ice_writer &writer, const material_parameter &p)
{
    auto res = serialize(writer, p.name);
    if (res.has_error())
        return res.error;

    ICE_MAT_PARAM_HEADER header;
    header.type = (uint8_t)p.type;
    auto res2   = writer.write(&header, sizeof(header));
    if (res2.has_error())
        return res2.error;

    switch (p.type)
    {
    case param_type::float_val:
    {
        ice_real v = p.r;
        auto r     = writer.write(&v, sizeof(v));
        if (r.has_error())
            return r.error;
        return {};
    }
    case param_type::int_val:
    case param_type::bool_val:
    {
        ice_int32_t v = p.i;
        auto r        = writer.write(&v, sizeof(v));
        if (r.has_error())
            return r.error;
        return {};
    }
    case param_type::vec2_val:
    {
        ICE_VEC2<ice_real, ice_real> v = p.v2;
        auto r                         = writer.write(&v, sizeof(v));
        if (r.has_error())
            return r.error;
        return {};
    }
    case param_type::vec3_val:
    {
        ICE_VEC3<ice_real, ice_real, ice_real> v = p.v3;
        auto r                                   = writer.write(&v, sizeof(v));
        if (r.has_error())
            return r.error;
        return {};
    }
    case param_type::vec4_val:
    case param_type::color_val:
    {
        // struct with 4 ice_reals
        ICE_VEC4<ice_real, ice_real, ice_real, ice_real> v(p.v4);
        auto r = writer.write(&v, sizeof(v));
        if (r.has_error())
            return r.error;
        return {};
    }
    default:
        return {};
    }
}

template <>
inline result<void> deserialize(ice_reader &reader, material_parameter &p)
{
    auto res = deserialize(reader, p.name);
    if (res.has_error())
        return res.error;

    ICE_MAT_PARAM_HEADER header;
    if (reader.read(&header, sizeof(header)) != sizeof(header))
        return report_error(error_code::unable_to_read);

    p.type = (param_type)(uint8_t)header.type;

    switch (p.type)
    {
    case param_type::float_val:
    {
        ice_real v;
        if (reader.read(&v, sizeof(v)) != sizeof(v))
            return report_error(error_code::unable_to_read);
        p.r = v;
        break;
    }
    case param_type::int_val:
    case param_type::bool_val:
    {
        ice_int32_t v;
        if (reader.read(&v, sizeof(v)) != sizeof(v))
            return report_error(error_code::unable_to_read);
        p.i = v;
        break;
    }
    case param_type::vec2_val:
    {
        ICE_VEC2<ice_real, ice_real> v;
        if (reader.read(&v, sizeof(v)) != sizeof(v))
            return report_error(error_code::unable_to_read);
        p.v2 = v;
        break;
    }
    case param_type::vec3_val:
    {
        ICE_VEC3<ice_real, ice_real, ice_real> v;
        if (reader.read(&v, sizeof(v)) != sizeof(v))
            return report_error(error_code::unable_to_read);
        p.v3 = v;
        break;
    }
    case param_type::vec4_val:
    case param_type::color_val:
    {
        struct
        {
            ice_real x, y, z, w;
        } v;
        if (reader.read(&v, sizeof(v)) != sizeof(v))
            return report_error(error_code::unable_to_read);
        p.v4 = vec4<real>(v.x, v.y, v.z, v.w);
        break;
    }
    default:
        break;
    }
    return {};
}

template <>
inline result<void> serialize(ice_writer &writer, const texture_slot &t)
{
    // Serialize uniform name and texture path
    auto r = serialize(writer, t.uniform_name);
    if (r.has_error())
        return r.error;

    string path = string(t.tex.path());
    return serialize(writer, path);
}

template <> inline result<void> deserialize(ice_reader &reader, texture_slot &t)
{
    auto r = deserialize(reader, t.uniform_name);
    if (r.has_error())
        return r.error;

    string path;
    r = deserialize(reader, path);
    if (r.has_error())
        return r.error;

    t.tex.set_path(path);
    return {};
}

} // namespace zabato
