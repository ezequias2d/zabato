#pragma once

#include <stdint.h>
#include <string.h>
#include <zabato/span.hpp>
#include <zabato/string.hpp>
#include <zabato/utils.hpp>

namespace zabato
{

/**
 * @class uuid
 * @brief Represents a Universally Unique Identifier (UUID) v7.
 *
 * UUID v7 is time-ordered and random, making it suitable for database keys.
 */
class uuid
{
public:
    using value_type = uint8_t;

    /** @brief Default constructor. Initializes to nil UUID. */
    constexpr uuid() : m_data{0} {}

    /** @brief Constructs from raw bytes. */
    uuid(const uint8_t data[16]) { memcpy(m_data, data, 16); }

    // Explicitly safe to copy
    uuid(const uuid &other)            = default;
    uuid &operator=(const uuid &other) = default;

    /**
     * @brief Generate a new UUID v7.
     * @return A new UUID.
     */
    static uuid generate();

    /** @brief Returns pointer to data. */
    const uint8_t *data() const { return m_data; }
    /** @brief Returns pointer to data. */
    uint8_t *data() { return m_data; }

    bool operator==(const uuid &other) const
    {
        return memcmp(m_data, other.m_data, 16) == 0;
    }

    bool operator!=(const uuid &other) const { return !(*this == other); }

    bool operator<(const uuid &other) const
    {
        return memcmp(m_data, other.m_data, 16) < 0;
    }

    string to_string() const;
    operator string() const { return to_string(); }

    /**
     * @brief Writes the UUID string representation to a buffer.
     * @param buffer The buffer to write to. Must be at least 36 bytes.
     * @return true if successful, false if buffer is too small.
     */
    bool to_chars(span<char> buffer) const;

    /**
     * @brief Parse a UUID from a string view.
     * @param sv The string view to parse.
     * @return The parsed UUID.
     */
    static uuid parse(string_view sv);

    /**
     * @brief Try to parse a UUID from a string view.
     * @param sv The string view to parse.
     * @param out The parsed UUID if successful.
     * @return true if successful, false otherwise.
     */
    static bool try_parse(string_view sv, uuid &out);

private:
    uint8_t m_data[16];
};

template <> struct hash<uuid>
{
    size_t operator()(const uuid &u) const
    {
        const uint8_t *p = u.data();
        uint32_t cv      = 2166136261u;
        for (int i = 0; i < 16; ++i)
        {
            cv ^= p[i];
            cv *= 16777619;
        }
        return cv;
    }
};

} // namespace zabato