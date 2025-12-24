#include <zabato/random.hpp>
#include <zabato/time.hpp>
#include <zabato/uuid.hpp>

namespace zabato
{

uuid uuid::generate()
{
    uuid u;

    uint64_t ms = time::system_now().as_milliseconds_uint();

    // Fill bytes 0-5 with timestamp (Big Endian)
    uint8_t *p = u.data();
    p[0]       = (ms >> 40) & 0xFF;
    p[1]       = (ms >> 32) & 0xFF;
    p[2]       = (ms >> 24) & 0xFF;
    p[3]       = (ms >> 16) & 0xFF;
    p[4]       = (ms >> 8) & 0xFF;
    p[5]       = ms & 0xFF;

    // Randomness (10 bytes: 6-15)
    random::buf(p + 6, 10);

    // Set Version (0111 = 7) in byte 6 high nibble
    p[6] = (p[6] & 0x0F) | 0x70;

    // Set Variant (10xx) in byte 8 high nibble
    p[8] = (p[8] & 0x3F) | 0x80;

    return u;
}

static uint8_t hex_to_byte(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0xFF;
}

bool uuid::try_parse(string_view sv, uuid &out)
{
    if (sv.size() != 36 && sv.size() != 32)
        return false;

    uint8_t *data    = out.data();
    int digit_count  = 0;
    int data_idx     = 0;
    bool high_nibble = true;

    for (size_t i = 0; i < sv.size(); ++i)
    {
        char c = sv[i];
        if (c == '-')
            continue;

        uint8_t b = hex_to_byte(c);
        if (b == 0xFF)
            return false;

        if (high_nibble)
        {
            data[data_idx] = b << 4;
            high_nibble    = false;
        }
        else
        {
            data[data_idx] |= b;
            data_idx++;
            high_nibble = true;
        }
        digit_count++;
    }

    return digit_count == 32;
}

uuid uuid::parse(string_view sv)
{
    uuid u;
    if (try_parse(sv, u))
        return u;
    return uuid();
}

string uuid::to_string() const
{
    char buffer[37];
    to_chars(buffer);
    return string(buffer);
}

bool uuid::to_chars(span<char> buffer) const
{
    if (buffer.size() < 36)
        return false;

    const char *hex = "0123456789abcdef";
    int b           = 0;
    for (int i = 0; i < 16; ++i)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            buffer[b++] = '-';
        buffer[b++] = hex[(m_data[i] >> 4) & 0xF];
        buffer[b++] = hex[m_data[i] & 0xF];
    }

    if (buffer.size() > 36)
        buffer[b] = '\0';

    return true;
}

} // namespace zabato
