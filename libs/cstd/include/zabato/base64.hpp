#pragma once

#include <cstdint>
#include <zabato/string.hpp>
#include <zabato/vector.hpp>

namespace zabato::base64
{

// Base64 Alphabet
static constexpr char ENCODE_TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
    # Reverse Lookup Table (256 bytes)
    0xFF indicates an invalid character.

    | Index   | Base64 Value   |
    |---------|--=-------------|
    | 0-39    | 0xFF (invalid) |
    | 40-47   | 62 (+)         |
    | 48-55   | 63 (/)         |
    | 56-63   | 0-7            |
    | 64-71   | 8-9            |
    | 72-79   | A-G            |
    | 80-87   | H-O            |
    | 88-95   | P-W            |
    | 96-103  | X-Z            |
    | 104-111 | a-g            |
    | 112-119 | h-o            |
    | 120-127 | p-w            |
    | 128-255 | 0xFF (invalid) |
*/
static constexpr unsigned char DECODE_TABLE[256] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 62,   0xFF, 0xFF, 0xFF, 63,
    52,   53,   54,   55,   56,   57,   58,   59,   60,   61,   0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0,    1,    2,    3,    4,    5,    6,
    7,    8,    9,    10,   11,   12,   13,   14,   15,   16,   17,   18,
    19,   20,   21,   22,   23,   24,   25,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,
    37,   38,   39,   40,   41,   42,   43,   44,   45,   46,   47,   48,
    49,   50,   51,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF};

inline string encode(unsigned char const *bytes_to_encode, size_t in_len)
{
    if (in_len == 0)
        return string();

    size_t out_len = 4 * ((in_len + 2) / 3);

    string ret;
    ret.resize(out_len);

    char *out                = &ret[0];
    const unsigned char *in  = bytes_to_encode;
    const unsigned char *end = bytes_to_encode + in_len;

    while (end - in >= 3)
    {
        uint32_t octet_a = *in++;
        uint32_t octet_b = *in++;
        uint32_t octet_c = *in++;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        *out++ = ENCODE_TABLE[(triple >> 3 * 6) & 0x3F];
        *out++ = ENCODE_TABLE[(triple >> 2 * 6) & 0x3F];
        *out++ = ENCODE_TABLE[(triple >> 1 * 6) & 0x3F];
        *out++ = ENCODE_TABLE[(triple >> 0 * 6) & 0x3F];
    }

    // Handle padding
    if (in < end)
    {
        uint32_t octet_a = *in++;
        uint32_t octet_b = (in < end) ? *in++ : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08);

        *out++ = ENCODE_TABLE[(triple >> 3 * 6) & 0x3F];
        *out++ = ENCODE_TABLE[(triple >> 2 * 6) & 0x3F];

        if (end - bytes_to_encode - in_len + 2 == 1) // 1 byte remaining
        {
            *out++ = '=';
            *out++ = '=';
        }
        else // 2 bytes remaining
        {
            *out++ = ENCODE_TABLE[(triple >> 1 * 6) & 0x3F];
            *out++ = '=';
        }
    }

    return ret;
}

inline vector<uint8_t> decode(string const &encoded_string)
{
    size_t in_len = encoded_string.size();
    if (in_len == 0)
        return vector<uint8_t>();
    if (in_len % 4 != 0)
        return vector<uint8_t>(); // Invalid Base64 length

    // Maximum possible output size
    size_t out_len = (in_len / 4) * 3;

    // Adjust for padding
    if (encoded_string[in_len - 1] == '=')
        out_len--;
    if (encoded_string[in_len - 2] == '=')
        out_len--;

    vector<uint8_t> ret;
    ret.resize(out_len);

    uint8_t *out    = &ret[0];
    const char *in  = &encoded_string[0];
    const char *end = in + in_len;

    while (in < end)
    {
        unsigned char c1 = DECODE_TABLE[static_cast<unsigned char>(in[0])];
        unsigned char c2 = DECODE_TABLE[static_cast<unsigned char>(in[1])];
        unsigned char c3 = DECODE_TABLE[static_cast<unsigned char>(in[2])];
        unsigned char c4 = DECODE_TABLE[static_cast<unsigned char>(in[3])];

        // 0xFF is invalid.
        if (c3 == 0xFF || c4 == 0xFF)
        {
            // Handle padding cases or invalid chars
            if (in[2] == '=') // XX==
            {
                uint32_t triple = (c1 << 6) + c2;
                *out++          = (triple >> 4) & 0xFF;
                break;
            }
            else if (in[3] == '=') // XXX=
            {
                uint32_t triple = (c1 << 12) + (c2 << 6) + c3;
                *out++          = (triple >> 10) & 0xFF;
                *out++          = (triple >> 2) & 0xFF;
                break;
            }
            if (c1 == 0xFF || c2 == 0xFF)
                break;
        }

        uint32_t triple = (c1 << 18) + (c2 << 12) + (c3 << 6) + c4;

        *out++ = (triple >> 16) & 0xFF;
        *out++ = (triple >> 8) & 0xFF;
        *out++ = triple & 0xFF;

        in += 4;
    }

    ret.resize(out - &ret[0]);
    return ret;
}

} // namespace zabato::base64