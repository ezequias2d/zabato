#pragma once

#include <stddef.h>
#include <stdint.h>

namespace zabato
{
// Pearson hashing permutation table
static constexpr uint8_t pearson_table[256] = {
    181, 90,  132, 164, 107, 71,  161, 20,  126, 76,  129, 33,  136, 106, 128,
    117, 131, 212, 37,  175, 239, 190, 48,  191, 94,  225, 214, 195, 23,  224,
    160, 130, 236, 217, 105, 216, 186, 240, 118, 97,  77,  62,  138, 24,  170,
    192, 174, 231, 122, 116, 57,  144, 158, 79,  188, 119, 142, 243, 81,  66,
    182, 189, 75,  238, 89,  168, 254, 141, 244, 152, 149, 226, 221, 251, 39,
    215, 184, 173, 211, 147, 6,   69,  193, 137, 49,  234, 248, 124, 245, 154,
    121, 146, 104, 204, 63,  153, 61,  56,  102, 35,  127, 145, 237, 73,  155,
    115, 109, 7,   185, 65,  78,  15,  233, 16,  223, 178, 59,  165, 230, 40,
    27,  140, 179, 22,  32,  68,  112, 180, 187, 53,  9,   208, 88,  120, 143,
    14,  201, 210, 235, 111, 103, 200, 5,   87,  134, 150, 167, 45,  60,  50,
    159, 82,  17,  74,  232, 197, 95,  67,  176, 222, 84,  46,  108, 203, 11,
    18,  242, 100, 227, 19,  41,  72,  3,   44,  85,  183, 171, 113, 54,  30,
    162, 34,  55,  202, 2,   135, 218, 92,  241, 58,  220, 99,  25,  93,  169,
    0,   219, 12,  177, 43,  247, 255, 228, 196, 110, 51,  26,  96,  163, 249,
    21,  198, 8,   252, 156, 250, 205, 42,  98,  28,  172, 80,  4,   123, 166,
    253, 157, 70,  1,   52,  199, 36,  38,  194, 151, 10,  229, 114, 209, 206,
    101, 125, 64,  47,  83,  139, 29,  246, 148, 86,  133, 207, 13,  91,  31,
    213,
};

/**
 * @brief Computes one step of the Pearson hash.
 * @param hash The current hash value.
 * @param data The next byte of data to mix in.
 * @return The new hash value.
 */
inline uint8_t pearson_hash(uint8_t hash, uint8_t data)
{
    return pearson_table[hash ^ data];
}

/**
 * @brief Computes the Pearson hash for an array of bytes.
 * @param initial_hash The initial hash value (often 0).
 * @param data A pointer to the start of the data buffer.
 * @param size The number of bytes in the buffer.
 * @return The final 8-bit hash value.
 */
inline uint8_t
pearson_hash_array(uint8_t initial_hash, const uint8_t *data, size_t size)
{
    uint8_t hash = initial_hash;
    for (size_t i = 0; i < size; ++i)
    {
        hash = pearson_hash(hash, data[i]);
    }
    return hash;
}
} // namespace zabato