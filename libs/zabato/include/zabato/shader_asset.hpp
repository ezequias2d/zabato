#pragma once

#include <zabato/resource.hpp>
#include <zabato/script.hpp>
#include <zabato/shader.hpp>
#include <zabato/vector.hpp>

namespace zabato
{

class shader_asset : public resource
{
public:
    static constexpr chunk_id CHUNK_ID = chunk_id("SHDA");
    static const rtti TYPE;
    virtual const rtti &type() const override { return TYPE; }

    shader_asset();
    virtual ~shader_asset();

    // Source Management
    void set_source(const string_view &source);
    const string &get_source() const { return m_source; }

    // Compilation
    bool compile(class script_system &ss,
                 class gpu *g,
                 const string_view &backend = "glsl120");
    bool is_compiled() const;

    // Interface
    const zshader_compilation_result &get_compilation_result() const
    {
        return m_result;
    }

    class program *get_program() const;

private:
    string m_source;
    zshader_compilation_result m_result;
    shared_ptr<program> m_program;
};

template <>
inline result<void> deserialize(ice_reader &reader, shader_asset &obj)
{
    // Basic deserialization of source
    // Compilation happens at runtime or we bake it?
    // For now, load source.
    bool compressed = false;
    auto chunk = reader.find_chunk_or_berg(shader_asset::CHUNK_ID, compressed);
    if (!chunk)
        return report_error(error_code::chunk_broken,
                            "Missing SHDA chunk",
                            (uint32_t)shader_asset::CHUNK_ID);

    vector<char> buf(chunk->size);
    reader.read(buf.data(), chunk->size);
    obj.set_source(string(buf.data(), buf.size()));
    return error_code::ok;
}

} // namespace zabato
