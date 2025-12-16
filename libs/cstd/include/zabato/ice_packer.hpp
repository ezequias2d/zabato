#pragma once

#include "zabato/ice.hpp"
#include "zabato/stream.hpp"

namespace zabato::fs
{
class ice_packer
{
public:
    ice_packer(file_stream &stream) : m_stream(stream), m_writer(stream) {}

    result<void> pack(const string &source_path);

private:
    file_stream &m_stream;
    ice_writer m_writer;
};
} // namespace zabato::fs