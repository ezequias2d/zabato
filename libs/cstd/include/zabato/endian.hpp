#pragma once

namespace zabato
{

/**
 * @brief Compile-time constant indicating if the system is little-endian.
 *
 * This value is determined using standard compiler macros (__BYTE_ORDER__ for
 * GCC/Clang) or platform macros (_WIN32 for Windows).
 */
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
inline constexpr bool is_little_endian =
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);
#elif defined(_WIN32)
inline constexpr bool is_little_endian = true;
#else
#error "Unable to determine endianness"
#endif

} // namespace zabato