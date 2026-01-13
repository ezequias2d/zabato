#pragma once

#include <zabato/span.hpp>
#include <zabato/vector.hpp>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

namespace zabato
{

/**
 * @enum origin
 * @brief Specifies the reference point for file seeking.
 */
enum class origin
{
    /** @brief Beginning of the file. */
    begin,
    /** @brief Current position of the file pointer. */
    current,
    /** @brief End of the file. */
    end,
};

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
     * @return The number of bytes actually read.
     */
    virtual size_t read(buffer buffer) = 0;

    /**
     * @brief Writes a block of data to the stream.
     * @param buffer The source buffer for the data.
     * @return The number of bytes actually written.
     */
    virtual size_t write(const_buffer buffer) = 0;

    /**
     * @brief Moves the file pointer to a specific location.
     * @param offset The offset in bytes relative to the origin.
     * @param origin The reference point for the offset.
     * @return True if the seek was successful, false otherwise.
     */
    virtual bool seek(int64_t offset, origin origin) = 0;

    /**
     * @brief Skips a number of bytes in the stream.
     * @param offset The number of bytes to skip.
     */
    virtual void skip(int64_t offset) { seek(offset, origin::current); }

    /** @brief Returns true if the stream is at the end. */
    virtual bool eof() const = 0;

    /** @brief Resets the stream position to the beginning. */
    virtual void rewind() { seek(0, origin::begin); }

    /** @brief Returns position of stream. */
    virtual size_t tell() const = 0;

    /** @brief Sets position of stream. */
    virtual void pos(int64_t offset) { seek(offset, origin::begin); }
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

    size_t read(buffer buffer) override final
    {
        return fread(buffer.data(), 1, buffer.size(), m_file);
    }

    size_t write(const_buffer buffer) override final
    {
        return fwrite(buffer.data(), 1, buffer.size(), m_file);
    }

    bool seek(int64_t offset, origin origin) override final
    {
        int whence = SEEK_SET;
        switch (origin)
        {
        case origin::begin:
            whence = SEEK_SET;
            break;
        case origin::current:
            whence = SEEK_CUR;
            break;
        case origin::end:
            whence = SEEK_END;
            break;
        }
        return fseek(m_file, static_cast<long>(offset), whence) == 0;
    }

    bool eof() const override final { return feof(m_file); }

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

    size_t read(buffer buffer) override
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

    size_t write(const_buffer buffer) override final
    {
        const uint8_t *byte_buffer = buffer.data();
        size_t current_size        = m_buffer.size();

        if (m_cursor + buffer.size() > current_size)
        {
            m_buffer.resize(m_cursor + buffer.size());
        }

        memcpy(m_buffer.data() + m_cursor, byte_buffer, buffer.size());
        m_cursor += buffer.size();

        return buffer.size();
    }

    bool seek(int64_t offset, origin origin) override final
    {
        int64_t new_pos = m_cursor;
        switch (origin)
        {
        case origin::begin:
            new_pos = offset;
            break;
        case origin::current:
            new_pos += offset;
            break;
        case origin::end:
            new_pos = m_buffer.size() + offset;
            break;
        }

        if (new_pos < 0)
            new_pos = 0;
        else if (new_pos > (int64_t)m_buffer.size())
            m_buffer.resize(new_pos);

        m_cursor = (size_t)new_pos;
        return true;
    }

    size_t tell() const override final { return m_cursor; }

    bool eof() const override final { return m_cursor >= m_buffer.size(); }
    size_t cursor() const { return m_cursor; }
    size_t size() const { return m_buffer.size(); }
    size_t capacity() const { return m_buffer.capacity(); }

private:
    vector<uint8_t> &m_buffer;
    size_t m_cursor;
};

} // namespace zabato
