#pragma once

#include <zabato/gpu.hpp>
#include <zabato/resource.hpp>
#include <zabato/string.hpp>

namespace zabato
{

class shader : public resource
{
public:
    static constexpr chunk_id CHUNK_ID = chunk_id("SHDR");

    virtual ~shader() = default;

    virtual void destroy()               = 0;
    virtual shader_type get_type() const = 0;
};

class program : public resource
{
public:
    static constexpr chunk_id CHUNK_ID = chunk_id("PROG");

    virtual ~program() = default;

    virtual void destroy()         = 0;
    virtual void attach(shader *s) = 0;
    virtual void link()            = 0;

    // Uniform location query could be useful, or handled entirely by
    // gpu::set_uniform implementation mapping strings
};

} // namespace zabato
