#pragma once

#include <zabato/path.hpp>
#include <zabato/span.hpp>
#include <zabato/string.hpp>
#include <zabato/tuple.hpp>
#include <zabato/vector.hpp>

namespace zabato::fs
{

/**
 * @enum origin
 * @brief Specifies the reference point for file seeking.
 */
enum class origin
{
    /** @brief Beginning of the file. */
    begin,
    /** @brief Current position of the file pointer. */
    current,
    /** @brief End of the file. */
    end,
};

/**
 * @class mount
 * @brief Tag class for filesystem mounting operations/types.
 */
class mount
{
};

/**
 * @brief Abstract interface representing an open file.
 *
 * Provides methods for reading, writing, and seeking within a file.
 * Concrete implementations handle the specifics of the storage medium.
 */
class file
{
public:
    virtual ~file() = default;

    /**
     * @brief Reads data from the file into the provided buffer.
     * @param buffer The buffer to store read data.
     * @return The number of bytes actually read.
     */
    virtual size_t read(buffer buffer) = 0;

    /**
     * @brief Writes data from the provided buffer to the file.
     * @param buffer The buffer containing data to write.
     * @return The number of bytes actually written.
     */
    virtual size_t write(const_buffer buffer) = 0;

    /**
     * @brief Closes the file, releasing any resources.
     */
    virtual void close() = 0;

    /**
     * @brief Moves the file pointer to a specific location.
     * @param offset The offset in bytes relative to the origin.
     * @param origin The reference point for the offset.
     * @return True if the seek was successful, false otherwise.
     */
    virtual bool seek(int64_t offset, origin origin) = 0;

    /**
     * @brief Checks if the file pointer is at the end of the file.
     * @return True if at EOF, false otherwise.
     */
    virtual bool eof() const = 0;

    /**
     * @brief Returns the current position of the file pointer.
     * @return The current byte offset from the beginning of the file.
     */
    virtual uint64_t tell() const = 0;
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
        auto [fs, relative_path] = resolve(path);
        if (!fs)
            return nullptr;
        return fs->open(relative_path, mode);
    }

    /** @copydoc file_system::exists */
    bool exists(string_view path) override
    {
        auto [fs, relative_path] = resolve(path);
        return fs && fs->exists(relative_path);
    }

    /** @copydoc file_system::ls */
    vector<file_info> ls(string_view path) override
    {
        auto [fs, relative_path] = resolve(path);
        if (fs)
            return fs->ls(relative_path);
        return {};
    }

    /** @copydoc file_system::is_dir */
    bool is_dir(string_view path) override
    {
        // Root of VFS is always a dir
        if (path == "/" || path.empty())
            return true;

        auto [fs, relative_path] = resolve(path);
        return fs && fs->is_dir(relative_path);
    }

    /** @copydoc file_system::is_file */
    bool is_file(string_view path) override
    {
        auto [fs, relative_path] = resolve(path);
        return fs && fs->is_file(relative_path);
    }

    /** @copydoc file_system::is_read_only */
    bool is_read_only(string_view path) override
    {
        auto [fs, relative_path] = resolve(path);
        return fs ? fs->is_read_only(relative_path) : true;
    }

    /** @copydoc file_system::mkdir */
    bool mkdir(string_view path) override
    {
        auto [fs, relative_path] = resolve(path);
        return fs && fs->mkdir(relative_path);
    }

    /** @copydoc file_system::remove */
    bool remove(string_view path) override
    {
        auto [fs, relative_path] = resolve(path);
        return fs && fs->remove(relative_path);
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
    tuple<file_system *, string> resolve(string_view path) const
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
                return {longest_mount->fs, "/"};

            if (sub_path.front() != '/')
            {
                string res = "/";
                res.reserve(sub_path.length() + 1);
                res += sub_path;
                return {longest_mount->fs, res};
            }

            return {longest_mount->fs, string(sub_path)};
        }

        return {nullptr, ""};
    }
};
} // namespace zabato::fs