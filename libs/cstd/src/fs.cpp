#include "zabato/path.hpp"
#include "zabato/string.hpp"
#include <filesystem>
#include <stdio.h>
#include <zabato/fs.hpp>

#if defined(__linux__)
#include <limits.h>
#include <unistd.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#include <limits.h>
#include <sys/sysctl.h>
#elif defined(__APPLE__)
#include <limits.h>
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace std_fs = std::filesystem;

namespace zabato::fs
{
string get_current_dir_path(void)
{
    std::error_code ec;
    auto p = std_fs::current_path(ec);
    if (ec)
        return "";
    return normalize(p.c_str());
}

string get_exe_path(void)
{
    char buffer[4096];
    int32_t size = sizeof(buffer);

#if defined(__linux__)
    size = readlink("/proc/self/exe", buffer, size);
    if (size == -1)
        return "";
    return normalize(string(buffer, size));

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
    int mib[4];
    mib[0]    = CTL_KERN;
    mib[1]    = KERN_PROC;
    mib[2]    = KERN_PROC_PATHNAME;
    mib[3]    = getpid();
    size_t cb = size;
    if (sysctl(mib, 4, buffer, &cb, NULL, 0) != 0)
        return "";
    return normalize(string(buffer, cb));

#elif defined(__APPLE__)
    if (_NSGetExecutablePath(buffer, &size) != 0)
        return "";
    return normalize(string(buffer, size));

#elif defined(_WIN32)
    DWORD len = GetModuleFileNameA(NULL, buffer, (DWORD)size);
    if (len == 0 || len == size)
        return "";
    return normalize(string(buffer, len));
#else
#error "Unsupported platform"
#endif
}

string get_exe_dir_path(void)
{
    string path = get_exe_path();
    if (path.empty())
        return "";

    size_t last_slash = path.rfind('/');
    if (last_slash != string::npos)
        return path.substr(0, last_slash);
    return "";
}

} // namespace zabato::fs