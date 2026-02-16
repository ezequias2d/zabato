#pragma once

#include <zabato/error.hpp>
#include <zabato/path.hpp>
#include <zabato/span.hpp>
#include <zabato/stream.hpp>
#include <zabato/string.hpp>
#include <zabato/tuple.hpp>
#include <zabato/vector.hpp>

namespace zabato::fs
{
string get_current_dir_path(void);
string get_exe_path(void);
string get_exe_dir_path(void);

/**
 * @typedef origin
 * @brief Specifies the reference point for file seeking.
 */
using origin = zabato::origin;

/**
 * @brief Abstract interface representing an open file.
 *
 * Provides methods for reading, writing, and seeking within a file.
 * Concrete implementations handle the specifics of the storage medium.
 */
class file : public zabato::stream
{
public:
    virtual ~file() = default;

    /**
     * @brief Closes the file, releasing any resources.
     */
    virtual void close() = 0;

    /**
     * @brief Checks if the file is closed.
     * @return True if the file is closed, false otherwise.
     */
    virtual bool is_closed() const = 0;

    /**
     * @brief Reads the entire file into a vector.
     * @param out The vector to store the file contents.
     * @return True if successful, false otherwise.
     */
    bool read_all(vector<uint8_t> &out)
    {
        size_t cur = tell();
        seek(0, origin::end);
        size_t end = tell();
        seek(cur, origin::begin);

        size_t size = end - cur;
        out.resize(size);
        return read(out) == size;
    }
};

/**
 * @struct file_info
 * @brief Contains metadata about a file or directory.
 */
struct file_info
{
    string name;       ///< The name of the file/directory.
    uint64_t size;     ///< The size of the file in bytes.
    bool is_dir;       ///< True if this is a directory.
    bool is_read_only; ///< True if the item is read-only.
};

/**
 * @enum open_mode
 * @brief Bitmask flags for opening files.
 */
enum class open_mode
{
    read     = 1 << 0, ///< Open for reading.
    write    = 1 << 1, ///< Open for writing.
    create   = 1 << 2, ///< Create the file if it doesn't exist.
    truncate = 1 << 3, ///< Truncate the file to zero length if it exists.
    append   = 1 << 4, ///< Open for appending (writes always at the end).
};

static constexpr open_mode operator|(open_mode a, open_mode b)
{
    return static_cast<open_mode>(static_cast<int>(a) | static_cast<int>(b));
}

static constexpr open_mode operator&(open_mode a, open_mode b)
{
    return static_cast<open_mode>(static_cast<int>(a) & static_cast<int>(b));
}

static constexpr open_mode operator~(open_mode a)
{
    return static_cast<open_mode>(~static_cast<int>(a));
}

static constexpr open_mode operator^(open_mode a, open_mode b)
{
    return static_cast<open_mode>(static_cast<int>(a) ^ static_cast<int>(b));
}

static constexpr bool operator==(open_mode a, open_mode b)
{
    return static_cast<int>(a) == static_cast<int>(b);
}

static constexpr bool operator!=(open_mode a, open_mode b)
{
    return static_cast<int>(a) != static_cast<int>(b);
}

/**
 * @brief Abstract interface for a file system.
 *
 * Implements operations for file management and access.
 * To create a custom file system (e.g., NetworkFS, PackFS), derive from this
 * class and implement the pure virtual methods.
 */
class file_system
{
public:
    virtual ~file_system() = default;

    /**
     * @brief Opens a file at the given path.
     * @param path The path to the file.
     * @param mode The mode(s) to open the file with.
     * @return A pointer to the open file object, or nullptr if failed.
     */
    virtual file *open(string_view path, open_mode mode) = 0;

    /**
     * @brief Checks if a path exists.
     * @param path The path to check.
     * @return True if the path exists, false otherwise.
     */
    virtual bool exists(string_view path) = 0;

    /**
     * @brief Lists contents of a directory.
     * @param path The path to the directory.
     * @return A vector of file_info structs for the directory contents.
     */
    virtual vector<file_info> ls(string_view path) = 0;

    /**
     * @brief Gets information about a specific file or directory.
     * @param path The path to the item.
     * @return A file_info struct with details, or empty/zeroed struct if not
     * found.
     */
    virtual file_info get_info(string_view path) = 0;

    /** @brief Checks if the path points to a directory. */
    virtual bool is_dir(string_view path) = 0;
    /** @brief Checks if the path points to a file. */
    virtual bool is_file(string_view path) = 0;
    /** @brief Checks if the path is read-only. */
    virtual bool is_read_only(string_view path) = 0;

    /**
     * @brief Removes a file or directory.
     * @param path The path to remove.
     * @return True if successful, false otherwise.
     */
    virtual bool remove(string_view path) = 0;

    /**
     * @brief Creates a new directory.
     * @param path The path of the directory to create.
     * @return True if successful, false otherwise.
     */
    virtual bool mkdir(string_view path) = 0;

    /**
     * @brief Renames a file or directory.
     * @param old_path The current path.
     * @param new_path The new path.
     * @return True if successful, false otherwise.
     */
    virtual bool rename(string_view old_path, string_view new_path) = 0;

    /**
     * @brief Gets the native OS path for a given virtual path.
     * @param path The virtual path.
     * @return The native path if supported, or an error.
     */
    virtual result<string> get_native_path(string_view path)
    {
        return report_error(error_code::not_supported);
    }

    /**
     * @brief Gets the virtual path for a given native path.
     * @param native_path The native path.
     * @return The virtual path if supported, or an error.
     */
    virtual result<string> get_virtual_path(string_view native_path)
    {
        return report_error(error_code::not_supported);
    }

    string read_all_text(string_view path)
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

    bool write_all_text(string_view path, string_view text)
    {
        auto file = open(path, open_mode::write | open_mode::truncate);
        if (!file)
            return false;
        size_t writted =
            file->write({(const uint8_t *)text.data(), text.size()});
        file->close();
        delete file;
        return writted == text.size();
    }

    vector<uint8_t> read_all_bytes(string_view path)
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

    bool write_all_bytes(string_view path, span<const uint8_t> bytes)
    {
        auto file = open(path, open_mode::write | open_mode::truncate);
        if (!file)
            return false;
        size_t writted = file->write(bytes);
        file->close();
        delete file;
        return writted == bytes.size();
    }
};

/**
 * @brief Virtual File System (VFS)
 *
 * Manages multiple "mounted" file systems under a single unified directory
 * structure. It dispatches calls to the appropriate underlying file system
 * based on the path.
 *
 * Usage:
 * @code
 *   virtual_fs vfs;
 *   vfs.mount("/data", &disk_fs);
 *   vfs.mount("/net", &network_fs);
 *   auto f = vfs.open("/data/config.txt", open_mode::read);
 * @endcode
 */
class virtual_fs : public file_system
{
public:
    /**
     * @brief Mounts a filesystem at a specific path.
     * The path is normalized before mounting.
     *
     * @param mount_point The virtual path to mount at (e.g., "/mnt/disk").
     * @param fs Pointer to the file system instance to mount.
     */
    void mount(const string &mount_point, file_system *fs)
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

    /**
     * @brief Unmounts the filesystem at the specified mount point.
     * @param mount_point The virtual path to unmount.
     */
    void unmount(const string &mount_point)
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

    /** @copydoc file_system::open */
    file *open(string_view path, open_mode mode) override
    {
        auto [fs, mount_point, relative_path] = resolve(path);
        if (!fs)
            return nullptr;
        return fs->open(relative_path, mode);
    }

    /** @copydoc file_system::exists */
    bool exists(string_view path) override
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

    /** @copydoc file_system::ls */
    vector<file_info> ls(string_view path) override
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
                info.name         = name_sv;
                info.size         = 0;
                info.is_dir       = true;
                info.is_read_only = true;
                files.push_back(info);
            }
        }

        return files;
    }

    /** @copydoc file_system::get_info */
    file_info get_info(string_view path) override
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

            fi.is_dir       = true;
            fi.size         = 0;
            fi.is_read_only = true;
            return fi;
        }

        return {};
    }

    /** @copydoc file_system::is_dir */
    bool is_dir(string_view path) override
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

    /** @copydoc file_system::is_file */
    bool is_file(string_view path) override
    {
        auto [fs, mount_point, relative_path] = resolve(path);
        return fs && fs->is_file(relative_path);
    }

    /** @copydoc file_system::is_read_only */
    bool is_read_only(string_view path) override
    {
        auto [fs, mount_point, relative_path] = resolve(path);
        return fs ? fs->is_read_only(relative_path) : true;
    }

    /** @copydoc file_system::mkdir */
    bool mkdir(string_view path) override
    {
        auto [fs, mount_point, relative_path] = resolve(path);
        return fs && fs->mkdir(relative_path);
    }

    /** @copydoc file_system::remove */
    bool remove(string_view path) override
    {
        auto [fs, mount_point, relative_path] = resolve(path);
        return fs && fs->remove(relative_path);
    }

    /** @copydoc file_system::rename */
    bool rename(string_view old_path, string_view new_path) override
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

    result<string> get_native_path(string_view path) override
    {
        auto [fs, mount_point, relative_path] = resolve(path);
        if (!fs)
            return report_error(error_code::file_not_found);
        return fs->get_native_path(relative_path);
    }

    result<string> get_virtual_path(string_view native_path) override
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

private:
    struct MountPoint
    {
        string path;
        file_system *fs;
    };
    vector<MountPoint> m_mounts;

    /**
     * @brief Resolves a virtual path to a concrete filesystem and a relative
     * path within it. Uses path normalization to handle ".." and "." correctly.
     */
    tuple<file_system *, string, string> resolve(string_view path) const
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
            if (longest_mount->path.length() == 1 &&
                longest_mount->path[0] == '/')
                sub_path = normalized;
            else
                sub_path = string_view(normalized)
                               .substr(longest_mount->path.length());

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
};
} // namespace zabato::fs