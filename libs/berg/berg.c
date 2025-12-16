/*
 * Berg Compression Algorithm - ANSI C Implementation
 *
 * This implementation uses only external memory allocation and provides
 * a pure C API compatible with ANSI C compilers.
 * Uses sized types (uint8_t, uint16_t, uint32_t, etc.) for platform
 * independence.
 *
 * Copyright (c) 2024 Zabato Engine Framework
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zabato/berg.h>
#include <zabato/crc32.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Internal constants */

#define BERG_MIN_MATCH_LENGTH ((size_t)3)
#define BERG_MAX_DIRECT_MATCH_LENGTH ((size_t)5)
#define BERG_MIN_EXTENDED_MATCH_LENGTH                                         \
    ((size_t)(BERG_MAX_DIRECT_MATCH_LENGTH + 1))
#define BERG_MAX_DISTANCE ((size_t)4095)
#define BERG_MAX_DIRECT_LITERAL_COUNT ((size_t)2)
#define BERG_MIN_EXTENDED_LITERAL_COUNT                                        \
    ((size_t)(BERG_MAX_DIRECT_LITERAL_COUNT + 1))
#define BERG_HASH_SIZE ((size_t)16384)
#define BERG_MAX_CHAIN_LENGTH ((size_t)8)
#define BERG_CHAIN_MASK ((size_t)0xFFFF)

#define CHECK_ERR(err)                                                         \
    do                                                                         \
    {                                                                          \
        berg_error_t ___err = (err);                                           \
        if (___err < 0)                                                        \
        {                                                                      \
            assert(0);                                                         \
            return ___err;                                                     \
        }                                                                      \
    } while (0);

/* Internal structures */
typedef struct
{
    size_t literal_count;
    uint16_t match_offset;
    size_t match_length;
} berg_token_t;

typedef struct
{
    uint16_t offset;
    size_t length;
} berg_match_result_t;

typedef struct
{
    uint32_t hash_table[BERG_HASH_SIZE];
    uint32_t chain_table[BERG_CHAIN_MASK + 1];
} berg_hash_matcher_t;

/* Little-endian helper functions */
static inline uint16_t berg_read_le16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static inline uint32_t berg_read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static inline void berg_write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value & 0xFF);
    data[1] = (uint8_t)((value >> 8) & 0xFF);
}

static inline void berg_write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value & 0xFF);
    data[1] = (uint8_t)((value >> 8) & 0xFF);
    data[2] = (uint8_t)((value >> 16) & 0xFF);
    data[3] = (uint8_t)((value >> 24) & 0xFF);
}

static uint32_t berg_calculate_hash(const uint8_t *data);
static void berg_reset_matcher(berg_hash_matcher_t *matcher);
static void berg_add_position(berg_hash_matcher_t *matcher,
                              const uint8_t *input,
                              size_t pos,
                              size_t input_size);
static berg_match_result_t berg_find_best_match(berg_hash_matcher_t *matcher,
                                                const uint8_t *input,
                                                size_t pos,
                                                size_t lookahead_size,
                                                size_t input_size);

/* Internal compression/decompression functions */
static inline berg_error_t
berg_compress_raw_internal(const void *input,
                           size_t input_size,
                           void *userdata,
                           berg_write_callback_t write,
                           const berg_config *config);

static berg_error_t berg_decompress_raw_internal(const void *compressed,
                                                 size_t compressed_size,
                                                 void *userdata,
                                                 berg_write_callback_t write,
                                                 size_t original_size);

static size_t berg_encode_varint(uint8_t *output, size_t value);
static berg_error_t berg_decode_varint(const uint8_t *input,
                                       size_t *pos,
                                       size_t size,
                                       size_t *value);

/* Hash matcher implementation */
inline static uint32_t berg_calculate_hash(const uint8_t *data)
{
    uint32_t sequence = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) |
                        (uint32_t)data[2];
    return (sequence * 2654435761UL) >> (32 - 14);
}

inline static void berg_reset_matcher(berg_hash_matcher_t *matcher)
{
    size_t i;

    for (i = 0; i < BERG_HASH_SIZE; i++)
        matcher->hash_table[i] = 0;

    for (i = 0; i < BERG_CHAIN_MASK + 1; i++)
        matcher->chain_table[i] = 0;
}

inline static void berg_add_position(berg_hash_matcher_t *matcher,
                                     const uint8_t *input,
                                     size_t pos,
                                     size_t input_size)
{
    uint32_t hash;
    size_t hash_idx, chain_idx;

    if (pos + 3 > input_size)
        return;

    hash      = berg_calculate_hash(input + pos);
    hash_idx  = hash & (BERG_HASH_SIZE - 1);
    chain_idx = pos & BERG_CHAIN_MASK;

    matcher->chain_table[chain_idx] = matcher->hash_table[hash_idx];
    matcher->hash_table[hash_idx]   = (uint32_t)pos;
}

inline static berg_match_result_t
berg_find_best_match(berg_hash_matcher_t *matcher,
                     const uint8_t *input,
                     size_t pos,
                     size_t lookahead_size,
                     size_t input_size)
{
    berg_match_result_t best = {0, 0};
    uint32_t hash, candidate_pos;
    size_t hash_idx, chain_idx, chain_length, distance, match_len, max_len;
    const uint8_t *current, *candidate;

    if (pos + 3 > input_size || lookahead_size < BERG_MIN_MATCH_LENGTH)
        return best;

    hash          = berg_calculate_hash(input + pos);
    hash_idx      = hash & (BERG_HASH_SIZE - 1);
    candidate_pos = matcher->hash_table[hash_idx];
    chain_length  = 0;
    current       = input + pos;
    max_len =
        (lookahead_size < input_size - pos) ? lookahead_size : input_size - pos;

    while (candidate_pos != 0 && chain_length < BERG_MAX_CHAIN_LENGTH &&
           pos > candidate_pos)
    {
        distance = pos - candidate_pos;

        if (distance > BERG_MAX_DISTANCE)
            break;

        candidate = input + candidate_pos;

        if (candidate[0] == current[0] &&
            (best.length == 0 ||
             candidate[best.length] == current[best.length]))
        {

            match_len = 0;
            while (match_len < max_len &&
                   current[match_len] == candidate[match_len])
            {
                match_len++;
            }

            if (match_len >= BERG_MIN_MATCH_LENGTH && match_len > best.length)
            {
                best.offset = (uint16_t)distance;
                best.length = (uint16_t)match_len;

                if (match_len >= max_len || match_len >= 16)
                    break;
            }
        }

        chain_idx     = candidate_pos & BERG_CHAIN_MASK;
        candidate_pos = matcher->chain_table[chain_idx];
        chain_length++;
    }

    return best;
}

/* Varint encoding/decoding */
inline static size_t berg_encode_varint(uint8_t *output, size_t value)
{
    size_t bytes_written = 0;

    // value -= is_literal ? 3 : 6;

    while (value >= 0x80)
    {
        output[bytes_written++] = (uint8_t)(value | 0x80);
        value >>= 7;
    }
    output[bytes_written++] = (uint8_t)value;

    return bytes_written;
}

inline static berg_error_t
berg_write_varint(berg_write_callback_t write, void *userdata, size_t value)
{
    const int buffer_len = (sizeof(value) * 8 + 7) / 7;
    uint8_t buffer[buffer_len];
    size_t i;
    for (i = 0; i < buffer_len; i++)
        buffer[i] = 0x00;

    size_t varint_size = berg_encode_varint(buffer, value);
    return write(buffer, varint_size, userdata);
}

inline static berg_error_t berg_decode_varint(const uint8_t *input,
                                              size_t *pos,
                                              size_t input_size,
                                              size_t *value)
{
    uint32_t result = 0;
    int32_t shift   = 0;
    uint8_t byte;

    if (*pos >= input_size)
        return BERG_ERROR_CORRUPT_DATA;

    while (*pos < input_size)
    {
        byte = input[(*pos)++];
        result |= (uint32_t)(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0)
        {
            *value = result /* + (is_literal ? 3 : 6) */;
            return BERG_OK;
        }

        shift += 7;
        if (shift >= 32)
            return BERG_ERROR_CORRUPT_DATA;
    }

    return BERG_ERROR_CORRUPT_DATA;
}

/* Token encoding/decoding */
inline static berg_error_t berg_decode_token(const uint8_t *input,
                                             size_t *pos,
                                             size_t input_size,
                                             berg_token_t *pResult)
{
    berg_token_t token;
    uint16_t token_value;

    if (*pos + 2 > input_size)
        return BERG_ERROR_COMPRESSION_FAILED;

    token_value = berg_read_le16(input + *pos);
    *pos += 2;

    const bool extended_literal_count = (token_value & 0xC000) == 0xC000;
    const bool extended_match_length  = (token_value & 0x0003) == 0x0003;

    if (extended_literal_count)
        token.literal_count = 0; /* Will be set from varint */
    else
        token.literal_count = (token_value >> 14) & 0x03;

    token.match_offset = (token_value >> 2) & 0xFFF;
    if (token.match_offset > 0)
    {
        if (extended_match_length)
            token.match_length = 0; /* Will be set from varint */
        else
            token.match_length = (token_value & 0x0003) + BERG_MIN_MATCH_LENGTH;
    }
    else
    {
        token.match_offset = 0;
        token.match_length = 0;
    }

    if (extended_literal_count)
    {
        CHECK_ERR(
            berg_decode_varint(input, pos, input_size, &token.literal_count));

        // Ensure that the literal count is less than the original literal count
        assert(token.literal_count <
               token.literal_count + BERG_MIN_EXTENDED_LITERAL_COUNT);
        token.literal_count += BERG_MIN_EXTENDED_LITERAL_COUNT;
    }

    if (extended_match_length)
    {
        CHECK_ERR(
            berg_decode_varint(input, pos, input_size, &token.match_length));

        // Ensure that the match length is less than the original match length
        assert(token.match_length <
               token.match_length + BERG_MIN_EXTENDED_MATCH_LENGTH);
        token.match_length += BERG_MIN_EXTENDED_MATCH_LENGTH;
    }

    *pResult = token;
    return BERG_OK;
}

inline static berg_error_t berg_encode_token(berg_write_callback_t write,
                                             void *write_userdata,
                                             const uint8_t *literals,
                                             const berg_token_t *pToken)
{
    const berg_token_t token = *pToken;
    uint16_t token_value     = 0;

#ifdef DEBUG
    uint8_t debug_buffer[10];
    size_t debug_pos = 0;
#endif

    assert(write && pToken);

    const bool extended_literal_count =
        token.literal_count > BERG_MAX_DIRECT_LITERAL_COUNT;
    const bool extended_match_length =
        token.match_length > BERG_MAX_DIRECT_MATCH_LENGTH;

    if (extended_literal_count)
        token_value |= 0xC000; // Extended literal count
    else
        token_value |= (uint16_t)(token.literal_count & 0x03) << 14;

    if (token.match_offset > 0)
    {
        token_value |= (uint16_t)(token.match_offset & 0xFFF) << 2;

        if (token.match_length > BERG_MAX_DIRECT_MATCH_LENGTH)
            token_value |= 0x0003; // Extended match length
        else
            token_value |=
                (uint16_t)((token.match_length - BERG_MIN_MATCH_LENGTH) & 0x03);
    }

    CHECK_ERR(write(&token_value, sizeof(token_value), write_userdata));

#ifdef DEBUG
    berg_write_le16(debug_buffer + debug_pos, token_value);
    debug_pos += sizeof(token_value);
#endif

    // write extended literal count
    if (extended_literal_count)
    {
        assert(token.literal_count >= BERG_MIN_EXTENDED_LITERAL_COUNT);
        CHECK_ERR(berg_write_varint(write,
                                    write_userdata,
                                    token.literal_count -
                                        BERG_MIN_EXTENDED_LITERAL_COUNT));

#ifdef DEBUG
        debug_pos += berg_encode_varint(debug_buffer + debug_pos,
                                        token.literal_count -
                                            BERG_MIN_EXTENDED_LITERAL_COUNT);
#endif
    }

    // write extended match length
    if (extended_match_length)
    {
        assert(token.match_length >= BERG_MIN_EXTENDED_MATCH_LENGTH);
        CHECK_ERR(berg_write_varint(write,
                                    write_userdata,
                                    token.match_length -
                                        BERG_MIN_EXTENDED_MATCH_LENGTH));

#ifdef DEBUG
        debug_pos += berg_encode_varint(debug_buffer + debug_pos,
                                        token.match_length -
                                            BERG_MIN_EXTENDED_MATCH_LENGTH);
#endif
    }

    CHECK_ERR(write(literals, token.literal_count, write_userdata));

#ifdef DEBUG
    // decode
    berg_token_t token2;
    debug_pos = 0;
    CHECK_ERR(berg_decode_token(
        debug_buffer, &debug_pos, sizeof(debug_buffer), &token2));
    // assert(token2.literal_count == token.literal_count);
    // assert(token2.match_offset == token.match_offset);
    // assert(token2.match_length == token.match_length);
    if ((token2.literal_count != token.literal_count) ||
        (token2.match_offset != token.match_offset) ||
        (token2.match_length != token.match_length))
        berg_encode_token(write, write_userdata, literals, pToken);
#endif

    return BERG_OK;
}

/* Public C API Implementation */

berg_config berg_get_default_config(void)
{
    berg_config config;
    config.lookahead_size = BERG_DEFAULT_LOOKAHEAD_SIZE;
    return config;
}

size_t berg_estimate_max_compressed_size(size_t input_size)
{
    /* Worst case: every byte becomes a literal token + header + checksum */
    return input_size + (input_size / 2) + 64;
}

berg_error_t berg_compress(const void *input,
                           size_t input_size,
                           void *output,
                           size_t output_capacity,
                           size_t *compressed_size,
                           const berg_config *config)
{
    uint8_t *output_data = (uint8_t *)output;
    size_t output_pos    = 0;
    size_t raw_compressed_size;
    uint32_t data_checksum;
    berg_error_t result;

    if (!input || !output || !compressed_size || input_size == 0)
    {
        return BERG_ERROR_INVALID_PARAM;
    }

    if (output_capacity < 12)
    { /* Minimum for header + checksum */
        return BERG_ERROR_BUFFER_TOO_SMALL;
    }

    /* Write header */
    if (output_pos + 8 > output_capacity)
        return BERG_ERROR_BUFFER_TOO_SMALL;
    output_data[output_pos++] = 'B';
    output_data[output_pos++] = 'E';
    output_data[output_pos++] = 'R';
    output_data[output_pos++] = 'G';

    /* Write original size (little-endian) */
    berg_write_le32(output_data + output_pos, (uint32_t)input_size);
    output_pos += 4;

    /* Compress data using raw compression */
    result = berg_compress_raw(input,
                               input_size,
                               output_data + output_pos,
                               output_capacity - output_pos -
                                   4, /* Reserve 4 bytes for checksum */
                               &raw_compressed_size,
                               config);
    if (result != BERG_OK)
        return result;

    output_pos += raw_compressed_size;

    /* Calculate and write checksum */
    if (output_pos + 4 > output_capacity)
        return BERG_ERROR_BUFFER_TOO_SMALL;
    data_checksum = berg_calculate_crc32(0, input, input_size);
    berg_write_le32(output_data + output_pos, data_checksum);
    output_pos += 4;

    *compressed_size = output_pos;
    return BERG_OK;
}

berg_error_t berg_decompress(const void *compressed,
                             size_t compressed_size,
                             void *output,
                             size_t output_capacity,
                             size_t *decompressed_size)
{
    const uint8_t *input_data = (const uint8_t *)compressed;
    size_t original_size;
    uint32_t stored_checksum, calculated_checksum;
    berg_error_t result;

    if (!compressed || !decompressed_size || compressed_size < 12 ||
        ((output_capacity == 0) != (output == NULL)))
        return BERG_ERROR_INVALID_PARAM;

    /* Check magic number */
    if (input_data[0] != 'B' || input_data[1] != 'E' || input_data[2] != 'R' ||
        input_data[3] != 'G')
        return BERG_ERROR_CORRUPT_DATA;

    /* Read original size */
    original_size = berg_read_le32(input_data + 4);

    *decompressed_size = original_size;

    if (!output_capacity && !output)
        return BERG_OK_NO_OUTPUT;

    if (original_size > output_capacity)
        return BERG_ERROR_BUFFER_TOO_SMALL;

    /* Read stored checksum from end */
    stored_checksum = berg_read_le32(input_data + compressed_size - 4);

    /* Decompress data using raw decompression */
    result = berg_decompress_raw(input_data + 8, /* Skip 8-byte header */
                                 compressed_size -
                                     12, /* Exclude header + checksum */
                                 output,
                                 output_capacity,
                                 original_size,
                                 decompressed_size);
    if (result < 0)
        return result;

    /* Verify checksum */
    calculated_checksum = berg_calculate_crc32(0, output, *decompressed_size);
    if (stored_checksum != calculated_checksum)
        return BERG_ERROR_CORRUPT_DATA;

    return BERG_OK;
}

/* Placeholder implementations for remaining API functions */
static inline berg_error_t
berg_compress_raw_internal(const void *input,
                           size_t input_size,
                           void *userdata,
                           berg_write_callback_t write,
                           const berg_config *config)
{
    const uint8_t *input_data = (const uint8_t *)input;
    berg_config cfg;
    berg_hash_matcher_t matcher;
    size_t pos = 0, literal_start;
    berg_token_t token;
    berg_match_result_t match;
    uint16_t token_value;

    if (!input || !write || input_size == 0)
        return BERG_ERROR_INVALID_PARAM;

    cfg = config ? *config : berg_get_default_config();
    berg_reset_matcher(&matcher);

    /* Compress data (no header) */
    while (pos < input_size)
    {

        token.literal_count = 0;
        token.match_offset  = 0;
        token.match_length  = 0;
        literal_start       = pos;

        /* Accumulate literals and find matches */
        while (pos < input_size)
        {
            size_t lookahead_size = (cfg.lookahead_size < input_size - pos)
                                        ? cfg.lookahead_size
                                        : input_size - pos;

            match = berg_find_best_match(
                &matcher, input_data, pos, lookahead_size, input_size);
            berg_add_position(&matcher, input_data, pos, input_size);

            if (match.offset > 0 && match.length >= BERG_MIN_MATCH_LENGTH)
            {
                token.match_offset = match.offset;
                token.match_length = match.length;
                break;
            }
            else
            {
                token.literal_count++;
                pos++;
            }
        }

        assert(token.literal_count <= input_size - literal_start);
        assert(token.match_offset <= pos);

        /* Encode token */
        CHECK_ERR(berg_encode_token(
            write, userdata, input_data + literal_start, &token));

        /* Advance past match */
        if (token.match_offset > 0)
        {
            size_t i;
            for (i = 0; i < token.match_length; i++)
                berg_add_position(&matcher, input_data, pos + i, input_size);
            pos += token.match_length;
        }
    }

    if (pos != input_size)
        return BERG_ERROR_CORRUPT_DATA;

    return BERG_OK;
}

typedef struct raw_internal_data_t
{
    uint8_t *buffer;
    size_t buffer_size;
    size_t buffer_pos;
} raw_internal_data_t;

static inline berg_error_t
write_internal_callback(const void *data, size_t size, void *userdata)
{
    raw_internal_data_t *d = (raw_internal_data_t *)userdata;
    if (d->buffer_pos + size > d->buffer_size)
        return BERG_ERROR_BUFFER_TOO_SMALL;
    memcpy(d->buffer + d->buffer_pos, data, size);
    d->buffer_pos += size;
    return BERG_OK;
}

berg_error_t berg_compress_raw(const void *input,
                               size_t input_size,
                               void *output,
                               size_t output_capacity,
                               size_t *compressed_size,
                               const berg_config *config)
{
    raw_internal_data_t data;
    data.buffer      = (uint8_t *)output;
    data.buffer_size = output_capacity;
    data.buffer_pos  = 0;

    CHECK_ERR(berg_compress_raw_internal(
        input, input_size, &data, write_internal_callback, config));

    *compressed_size = data.buffer_pos;
    return BERG_OK;
}

static inline berg_error_t
berg_decompress_raw_internal(const void *compressed,
                             size_t compressed_size,
                             void *userdata,
                             berg_write_callback_t write,
                             size_t original_size)
{
    const uint8_t *input_data = (const uint8_t *)compressed;
    size_t pos = 0, output_pos = 0;
    berg_token_t token;
    uint16_t token_value;
    uint32_t extended_value;
    size_t i, copy_start, copy_offset;

    uint8_t window[BERG_DEFAULT_WINDOW_SIZE] = {0};

    if (!compressed || !write || compressed_size == 0)
        return BERG_ERROR_INVALID_PARAM;

    /* Decompress data (no header) */
    while (pos < compressed_size && output_pos < original_size)
    {
        if (pos + 2 > compressed_size)
            return BERG_ERROR_CORRUPT_DATA;

        /* Read token */
        CHECK_ERR(berg_decode_token(input_data, &pos, compressed_size, &token));

        /* Copy literals */
        if (pos + token.literal_count > compressed_size)
            return BERG_ERROR_CORRUPT_DATA;

        CHECK_ERR(write(input_data + pos, token.literal_count, userdata));
        size_t i;
        for (i = 0; i < token.literal_count; i++)
            window[(output_pos + i) % BERG_DEFAULT_WINDOW_SIZE] =
                input_data[pos + i];

        pos += token.literal_count;

        assert(token.literal_count <= original_size - output_pos);
        output_pos += token.literal_count;

        /* Copy match */
        if (token.match_offset > 0)
        {
            if (token.match_offset > output_pos)
                return BERG_ERROR_CORRUPT_DATA;

            copy_start = output_pos - token.match_offset;

            for (i = 0; i < token.match_length; i++)
            {
                copy_offset =
                    i < token.match_offset ? i : (i % token.match_offset);
                uint8_t *data = &window[(copy_start + copy_offset) %
                                        BERG_DEFAULT_WINDOW_SIZE];
                CHECK_ERR(write(data, 1, userdata));
                window[(output_pos + i) % BERG_DEFAULT_WINDOW_SIZE] = *data;
            }
            assert(output_pos <= original_size - token.match_length);
            output_pos += token.match_length;
        }
    }

    /* Verify size */
    if (output_pos != original_size)
        return BERG_ERROR_CORRUPT_DATA;

    return BERG_OK;
}

berg_error_t berg_decompress_raw(const void *compressed,
                                 size_t compressed_size,
                                 void *output,
                                 size_t output_capacity,
                                 size_t original_size,
                                 size_t *decompressed_size)
{
    raw_internal_data_t data;
    data.buffer      = (uint8_t *)output;
    data.buffer_size = output_capacity;
    data.buffer_pos  = 0;

    CHECK_ERR(berg_decompress_raw_internal(compressed,
                                           compressed_size,
                                           &data,
                                           write_internal_callback,
                                           original_size));

    *decompressed_size = data.buffer_pos;
    if (original_size != data.buffer_pos)
        return BERG_ERROR_CORRUPT_DATA;
    return BERG_OK;
}

typedef struct raw_internal_data_it_t
{
    uint8_t *buffer;
    size_t pos;
    size_t buffer_size;
    size_t total_written;

    berg_write_callback_t callback;
    void *user_data;
} raw_internal_data_it_t;

// write into a ring buffer, when ring buffer is full, flush it with callback of
// raw_internal_data_it_t
static inline berg_error_t
write_internal_it_callback(const void *data, size_t size, void *userdata)
{
    raw_internal_data_it_t *d = (raw_internal_data_it_t *)userdata;
    const uint8_t *input_data = (const uint8_t *)data;
    size_t remaining          = size;

    while (remaining > 0)
    {
        const size_t available_space = d->buffer_size - d->pos;
        const size_t to_write =
            remaining < available_space ? remaining : available_space;

        // If buffer is full, flush it first
        if (to_write == 0)
        {
            assert(d->pos == d->buffer_size);
            CHECK_ERR(d->callback(d->buffer, d->buffer_size, d->user_data));
            d->pos = 0;
            continue;
        }

        memcpy(d->buffer + d->pos, input_data, to_write);
        d->pos += to_write;
        d->total_written += to_write;
        input_data += to_write;
        remaining -= to_write;
    }

    // if buffer is not empty, flush it
    if (d->pos != 0)
    {
        CHECK_ERR(d->callback(d->buffer, d->pos, d->user_data));
        d->pos = 0;
    }

    return BERG_OK;
}

berg_error_t berg_compress_raw_stream(const void *input,
                                      size_t input_size,
                                      berg_write_callback_t callback,
                                      void *user_data,
                                      void *buffer,
                                      size_t buffer_size,
                                      const berg_config *config)
{
    raw_internal_data_it_t data;
    data.buffer        = (uint8_t *)buffer;
    data.buffer_size   = buffer_size;
    data.total_written = 0;
    data.pos           = 0;
    data.callback      = callback;
    data.user_data     = user_data;

    CHECK_ERR(berg_compress_raw_internal(
        input, input_size, &data, write_internal_it_callback, config));

    return BERG_OK;
}

berg_error_t berg_decompress_raw_stream(const void *compressed,
                                        size_t compressed_size,
                                        size_t original_size,
                                        berg_write_callback_t callback,
                                        void *user_data,
                                        void *buffer,
                                        size_t buffer_size)
{
    raw_internal_data_it_t data;
    data.buffer        = (uint8_t *)buffer;
    data.buffer_size   = buffer_size;
    data.total_written = 0;
    data.pos           = 0;
    data.callback      = callback;
    data.user_data     = user_data;

    CHECK_ERR(berg_decompress_raw_internal(compressed,
                                           compressed_size,
                                           &data,
                                           write_internal_it_callback,
                                           original_size));

    if (original_size != data.total_written)
        return BERG_ERROR_CORRUPT_DATA;

    return BERG_OK;
}

berg_error_t berg_compress_stream(const void *input,
                                  size_t input_size,
                                  berg_write_callback_t callback,
                                  void *user_data,
                                  void *buffer,
                                  size_t buffer_size,
                                  const berg_config *config)
{
    uint8_t header_buf[8];
    uint32_t checksum;

    if (!input || !callback || !buffer)
    {
        return BERG_ERROR_INVALID_PARAM;
    }

    /* Write header: magic number and original size */
    header_buf[0] = 'B';
    header_buf[1] = 'E';
    header_buf[2] = 'R';
    header_buf[3] = 'G';
    berg_write_le32(header_buf + 4, (uint32_t)input_size);
    CHECK_ERR(callback(header_buf, 8, user_data));

    /* Stream the compressed raw data */
    CHECK_ERR(berg_compress_raw_stream(
        input, input_size, callback, user_data, buffer, buffer_size, config));

    /* Calculate and write checksum of original data */
    checksum = berg_calculate_crc32(0, input, input_size);
    berg_write_le32(header_buf, checksum); // Reuse buffer
    CHECK_ERR(callback(header_buf, 4, user_data));

    return BERG_OK;
}

/**
 * @brief State for streaming decompression with checksum verification.
 */
typedef struct decompress_stream_data_t
{
    berg_write_callback_t user_callback;
    void *user_data;
    uint32_t running_crc;
} decompress_stream_data_t;

/**
 * @brief Internal callback that updates a running CRC32 checksum and then
 * forwards the data to the user-provided callback.
 */
static inline berg_error_t
write_and_checksum_callback(const void *data, size_t size, void *userdata)
{
    decompress_stream_data_t *stream_data =
        (decompress_stream_data_t *)userdata;
    stream_data->running_crc =
        berg_calculate_crc32(stream_data->running_crc, data, size);
    return stream_data->user_callback(data, size, stream_data->user_data);
}

berg_error_t berg_decompress_stream(const void *compressed,
                                    size_t compressed_size,
                                    berg_write_callback_t callback,
                                    void *user_data,
                                    void *buffer,
                                    size_t buffer_size)
{
    const uint8_t *input_data = (const uint8_t *)compressed;
    size_t original_size;
    uint32_t stored_checksum;
    berg_error_t result;
    decompress_stream_data_t stream_state;

    if (!compressed || !callback || !buffer || compressed_size < 12)
    {
        return BERG_ERROR_INVALID_PARAM;
    }

    /* Check magic number */
    if (input_data[0] != 'B' || input_data[1] != 'E' || input_data[2] != 'R' ||
        input_data[3] != 'G')
    {
        return BERG_ERROR_CORRUPT_DATA;
    }

    /* Read original size and stored checksum */
    original_size   = berg_read_le32(input_data + 4);
    stored_checksum = berg_read_le32(input_data + compressed_size - 4);

    /* Set up the state for the checksum-calculating callback */
    stream_state.user_callback = callback;
    stream_state.user_data     = user_data;
    stream_state.running_crc   = 0;

    /* Decompress raw data, streaming output via the checksum wrapper */
    result = berg_decompress_raw_stream(input_data + 8, /* Skip 8-byte header */
                                        compressed_size -
                                            12, /* Exclude header + checksum */
                                        original_size,
                                        write_and_checksum_callback,
                                        &stream_state,
                                        buffer,
                                        buffer_size);
    if (result < 0)
    {
        return result;
    }

    /* Verify checksum of the decompressed data */
    if (stream_state.running_crc != stored_checksum)
    {
        return BERG_ERROR_CORRUPT_DATA;
    }

    return BERG_OK;
}

#ifdef __cplusplus
}
#endif