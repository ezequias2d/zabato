#include <filesystem>
#include <stdio.h>

#include <zabato/fs.hpp>
#include <zabato/path.hpp>
#include <zabato/string.hpp>

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

#if defined(_WIN32)
#include <windows.h>
static string to_utf8(const wchar_t *wstr)
{
    if (!wstr)
        return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size <= 0)
        return "";
    string result;
    result.resize(size - 1);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), size, NULL, NULL);
    return result;
}

static std::wstring to_utf16(const char *str)
{
    if (!str)
        return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (size <= 0)
        return L"";
    std::wstring result;
    result.resize(size - 1);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, result.data(), size);
    return result;
}
#endif

string get_current_dir_path(void)
{
    std::error_code ec;
    auto p = std_fs::current_path(ec);
    if (ec)
        return "";
#if defined(_WIN32)
    return normalize(to_utf8(p.c_str()));
#else
    return normalize(p.string().c_str());
#endif
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
    wchar_t wbuffer[4096];
    DWORD len = GetModuleFileNameW(
        NULL, wbuffer, (DWORD)(sizeof(wbuffer) / sizeof(wchar_t)));
    if (len == 0 || len == (sizeof(wbuffer) / sizeof(wchar_t)))
        return "";
    return normalize(to_utf8(wbuffer));
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

bool file::read_all(vector<uint8_t> &out)
{
    size_t cur = tell();
    seek(0, origin::end);
    size_t end = tell();
    seek(cur, origin::begin);

    size_t size = end - cur;
    out.resize(size);
    return read(out) == size;
}

result<string> file_system::get_native_path(string_view path)
{
    return report_error(error_code::not_supported);
}

result<string> file_system::get_virtual_path(string_view native_path)
{
    return report_error(error_code::not_supported);
}

string file_system::read_all_text(string_view path)
{
    auto file = open(path, open_mode::read);
    if (!file)
        return "";
    vector<uint8_t> buf;
    file->read_all(buf);
    file->close();
    delete file;
    return string((const char *)buf.data(), buf.size());
}

bool file_system::write_all_text(string_view path, string_view text)
{
    auto file = open(path, open_mode::write | open_mode::truncate);
    if (!file)
        return false;
    size_t writted = file->write({(const uint8_t *)text.data(), text.size()});
    file->close();
    delete file;
    return writted == text.size();
}

vector<uint8_t> file_system::read_all_bytes(string_view path)
{
    auto file = open(path, open_mode::read);
    if (!file)
        return {};
    vector<uint8_t> buf;
    file->read_all(buf);
    file->close();
    delete file;
    return buf;
}

bool file_system::write_all_bytes(string_view path, span<const uint8_t> bytes)
{
    auto file = open(path, open_mode::write | open_mode::truncate);
    if (!file)
        return false;
    size_t writted = file->write(bytes);
    file->close();
    delete file;
    return writted == bytes.size();
}

void virtual_fs::mount(const string &mount_point, file_system *fs)
{
    string mp = normalize(mount_point);

    // Ensure mount point is absolute for consistency
    if (!is_absolute(mp))
    {
        string tmp = "/";
        tmp += mp;
        mp = normalize(tmp);
    }

    // Remove trailing slash for consistency in matching (unless root)
    if (mp.length() > 1 && mp.back() == '/')
    {
        mp.pop_back();
    }

    m_mounts.push_back({mp, fs});

    // Sort by length descending ensures longest prefix match wins
    // e.g. "/usr/bin" is checked before "/usr"
    sort(m_mounts.begin(),
         m_mounts.end(),
         [](const auto &a, const auto &b)
         { return a.path.length() > b.path.length(); });
}

void virtual_fs::unmount(const string &mount_point)
{
    string mp = normalize(mount_point);
    // Ensure normalization matches mount logic
    if (!is_absolute(mp))
    {
        string tmp = "/";
        tmp += mp;
        mp = normalize(tmp);
    }
    if (mp.length() > 1 && mp.back() == '/')
    {
        mp.pop_back();
    }

    for (auto it = m_mounts.begin(); it != m_mounts.end(); ++it)
    {
        if (it->path == mp)
        {
            // Remove the mount point by copying the last element over it
            // then popping the back. The order is destroyed, so we re-sort.
            *it = m_mounts.back();
            m_mounts.pop_back();

            // Restore sort invariant (Longest path first)
            sort(m_mounts.begin(),
                 m_mounts.end(),
                 [](const auto &a, const auto &b)
                 { return a.path.length() > b.path.length(); });
            return;
        }
    }
}

file *virtual_fs::open(string_view path, open_mode mode)
{
    auto [fs, mount_point, relative_path] = resolve(path);
    if (!fs)
        return nullptr;
    return fs->open(relative_path, mode);
}

bool virtual_fs::exists(string_view path)
{
    auto [fs, mount_point, relative_path] = resolve(path);
    if (fs && fs->exists(relative_path))
        return true;

    string p = normalize(path);
    if (!is_absolute(p))
    {
        p.prepend("/");
        p = normalize(p);
    }

    if (p == "/")
        return true;

    // Check if it is a parent of any mount
    for (const auto &mount : m_mounts)
    {
        // If mount path starts with p and has more chars, and p is a prefix
        // component
        if (mount.path.length() > p.length() && mount.path.starts_with(p))
        {
            // Check if p is a full directory component of mount.path
            // path: /a/b, mount: /a/b/c -> starts_with true. mount[len] ==
            // '/' mount: /a/bc -> starts_with true. mount[len] == 'c' !=
            // '/' (unless p ends in /)
            if (mount.path[p.length()] == '/')
                return true;
        }
    }
    return false;
}

vector<file_info> virtual_fs::ls(string_view path)
{
    auto p = normalize(path);
    if (!is_absolute(p))
    {
        p.prepend("/");
        p = normalize(p);
    }

    auto [fs, mount_point, relative_path] = resolve(path);
    vector<file_info> files;
    if (fs)
        files = move(fs->ls(relative_path));

    for (const auto &mount : m_mounts)
    {
        if (mount.path.starts_with(p) && mount.path.length() > p.length())
        {
            string_view remaining_sv =
                string_view(mount.path).substr(p.length());

            if (remaining_sv.starts_with('/'))
                remaining_sv.remove_prefix(1);

            if (remaining_sv.empty())
                continue;

            auto pos = remaining_sv.find(PATH_SEP);
            if (pos == string::npos)
                pos = remaining_sv.length();

            string_view name_sv = remaining_sv.substr(0, pos);

            bool exists = false;
            for (const auto &f : files)
            {
                if (f.name == name_sv)
                {
                    exists = true;
                    break;
                }
            }
            if (exists)
                continue;

            file_info info;
            info.name               = name_sv;
            info.size               = 0;
            info.last_modified_time = 0;
            info.is_dir             = true;
            info.is_read_only       = true;
            files.push_back(info);
        }
    }

    return files;
}

file_info virtual_fs::get_info(string_view path)
{
    auto [fs, mount_point, relative_path] = resolve(path);
    if (fs)
    {
        file_info info = fs->get_info(relative_path);
        if (!info.name.empty())
            return info;
    }

    // If not resolved to a physical FS, check if it's a virtual dir
    if (exists(path)) // This now checks virtual dirs too
    {
        file_info fi;
        string p = normalize(path);
        // extracting name from path
        size_t last_slash = p.rfind('/');
        if (last_slash != string::npos && last_slash < p.length() - 1)
            fi.name = p.substr(last_slash + 1);
        else if (p == "/")
            fi.name = "/";
        else
            fi.name = p;

        fi.is_dir             = true;
        fi.size               = 0;
        fi.last_modified_time = 0;
        fi.is_read_only       = true;
        return fi;
    }

    return {};
}

bool virtual_fs::is_dir(string_view path)
{
    // Root of VFS is always a dir
    if (path == "/" || path.empty())
        return true;

    auto [fs, mount_point, relative_path] = resolve(path);
    if (fs && fs->is_dir(relative_path))
        return true;

    string p = normalize(path);
    if (!is_absolute(p))
    {
        p.prepend("/");
        p = normalize(p);
    }

    // Check if it is a parent of any mount (virtual directory)
    for (const auto &mount : m_mounts)
    {
        if (mount.path.length() > p.length() && mount.path.starts_with(p))
        {
            if (mount.path[p.length()] == '/')
                return true;
        }
    }
    return false;
}

bool virtual_fs::is_file(string_view path)
{
    auto [fs, mount_point, relative_path] = resolve(path);
    return fs && fs->is_file(relative_path);
}

bool virtual_fs::is_read_only(string_view path)
{
    auto [fs, mount_point, relative_path] = resolve(path);
    return fs ? fs->is_read_only(relative_path) : true;
}

bool virtual_fs::mkdir(string_view path)
{
    auto [fs, mount_point, relative_path] = resolve(path);
    return fs && fs->mkdir(relative_path);
}

bool virtual_fs::remove(string_view path)
{
    auto [fs, mount_point, relative_path] = resolve(path);
    return fs && fs->remove(relative_path);
}

bool virtual_fs::rename(string_view old_path, string_view new_path)
{
    auto [fs_old, mp_old, rel_old] = resolve(old_path);
    auto [fs_new, mp_new, rel_new] = resolve(new_path);

    // Renaming across different file systems is not supported natively.
    // It would require copy + delete.
    if (fs_old && fs_new && fs_old == fs_new)
    {
        return fs_old->rename(rel_old, rel_new);
    }
    return false;
}

result<string> virtual_fs::get_native_path(string_view path)
{
    auto [fs, mount_point, relative_path] = resolve(path);
    if (!fs)
        return report_error(error_code::file_not_found);
    return fs->get_native_path(relative_path);
}

result<string> virtual_fs::get_virtual_path(string_view native_path)
{
    for (const auto &mount : m_mounts)
    {
        if (mount.fs)
        {
            auto res = mount.fs->get_virtual_path(native_path);
            if (!res.has_error())
            {
                string relative = res.value;
                string mp       = mount.path;

                if (mp == "/")
                {
                    if (relative.starts_with("/"))
                        return relative;
                    string res = "/";
                    res.append(relative);
                    return res;
                }

                if (relative.empty())
                    return mp;

                if (relative.starts_with("/"))
                {
                    string res = mp;
                    res.append(relative);
                    return res;
                }

                string res = mp;
                res.append("/");
                res.append(relative);
                return res;
            }
        }
    }
    return report_error(error_code::file_not_found);
}

tuple<file_system *, string, string> virtual_fs::resolve(string_view path) const
{
    string normalized = normalize(path);

    if (!is_absolute(normalized))
    {
        normalized.prepend("/");
        normalized = normalize(normalized);
    }

    const MountPoint *longest_mount = nullptr;
    size_t longest_match            = 0;

    for (const auto &mp : m_mounts)
    {
        if (normalized.starts_with(mp.path))
        {
            size_t len = mp.path.length();

            bool is_root_mount = (len == 1 && mp.path[0] == '/');
            bool exact_match   = (normalized.length() == len);
            bool prefix_match =
                (normalized.length() > len && normalized[len] == '/');

            if (is_root_mount || exact_match || prefix_match)
            {
                if (len > longest_match) // Prefer longest specific mount
                {
                    longest_mount = &mp;
                    longest_match = len;
                }
            }
        }
    }

    if (longest_mount)
    {
        // Strip the mount point from the path
        string_view sub_path;
        if (longest_mount->path.length() == 1 && longest_mount->path[0] == '/')
            sub_path = normalized;
        else
            sub_path =
                string_view(normalized).substr(longest_mount->path.length());

        if (sub_path.empty())
            return {longest_mount->fs, longest_mount->path, "/"};

        if (sub_path.front() != '/')
        {
            string res = "/";
            res.reserve(sub_path.length() + 1);
            res += sub_path;
            return {longest_mount->fs, longest_mount->path, res};
        }

        return {longest_mount->fs, longest_mount->path, string(sub_path)};
    }

    return {nullptr, "", ""};
}
} // namespace zabato::fs