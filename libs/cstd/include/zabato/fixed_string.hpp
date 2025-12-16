#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zabato/string.hpp>

namespace zabato
{
template <size_t MaxSize> class fixed_string
{
    static_assert(MaxSize <= 256, "fixed_string MaxSize must be 256 or less");

public:
    /** @brief Default constructor. */
    fixed_string() : m_size(0) { m_buffer[0] = '\0'; }

    /** @brief Constructs from C-string. */
    fixed_string(const char *str)
    {
        m_size = 0;
        while (str[m_size] != '\0' && m_size < MaxSize - 1)
        {
            m_buffer[m_size] = str[m_size];
            m_size++;
        }
        m_buffer[m_size] = '\0';
    }

    /** @brief Constructs from C-string with specific length. */
    fixed_string(const char *str, size_t size)
    {
        m_size = 0;
        while (str[m_size] != '\0' && m_size < MaxSize - 1 && m_size < size)
        {
            m_buffer[m_size] = str[m_size];
            m_size++;
        }
        m_buffer[m_size] = '\0';
    }

    /** @brief Returns pointer to data. */
    const char *c_str() const { return m_buffer; }
    /** @brief Returns pointer to data. */
    const char *data() const { return m_buffer; }

    /** @brief Returns size of string. */
    size_t size() const { return m_size; }

    /** @brief Returns length of string. */
    size_t length() const { return m_size; }

    /** @brief Checks if empty. */
    bool empty() const { return m_size == 0; }

    /** @brief Implicit conversion to string_view. */
    operator zabato::string_view() const
    {
        return zabato::string_view(m_buffer, m_size);
    }

    bool operator==(const fixed_string &other) const
    {
        return strcmp(m_buffer, other.m_buffer) == 0;
    }

    bool operator!=(const fixed_string &other) const
    {
        return strcmp(m_buffer, other.m_buffer) != 0;
    }

    bool operator<(const fixed_string &other) const
    {
        return strcmp(m_buffer, other.m_buffer) < 0;
    }

    bool operator>(const fixed_string &other) const
    {
        return strcmp(m_buffer, other.m_buffer) > 0;
    }

    bool operator<=(const fixed_string &other) const
    {
        return strcmp(m_buffer, other.m_buffer) <= 0;
    }

    bool operator>=(const fixed_string &other) const
    {
        return strcmp(m_buffer, other.m_buffer) >= 0;
    }

    bool operator==(const char *other) const
    {
        return strcmp(m_buffer, other) == 0;
    }

    bool operator!=(const char *other) const
    {
        return strcmp(m_buffer, other) != 0;
    }

private:
    char m_buffer[MaxSize];
    uint8_t m_size;
};

} // namespace zabato