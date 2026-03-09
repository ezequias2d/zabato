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
    bool read_all(vector<uint8_t> &out);
};

/**
 * @struct file_info
 * @brief Contains metadata about a file or directory.
 */
struct file_info
{
    string name;                 ///< The name of the file/directory.
    uint64_t size;               ///< The size of the file in bytes.
    uint64_t last_modified_time; ///< The last write/modification time.
    bool is_dir;                 ///< True if this is a directory.
    bool is_read_only;           ///< True if the item is read-only.
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
    virtual result<string> get_native_path(string_view path);

    /**
     * @brief Gets the virtual path for a given native path.
     * @param native_path The native path.
     * @return The virtual path if supported, or an error.
     */
    virtual result<string> get_virtual_path(string_view native_path);

    string read_all_text(string_view path);

    bool write_all_text(string_view path, string_view text);

    vector<uint8_t> read_all_bytes(string_view path);

    bool write_all_bytes(string_view path, span<const uint8_t> bytes);
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
    void mount(const string &mount_point, file_system *fs);

    /**
     * @brief Unmounts the filesystem at the specified mount point.
     * @param mount_point The virtual path to unmount.
     */
    void unmount(const string &mount_point);

    /** @copydoc file_system::open */
    file *open(string_view path, open_mode mode) override;

    /** @copydoc file_system::exists */
    bool exists(string_view path) override;

    /** @copydoc file_system::ls */
    vector<file_info> ls(string_view path) override;

    /** @copydoc file_system::get_info */
    file_info get_info(string_view path) override;

    /** @copydoc file_system::is_dir */
    bool is_dir(string_view path) override;

    /** @copydoc file_system::is_file */
    bool is_file(string_view path) override;

    /** @copydoc file_system::is_read_only */
    bool is_read_only(string_view path) override;

    /** @copydoc file_system::mkdir */
    bool mkdir(string_view path) override;

    /** @copydoc file_system::remove */
    bool remove(string_view path) override;

    /** @copydoc file_system::rename */
    bool rename(string_view old_path, string_view new_path) override;

    result<string> get_native_path(string_view path) override;

    result<string> get_virtual_path(string_view native_path) override;

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
    tuple<file_system *, string, string> resolve(string_view path) const;
};
} // namespace zabato::fs