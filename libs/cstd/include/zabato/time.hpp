#pragma once

#include <stdint.h>
#include <zabato/real.hpp>

namespace zabato
{

class time
{
public:
    time() : m_value(0) {}
    constexpr explicit time(uint64_t nanoseconds) : m_value(nanoseconds) {}

    /**
     * @brief Gets the current monotonic time.
     *
     * This clock represents a steady flow of time that cannot decrease or jump.
     * It is not tied to the system's wall-clock time and is best used for
     * measuring durations, intervals, and for the main game loop timers.
     *
     * @return The current time from a monotonic clock.
     */
    static time now();

    /**
     * @brief Gets the current system wall-clock time.
     *
     * This clock represents the real-world time (Unix epoch).
     * It can jump forward or backward if the system time is changed (e.g., NTP
     * sync). Use this for generating timestamps (like UUID v7) or displaying
     * the current time.
     *
     * @return The current time from the system's real-time clock.
     */
    static time system_now();

    static constexpr time from_seconds(real seconds)
    {
        return time(static_cast<uint64_t>(seconds * 1000000000));
    }

    static constexpr time from_milliseconds(real ms)
    {
        return time(static_cast<uint64_t>(ms * 1000000));
    }

    /** @brief Creates a time value from microseconds. */
    static constexpr time from_microseconds(real us)
    {
        return time(static_cast<uint64_t>(us * 1000));
    }

    /** @brief Converts the time value to seconds. */
    real as_seconds_real() const
    {
        return static_cast<real>(m_value) / 1000000000.0;
    }

    /** @brief Converts the time value to milliseconds. */
    real as_milliseconds_real() const
    {
        return static_cast<real>(m_value) / 1000000.0;
    }

    /** @brief Converts the time value to milliseconds. */
    uint64_t as_milliseconds_uint() const { return m_value / 1000000; }

    /** @brief Converts the time value to microseconds. */
    uint64_t as_microseconds_uint() const { return m_value / 1000; }

    /** @brief Gets the raw time value in nanoseconds. */
    uint64_t as_nanoseconds() const { return m_value; }

#pragma region Operators
    bool operator==(const time &other) const
    {
        return m_value == other.m_value;
    }
    bool operator!=(const time &other) const
    {
        return m_value != other.m_value;
    }
    bool operator<(const time &other) const { return m_value < other.m_value; }
    bool operator<=(const time &other) const
    {
        return m_value <= other.m_value;
    }
    bool operator>(const time &other) const { return m_value > other.m_value; }
    bool operator>=(const time &other) const
    {
        return m_value >= other.m_value;
    }

    time operator+(const time &other) const
    {
        return time(m_value + other.m_value);
    }
    time operator-(const time &other) const
    {
        return time(m_value - other.m_value);
    }

    time &operator+=(const time &other)
    {
        m_value += other.m_value;
        return *this;
    }
    time &operator-=(const time &other)
    {
        m_value -= other.m_value;
        return *this;
    }
#pragma endregion Operators

private:
    uint64_t m_value; // Nanoseconds
};

} // namespace zabato
