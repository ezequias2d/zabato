#include <zabato/time.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

namespace zabato
{

time time::now()
{
#ifdef _WIN32
    static const int64_t frequency = []()
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        return freq.QuadPart;
    }();

    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);

    int64_t sec = count.QuadPart / frequency;
    int64_t rem = count.QuadPart % frequency;

    uint64_t ns = (static_cast<uint64_t>(sec) * 1000000000ULL) +
                  ((static_cast<uint64_t>(rem) * 1000000000ULL) / frequency);

    return time(ns);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    uint64_t ns = static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL +
                  static_cast<uint64_t>(ts.tv_nsec);

    return time(ns);
#endif
}

time time::system_now()
{
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER ull;
    ull.LowPart  = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;

    uint64_t ns_100 = ull.QuadPart - 116444736000000000ULL;
    return time(ns_100 * 100);
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    uint64_t ns = static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL +
                  static_cast<uint64_t>(ts.tv_nsec);

    return time(ns);
#endif
}

} // namespace zabato
