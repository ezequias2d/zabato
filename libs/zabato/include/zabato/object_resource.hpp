#pragma once

#include "zabato/stream.hpp"
#include <zabato/error.hpp>
#include <zabato/object.hpp>
#include <zabato/resource.hpp>
#include <zabato/serializer.hpp>

namespace zabato
{
class object_resource : public resource
{
public:
    object_resource() : m_object(nullptr) {}
    ~object_resource() override {}

    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }

    static constexpr chunk_id CHUNK_ID = chunk_id("OBJC");

    pointer<object> get_object() const { return m_object; }
    void set_object(pointer<object> object) { m_object = object; }

private:
    pointer<object> m_object = nullptr;

    template <typename T>
    friend result<void> deserialize(ice_reader &reader, T &obj);
};

template <>
inline result<void> deserialize(ice_reader &reader, object_resource &obj)
{
    TRACE_FUNCTION;

    auto [error, chunk] = reader.find_chunk(object_resource::CHUNK_ID);
    if (error)
        return error;

    vector<uint8_t> data(chunk.size);
    if (auto res = reader.read(data.data(), chunk.size); !res)
        return report_error(error_code::operation, "deserialize", "");

    memory_stream stream(data);
    serializer ser(nullptr);
    if (auto res = ser.load(stream); !res)
        return report_error(error_code::operation, "deserialize", "");

    return error_code::ok;
}

template <>
inline result<void> serialize(ice_writer &writer, const object_resource &obj)
{
    TRACE_FUNCTION;

    auto o = obj.get_object();
    if (!o)
        return report_error(error_code::null_handle);

    serializer ser(nullptr);

    vector<uint8_t> data;
    memory_stream stream(data);

    if (auto res = ser.save(stream, o); !res)
        return report_error(error_code::operation, "serialize", "");

    writer.write_chunk_header(object_resource::CHUNK_ID, data.size());
    writer.write(data.data(), data.size());

    return error_code::ok;
}
} // namespace zabato