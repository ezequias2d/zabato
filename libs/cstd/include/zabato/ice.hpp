#pragma once

#include <zabato/berg.h>
#include <zabato/error.hpp>
#include <zabato/fixed_string.hpp>
#include <zabato/math.hpp>
#include <zabato/real.hpp>
#include <zabato/span.hpp>
#include <zabato/stream.hpp>
#include <zabato/string.hpp>
#include <zabato/vector.hpp>

#include <iostream>
#include <math.h>
#include <stdint.h>
#include <zabato/be.hpp>
#include <zabato/le.hpp>

namespace zabato
{

#ifdef ICE_LITTLE_ENDIAN
using ice_int8_t   = le_int8_t;
using ice_uint8_t  = le_uint8_t;
using ice_int16_t  = le_int16_t;
using ice_uint16_t = le_uint16_t;
using ice_int32_t  = le_int32_t;
using ice_uint32_t = le_uint32_t;
using ice_int64_t  = le_int64_t;
using ice_uint64_t = le_uint64_t;
using ice_float    = le_float;
using ice_double   = le_double;
using ice_real     = le_real;
#else
using ice_int8_t   = be_int8_t;
using ice_uint8_t  = be_uint8_t;
using ice_int16_t  = be_int16_t;
using ice_uint16_t = be_uint16_t;
using ice_int32_t  = be_int32_t;
using ice_uint32_t = be_uint32_t;
using ice_int64_t  = be_int64_t;
using ice_uint64_t = be_uint64_t;
using ice_float    = be_float;
using ice_double   = be_double;
using ice_real     = be_real;
#endif

class chunk_id
{
private:
    union Value
    {
        uint32_t u;
        char c[4];

        constexpr Value(uint32_t value) : u(value) {}
        constexpr Value(const char (&str)[5])
            : c{str[0], str[1], str[2], str[3]}
        {
        }
    };

public:
    constexpr chunk_id(uint32_t value = 0) : m_value(value) {};
    constexpr chunk_id(const char (&str)[5]) : m_value(str) {}

    constexpr bool operator==(const chunk_id &other) const
    {
        return m_value.u == other.m_value.u;
    }
    constexpr bool operator!=(const chunk_id &other) const
    {
        return m_value.u != other.m_value.u;
    }

    constexpr operator uint32_t() const { return m_value.u; }

    fixed_string<5> to_string() const { return fixed_string<5>(m_value.c, 4); }

private:
    Value m_value;
};

/** @brief Header for a chunk within an ICE file. */
struct chunk_header
{
    chunk_id id;
    ice_uint32_t size;
};

static const chunk_id BERG_CHUNK_ID("BERG");
static const chunk_id CHUNK_INDEX("IDEX");
static const chunk_id CHUNK_PACK("PACK");
static const chunk_id CHUNK_FILE("FILE");

#pragma pack(push, 1)

/** @brief Header for a Berg-compressed chunk in an ICE file. */
struct ICE_BERG_HEADER
{
    /** The chunk ID of the original uncompressed data */
    chunk_id original_chunk_id;
    /** Size of the original uncompressed data */
    ice_uint32_t original_size;
    /** Size of the compressed data (for verification) */
    ice_uint32_t compressed_size;
};

struct ICE_INDEX_ENTRY
{
    ice_uint64_t path_offset; //< Offset into string block
    ice_uint64_t data_offset; //< Offset of file/dir data
    ice_uint64_t size;        //< Size of data
    ice_uint64_t flags;       //< Reserved, always 0
};

struct ICE_PACK
{
    ice_uint32_t version;
    ice_uint32_t file_count;
    ice_uint64_t root_dir_offset;
};

#pragma pack(pop)

/**
 * @class ice_reader
 * @brief Reads data and chunks from an ICE stream.
 */
class ice_reader
{
public:
    explicit ice_reader(stream &stream) : m_stream(stream) {}
    ice_reader(const ice_reader &other) : m_stream(other.m_stream) {}
    ice_reader(ice_reader &&other) : m_stream(other.m_stream) {}

    ice_reader &operator=(const ice_reader &other)
    {
        m_stream = other.m_stream;
        return *this;
    }
    ice_reader &operator=(ice_reader &&other)
    {
        m_stream = other.m_stream;
        return *this;
    }

    /** @brief Resets the underlying stream to the beginning. */
    void rewind() { m_stream.rewind(); }

    /**
     * @brief Finds the next chunk with the specified ID.
     * @param id The four-character code of the chunk to find.
     * @return A header with the chunk's ID and size, or a header with id 0 if
     * not found.
     */
    result<chunk_header> find_chunk(chunk_id id)
    {
        chunk_header header = {0, 0};
        while (!m_stream.eof())
        {
            auto buf = as_writable_bytes(header);
            if (m_stream.read(buf) != sizeof(header))
                return report_error(error_code::end_of_stream);

            if (header.id == id)
                return header; // Found

            // Skip to next chunk
            m_stream.skip(header.size);
        }
        return report_error(error_code::chunk_not_reached,
                            id.to_string().c_str(),
                            (uint32_t)id);
    }

    /**
     * @brief Finds a chunk, checking for both original and Berg-compressed
     * versions.
     * @param original_chunk_id The ID of the original uncompressed chunk type
     * @param out_compressed Set to true if a Berg-compressed version was found
     * @return A header with the chunk's ID and size, or error if not found
     */
    result<chunk_header> find_chunk_or_berg(chunk_id original_chunk_id,
                                            bool &out_compressed)
    {
        rewind(); // Start from beginning

        chunk_header header = {0, 0};
        while (!m_stream.eof())
        {
            auto buf = as_writable_bytes(header);
            if (m_stream.read(buf) != sizeof(header))
                return report_error(error_code::end_of_stream);

            if (header.id == original_chunk_id)
            {
                out_compressed = false;
                return header; // Found uncompressed version
            }
            else if (header.id == BERG_CHUNK_ID)
            {
                // Check if this Berg chunk contains our target chunk
                ICE_BERG_HEADER berg_header;
                auto buf = as_writable_bytes(berg_header);
                if (m_stream.read(buf) != sizeof(berg_header))
                {
                    m_stream.skip(header.size -
                                  sizeof(berg_header)); // Skip rest of chunk
                    continue;
                }

                if (berg_header.original_chunk_id == original_chunk_id)
                {
                    // Found compressed version! Rewind to start of Berg chunk
                    m_stream.skip(-static_cast<int64_t>(sizeof(berg_header)));
                    out_compressed = true;
                    return header;
                }
                else
                {
                    // Not our chunk, skip the compressed data
                    m_stream.skip(header.size - sizeof(berg_header));
                }
            }
            else
            {
                // Skip to next chunk
                m_stream.skip(header.size);
            }
        }

        return report_error(error_code::chunk_not_reached,
                            original_chunk_id.to_string().c_str(),
                            (uint32_t)original_chunk_id);
    }

    /**
     * @brief Reads and decompresses a Berg chunk.
     * @param expected_original_id The expected original chunk ID
     * @param out_data Output vector to store decompressed data
     * @return Result indicating success or failure
     */
    result<void> read_berg_chunk(chunk_id expected_original_id,
                                 vector<uint8_t> &out_data)
    {
        // Read Berg header
        ICE_BERG_HEADER berg_header;
        if (read(&berg_header, sizeof(berg_header)) != sizeof(berg_header))
            return report_error(error_code::unable_to_read);

        // Verify this is the chunk we expect
        if (berg_header.original_chunk_id != expected_original_id)
        {
            std::cout << "Expected chunk ID: "
                      << expected_original_id.to_string().c_str()
                      << " but got: " << berg_header.original_chunk_id
                      << std::endl;
            return report_error(error_code::chunk_broken, BERG_CHUNK_ID);
        }

        // Read compressed data
        vector<uint8_t> compressed_data(berg_header.compressed_size);
        if (read(compressed_data.data(), compressed_data.size()) !=
            berg_header.compressed_size)
            return report_error(error_code::unable_to_read);

        // Decompress data
        out_data.resize(berg_header.original_size);
        size_t decompressed_size = 0;

        auto d_err = berg_decompress_raw(compressed_data.data(),
                                         compressed_data.size(),
                                         out_data.data(),
                                         out_data.size(),
                                         berg_header.original_size,
                                         &decompressed_size);

        if (d_err < 0)
            return report_error(error_code::fail_to_decompress_berg,
                                expected_original_id.to_string().c_str(),
                                (uint32_t)expected_original_id,
                                d_err);

        out_data.resize(decompressed_size);
        // Verify decompressed size matches expected
        if (out_data.size() != berg_header.original_size)
        {
            std::cout << "Decompressed size: " << out_data.size()
                      << " but expected: " << berg_header.original_size
                      << std::endl;
            return report_error(error_code::chunk_broken, BERG_CHUNK_ID);
        }

        return result<void>();
    }

    result<void> find_and_read(chunk_id id, vector<uint8_t> &out_data)
    {
        bool is_compressed   = false;
        auto [error, header] = find_chunk_or_berg(id, is_compressed);
        if (error)
            return error;

        if (is_compressed)
            return read_berg_chunk(id, out_data);

        out_data.resize(header.size);
        if (read(out_data.data(), header.size) != header.size)
            return report_error(error_code::unable_to_read);

        return error_code::ok;
    }

    /**
     * @brief Reads a block of data from the current stream position.
     * @param ptr The destination buffer.
     * @param size The number of bytes to read.
     * @return The number of bytes actually read.
     */
    size_t read(void *ptr, size_t size)
    {
        buffer buf((uint8_t *)ptr, size);
        return read(buf);
    }

    size_t read(buffer &buf) { return m_stream.read(buf); }

    template <typename T> size_t read(T &data)
    {
        auto buf = as_writable_bytes(data);
        return read(buf);
    }

private:
    stream &m_stream;
};

/**
 * @class ice_writer
 * @brief Writes data and chunks to an ICE stream.
 */
class ice_writer
{
public:
    explicit ice_writer(stream &stream) : m_stream(stream) {}
    ice_writer(const ice_writer &other) : m_stream(other.m_stream) {}
    ice_writer(ice_writer &&other) : m_stream(other.m_stream) {}

    ice_writer &operator=(const ice_writer &other)
    {
        m_stream = other.m_stream;
        return *this;
    }
    ice_writer &operator=(ice_writer &&other)
    {
        m_stream = other.m_stream;
        return *this;
    }

    /**
     * @brief Writes a chunk header to the stream.
     * @param id The four-character code for the chunk.
     * @param size The size of the chunk's payload data.
     */
    result<size_t> write_chunk_header(chunk_id id, size_t size)
    {
        chunk_header header = {
            id,
            static_cast<uint32_t>(size),
        };
        auto buf       = as_writable_bytes(header);
        size_t written = m_stream.write(buf);
        if (written != sizeof(header))
            return report_error(error_code::unable_to_write);
        return written;
    }

    /**
     * @brief Writes a block of data to the stream.
     * @param ptr The source buffer.
     * @param size The number of bytes to write.
     * @return The number of bytes actually written.
     */
    result<size_t> write(const void *ptr, size_t size)
    {
        buffer buf((uint8_t *)ptr, size);
        size_t written = m_stream.write(buf);
        if (written != size)
            return report_error(error_code::unable_to_write);
        return written;
    }

    /**
     * @brief Writes a Berg-compressed chunk to the stream.
     * @param original_chunk_id The ID of the original uncompressed chunk type
     * @param buffer The source buffer to compress and write
     * @param size The number of bytes to compress
     * @return Result indicating success or failure
     */
    result<void> write_berg_chunk(const void *buffer, size_t size)
    {
        vector<uint8_t> compressed_data(
            berg_estimate_max_compressed_size(size));
        size_t compressed_size = 0;

        berg_config config{
            .lookahead_size = 16,
        };

        uint32_t original_id = 0;
        memcpy(&original_id, buffer, sizeof(original_id));

        auto c_err = berg_compress_raw(static_cast<const uint8_t *>(buffer),
                                       size,
                                       compressed_data.data(),
                                       compressed_data.size(),
                                       &compressed_size,
                                       &config);
        if (c_err < 0)
            return report_error(error_code::fail_to_compress_berg,
                                BERG_CHUNK_ID,
                                (uint32_t)BERG_CHUNK_ID,
                                c_err);

        compressed_data.resize(compressed_size);
        compressed_data.shrink_to_fit();

        // Create Berg header
        ICE_BERG_HEADER berg_header = {
            original_id,
            static_cast<uint32_t>(size),
            static_cast<uint32_t>(compressed_data.size())};

        // Calculate total chunk size
        size_t total_chunk_size = sizeof(berg_header) + compressed_data.size();

        // Write Berg chunk header
        auto chunk_result = write_chunk_header(BERG_CHUNK_ID, total_chunk_size);
        if (chunk_result.has_error())
            return chunk_result.error;

        // Write Berg header
        auto header_result = write(&berg_header, sizeof(berg_header));
        if (header_result.has_error())
            return header_result.error;

        // Write compressed data
        auto data_result =
            write(compressed_data.data(), compressed_data.size());
        if (data_result.has_error())
            return data_result.error;

        return result<void>();
    }

private:
    stream &m_stream;
};

template <typename T> result<void> serialize(ice_writer &writer, const T &obj);
template <typename T> result<void> deserialize(ice_reader &reader, T &obj);

template <typename T>
result<void> serialize(ice_writer &writer, const T &obj, bool compressed)
{
    if (compressed)
    {
        // Serialize to memory stream first
        vector<uint8_t> buffer = {};
        memory_stream mem_stream(buffer);
        ice_writer mem_writer(mem_stream);
        auto sres = serialize(mem_writer, obj);
        if (sres.has_error())
            return sres.error;

        // Write as Berg chunk
        return writer.write_berg_chunk(buffer.data(), buffer.size());
    }
    return serialize(writer, obj);
};

template <typename T>
result<void> deserialize(ice_reader &reader, T &obj, bool compressed)
{
    if (compressed)
    {
        vector<uint8_t> buffer;
        auto rres = reader.read_berg_chunk(T::CHUNK_ID, buffer);
        if (rres.has_error())
            return rres.error;

        memory_stream mem_stream(buffer);
        ice_reader mem_reader(mem_stream);
        return deserialize(mem_reader, obj);
    }
    return deserialize(reader, obj);
};

#pragma region ICE Structs
#pragma pack(push, 1)
static constexpr real ICE_SCALE = real(64);

struct ICE_R16
{
    ice_int16_t v;

    ICE_R16() : v(0) {}
    ICE_R16(real value) : v(static_cast<int16_t>(value * ICE_SCALE)) {}
    operator real() const { return real(int16_t(v)) * (real(1) / ICE_SCALE); }

    bool operator==(const ICE_R16 &other) const
    {
        return int16_t(v) == int16_t(other.v);
    }
    bool operator!=(const ICE_R16 &other) const
    {
        return int16_t(v) != int16_t(other.v);
    }
    bool operator<(const ICE_R16 &other) const
    {
        return int16_t(v) < int16_t(other.v);
    }
    bool operator<=(const ICE_R16 &other) const
    {
        return int16_t(v) <= int16_t(other.v);
    }
    bool operator>(const ICE_R16 &other) const
    {
        return int16_t(v) > int16_t(other.v);
    }
    bool operator>=(const ICE_R16 &other) const
    {
        return int16_t(v) >= int16_t(other.v);
    }
};

struct ICE_R8
{
    ice_int8_t v;

    ICE_R8() : v(0) {}
    ICE_R8(real value) : v(static_cast<int8_t>(value * ICE_SCALE)) {}
    operator real() const { return real(int8_t(v)) * (real(1) / ICE_SCALE); }
};

struct ICE_N16
{
    ice_int16_t v;

    ICE_N16() : v(0) {}
    ICE_N16(real value)
    {
        auto iv = map<real>(
            value, real(-1.0), real(1.0), real(INT16_MIN), real(INT16_MAX));
        v = static_cast<int16_t>(iv);
    }
    operator real() const
    {
        auto iv = map<real>(real(int16_t(v)),
                            real(INT16_MIN),
                            real(INT16_MAX),
                            real(-1.0),
                            real(1.0));
        return iv;
    }
};

template <typename XT, typename YT> struct ICE_VEC2
{
    XT x;
    YT y;

    ICE_VEC2() : x(0), y(0) {}
    ICE_VEC2(XT x, YT y) : x(x), y(y) {}
    ICE_VEC2(const vec2<real> &vec)
        : x(static_cast<XT>(vec.x)), y(static_cast<YT>(vec.y))
    {
    }
    operator vec2<real>() const { return vec2<real>(real(x), real(y)); }
};

typedef ICE_VEC2<ICE_R16, ICE_R16> ICE_VEC2_R16;
typedef ICE_VEC2<ice_uint16_t, ice_uint16_t> ICE_VEC2_U16;
typedef ICE_VEC2<ice_int16_t, ice_int16_t> ICE_VEC2_I16;

template <typename XT, typename YT, typename ZT> struct ICE_VEC3
{
    XT x;
    YT y;
    ZT z;

    ICE_VEC3() : x(0), y(0), z(0) {}
    ICE_VEC3(XT x, YT y, ZT z) : x(x), y(y), z(z) {}
    ICE_VEC3(const vec3<real> &vec)
        : x(static_cast<XT>(vec.x)), y(static_cast<YT>(vec.y)),
          z(static_cast<ZT>(vec.z))
    {
    }
    operator vec3<real>() const
    {
        return vec3<real>(real(x), real(y), real(z));
    }
};

typedef ICE_VEC3<ICE_R16, ICE_R16, ICE_R16> ICE_VEC3_R16;
typedef ICE_VEC3<ice_int16_t, ice_int16_t, ice_int16_t> ICE_VEC3_I16;
typedef ICE_VEC3<ice_uint16_t, ice_uint16_t, ice_uint16_t> ICE_VEC3_U16;

struct ICE_NORM3 : ICE_VEC2<ICE_N16, ICE_N16>
{
    ICE_NORM3() {}

    // Octahedral encoding
    ICE_NORM3(const vec3<real> &vec)
    {
        vec3<real> n = normalize(vec);
        n *= real(1.0) / (abs(n.x) + abs(n.y) + abs(n.z));
        vec2<real> enc;

        enc = n.z > real(0.0) ? vec2<real>(n.x, n.y)
                              : oct_wrap(vec2<real>(n.x, n.y));
        enc = enc * real(0.5) + real(0.5);

        x = enc.x;
        y = enc.y;
    }

    operator vec3<real>() const
    {
        vec2<real> f = vec2<real>(real(x), real(y));

        vec3 n = vec3(f.x, f.y, real(1.0) - abs(f.x) - abs(f.y));
        real t = max(-n.z, real(0.0));

        n.x += n.x >= real(0.0) ? -t : t;
        n.y += n.y >= real(0.0) ? -t : t;

        return normalize(n);
    }

private:
    static vec2<real> oct_wrap(const vec2<real> &v)
    {
        vec2<real> w = vec2<real>(1) - abs(v);
        if (v.x < real(0.0))
            w.x = -w.x;
        if (v.y < real(0.0))
            w.y = -w.y;
        return w;
    }
};

// Cayley-Klein parameterization
struct ICE_QUAT_R16 : ICE_VEC3<ICE_R16, ICE_R16, ICE_R16>
{
    ICE_QUAT_R16() {}
    ICE_QUAT_R16(const quat<real> &q)
    {
        real s = real(1.0) / (real(1.0) + q.w);
        vec3 r = vec3<real>(q.x, q.y, q.z) * s;
        x      = r.x;
        y      = r.y;
        z      = r.z;
    }

    operator quat<real>() const
    {
        vec3<real> v = vec3<real>(real(x), real(y), real(z));
        real s       = real(2.0) / (real(1.0) + dot(v, v));
        return quat<real>(v * s, s - 1.0);
    }
};

template <typename T> struct ICE_MAT4X4
{
    T m[4][4];

    ICE_MAT4X4() { memset(m, 0, sizeof(m)); }
    ICE_MAT4X4(const mat4<real> &mat)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                m[row][col] = static_cast<T>(mat.m[row][col]);
            }
        }
    }
    operator mat4<real>() const
    {
        mat4<real> mat;
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                mat.m[row][col] = real(m[row][col]);
            }
        }
        return mat;
    }
};

typedef ICE_MAT4X4<ICE_R16> ICE_MAT4X4_R16;

#pragma pack(pop)
#pragma endregion

} // namespace zabato