#pragma once

#include <zabato/span.hpp>
#include <zabato/vector.hpp>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

namespace zabato
{
/**
 * @class stream
 * @brief An abstract interface for stream I/O operations.
 *
 * Implement this interface to allow an reader or writer to read or write to and
 * from any source (e.g., memory, network, files).
 */
class stream
{
public:
    virtual ~stream() = default;

    /**
     * @brief Reads a block of data from the stream.
     * @param buffer The destination buffer for the data.
     * @param size The number of bytes to read.
     * @return The number of bytes actually read.
     */
    virtual size_t read(buffer &buffer) = 0;

    /**
     * @brief Writes a block of data to the stream.
     * @param buffer The source buffer for the data.
     * @return The number of bytes actually written.
     */
    virtual size_t write(const buffer &buffer) = 0;

    /**
     * @brief Skips a number of bytes in the stream.
     * @param offset The number of bytes to skip.
     */
    virtual void skip(int64_t offset) = 0;

    /** @brief Returns true if the stream is at the end. */
    virtual bool eof() const = 0;

    /** @brief Resets the stream position to the beginning. */
    virtual void rewind() = 0;

    /** @brief Returns position of stream. */
    virtual size_t tell() const = 0;

    /** @brief Sets position of stream. */
    virtual void pos(int64_t offset) = 0;
};

/**
 * @class file_stream
 * @brief An implementation of stream for standard C FILE pointers.
 */
class file_stream : public stream
{
public:
    explicit file_stream(FILE *f) : m_file(f) {}

    FILE *get_file() { return m_file; }

    size_t read(buffer &buffer) override final
    {
        return fread(buffer.data(), 1, buffer.size(), m_file);
    }

    size_t write(const buffer &buffer) override final
    {
        return fwrite(buffer.data(), 1, buffer.size(), m_file);
    }

    void skip(int64_t offset) override final
    {
        fseek(m_file, static_cast<long>(offset), SEEK_CUR);
    }

    bool eof() const override final { return feof(m_file); }

    void rewind() override final { ::rewind(m_file); }

    void pos(int64_t offset) override final
    {
        fseek(m_file, static_cast<long>(offset), SEEK_SET);
    }

    size_t tell() const override final { return ftell(m_file); }

private:
    FILE *m_file;
};

/**
 * @class memory_stream
 * @brief An implementation of stream for an in-memory vector.
 */
class memory_stream : public stream
{
public:
    explicit memory_stream(vector<uint8_t> &buffer)
        : m_buffer(buffer), m_cursor(0)
    {
    }

    size_t read(buffer &buffer) override
    {
        if (eof())
            return 0;
        size_t bytes_to_read = min(buffer.size(), m_buffer.size() - m_cursor);
        if (bytes_to_read > 0)
        {
            memcpy(buffer.data(), m_buffer.data() + m_cursor, bytes_to_read);
            m_cursor += bytes_to_read;
        }
        return bytes_to_read;
    }

    size_t write(const buffer &buffer) override final
    {
        const uint8_t *byte_buffer = buffer.data();
        for (size_t i = 0; i < buffer.size(); ++i)
            m_buffer.push_back(byte_buffer[i]);
        return buffer.size();
    }

    void skip(int64_t offset) override final
    {
        if (offset > 0 && m_cursor + offset > m_buffer.size())
        {
            m_cursor = m_buffer.size();
        }
        else if (offset < 0 && m_cursor < static_cast<size_t>(-offset))
        {
            m_cursor = 0;
        }
        else
        {
            m_cursor += offset;
        }
    }

    void pos(int64_t offset) override final
    {
        if (offset < 0)
            m_cursor = 0;
        else if (offset > (int64_t)m_buffer.size())
            m_cursor = m_buffer.size();
        else
            m_cursor = (size_t)offset;
    }

    size_t tell() const override final { return m_cursor; }

    bool eof() const override final { return m_cursor >= m_buffer.size(); }
    void rewind() override final { m_cursor = 0; }
    size_t cursor() const { return m_cursor; }
    size_t size() const { return m_buffer.size(); }
    size_t capacity() const { return m_buffer.capacity(); }

private:
    vector<uint8_t> &m_buffer;
    size_t m_cursor;
};

} // namespace zabato
