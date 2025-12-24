#include "zabato/random.hpp"

#ifdef _WIN32
#define _CRT_RAND_S
#include <stdlib.h>
#include <windows.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif

namespace zabato
{

uint64_t random::rand()
{
    uint64_t r;
    buf(&r, sizeof(r));
    return r;
}

void random::buf(void *buf, size_t size)
{
    uint8_t *p = (uint8_t *)buf;
#ifdef _WIN32
    for (size_t i = 0; i < size; ++i)
    {
        unsigned int r;
        if (rand_s(&r) == 0)
            p[i] = (uint8_t)r;
        else
            p[i] = (uint8_t)(rand() & 0xFF);
    }
#else
    static FILE *f = fopen("/dev/urandom", "rb");
    bool seeded    = false;
    if (f)
    {
        if (fread(p, 1, size, f) == size)
            seeded = true;
    }

    if (!seeded)
    {
        for (size_t i = 0; i < size; i++)
            p[i] = (uint8_t)rand();
    }
#endif
}

void random::buf(buffer &b) { buf(b.data(), b.size()); }
} // namespace zabato