#include <zabato/host_fs.hpp>

#include <filesystem>
#include <stdio.h>

namespace std_fs = std::filesystem;

namespace zabato::fs
{

class host_file : public file
{
public:
    host_file(FILE *f) : m_file(f) {}
    ~host_file() override { close(); }

    size_t read(buffer buffer) override final
    {
        if (!m_file)
            return 0;
        return fread(buffer.data(), 1, buffer.size(), m_file);
    }

    size_t write(const_buffer buffer) override final
    {
        if (!m_file)
            return 0;
        return fwrite(buffer.data(), 1, buffer.size(), m_file);
    }

    void close() override final
    {
        if (m_file)
        {
            fclose(m_file);
            m_file = nullptr;
        }
    }

    bool seek(int64_t offset, origin origin) override final
    {
        if (!m_file)
            return false;
        int seek_origin = SEEK_SET;
        switch (origin)
        {
        case origin::begin:
            seek_origin = SEEK_SET;
            break;
        case origin::current:
            seek_origin = SEEK_CUR;
            break;
        case origin::end:
            seek_origin = SEEK_END;
            break;
        }
        return fseek(m_file, offset, seek_origin) == 0;
    }

    bool eof() const override final
    {
        if (!m_file)
            return true;
        return feof(m_file) != 0;
    }

    size_t tell() const override final
    {
        if (!m_file)
            return 0;
        return ftell(m_file);
    }

    bool is_closed() const override final { return !m_file; }

    void flush() override final { fflush(m_file); }

private:
    FILE *m_file;
};

struct host_fs_internal
{
    std_fs::path root;
};

static std_fs::path to_path(const string_view &sv)
{
    return std_fs::path(std::string(sv.data(), sv.length())).make_preferred();
}

result<host_fs *> host_fs::create(string_view root_path)
{
    std::error_code ec;
    std_fs::path p = std_fs::absolute(to_path(root_path), ec);

    if (ec || !std_fs::exists(p) || !std_fs::is_directory(p))
        return error_code::invalid_path;

    return new host_fs(root_path);
}

host_fs::host_fs(string_view root_path) : m_data(new host_fs_internal())
{
    auto impl = static_cast<host_fs_internal *>(m_data);
    std::error_code ec;
    std_fs::path p = std_fs::absolute(to_path(root_path), ec);
    impl->root     = std_fs::canonical(p, ec);
}

host_fs::~host_fs() { delete static_cast<host_fs_internal *>(m_data); }

static std::pair<bool, std_fs::path> resolve_safe(const std_fs::path &root,
                                                  const string_view &path)
{
    if (root.empty())
        return {false, {}};

    std_fs::path rel = to_path(path);
    if (rel.has_root_path())
        rel = rel.relative_path();

    std_fs::path full = root / rel;

    std::error_code ec;
    std_fs::path canon = std_fs::weakly_canonical(full, ec);
    if (ec)
        return {false, {}};

    auto root_str  = root.string();
    auto canon_str = canon.string();

    if (canon_str.find(root_str) != 0)
        return {false, {}};

    return {true, canon};
}

vector<file_info> host_fs::ls(string_view path)
{
    auto impl = static_cast<host_fs_internal *>(m_data);
    vector<file_info> results;

    auto [safe, target] = resolve_safe(impl->root, path);
    if (!safe || !std_fs::exists(target) || !std_fs::is_directory(target))
        return results;

    for (const auto &entry : std_fs::directory_iterator(target))
    {
        file_info fi;
        fi.name = entry.path().filename().string().c_str();

        std::error_code ec;
        if (entry.is_directory())
        {
            fi.is_dir = true;
            fi.size   = 0;
        }
        else
        {
            fi.is_dir = false;
            fi.size   = entry.file_size(ec);
        }

        auto perms = entry.status().permissions();
        fi.is_read_only =
            (perms & std_fs::perms::owner_write) == std_fs::perms::none;

        results.push_back(fi);
    }
    return results;
}

file_info host_fs::get_info(string_view path)
{
    auto impl = static_cast<host_fs_internal *>(m_data);
    file_info fi;

    auto [safe, target] = resolve_safe(impl->root, path);
    if (!safe || !std_fs::exists(target))
        return fi;

    fi.name = target.filename().string().c_str();

    std::error_code ec;
    if (std_fs::is_directory(target))
    {
        fi.is_dir = true;
        fi.size   = 0;
        try
        {
            for (const auto &entry : std_fs::recursive_directory_iterator(
                     target, std_fs::directory_options::skip_permission_denied))
            {
                if (entry.is_symlink())
                    continue;

                if (entry.is_regular_file())
                {
                    fi.size += entry.file_size(ec);
                    if (ec)
                        ec.clear();
                }
            }
        }
        catch (...)
        {
        }
    }
    else
    {
        fi.is_dir = false;
        fi.size   = std_fs::file_size(target, ec);
    }

    auto perms = std_fs::status(target).permissions();
    fi.is_read_only =
        (perms & std_fs::perms::owner_write) == std_fs::perms::none;

    return fi;
}

bool host_fs::remove(string_view path)
{
    auto impl           = static_cast<host_fs_internal *>(m_data);
    auto [safe, target] = resolve_safe(impl->root, path);
    if (!safe)
        return false;

    std::error_code ec;
    return std_fs::remove_all(target, ec) > 0;
}

bool host_fs::rename(string_view old_path, string_view new_path)
{
    auto impl        = static_cast<host_fs_internal *>(m_data);
    auto [safe, old] = resolve_safe(impl->root, old_path);
    auto [safe2, nw] = resolve_safe(impl->root, new_path);

    if (!safe || !safe2)
        return false;

    std::error_code ec;
    std_fs::rename(old, nw, ec);
    return !ec;
}

bool host_fs::mkdir(string_view path)
{
    auto impl           = static_cast<host_fs_internal *>(m_data);
    auto [safe, target] = resolve_safe(impl->root, path);
    if (!safe)
        return false;

    std::error_code ec;
    return std_fs::create_directories(target, ec);
}

bool host_fs::exists(string_view path)
{
    auto impl           = static_cast<host_fs_internal *>(m_data);
    auto [safe, target] = resolve_safe(impl->root, path);
    if (!safe)
        return false;
    return std_fs::exists(target);
}

bool host_fs::is_dir(string_view path)
{
    auto impl           = static_cast<host_fs_internal *>(m_data);
    auto [safe, target] = resolve_safe(impl->root, path);
    if (!safe)
        return false;
    return std_fs::is_directory(target);
}

bool host_fs::is_file(string_view path)
{
    auto impl           = static_cast<host_fs_internal *>(m_data);
    auto [safe, target] = resolve_safe(impl->root, path);
    if (!safe)
        return false;
    return std_fs::is_regular_file(target);
}

bool host_fs::is_read_only(string_view path)
{
    auto impl           = static_cast<host_fs_internal *>(m_data);
    auto [safe, target] = resolve_safe(impl->root, path);
    if (!safe)
        return true;

    auto status = std_fs::status(target);
    return (status.permissions() & std_fs::perms::owner_write) ==
           std_fs::perms::none;
}

file *host_fs::open(string_view path, open_mode mode)
{
    auto impl           = static_cast<host_fs_internal *>(m_data);
    auto [safe, target] = resolve_safe(impl->root, path);
    if (!safe)
        return nullptr;

    const char *mode_str = "rb";
    bool read            = (mode & open_mode::read) == open_mode::read;
    bool write           = (mode & open_mode::write) == open_mode::write;
    if (write)
    {
        bool create   = (mode & open_mode::create) == open_mode::create;
        bool truncate = (mode & open_mode::truncate) == open_mode::truncate;
        bool append   = (mode & open_mode::append) == open_mode::append;

        if (truncate)
            mode_str = read ? "wb+" : "wb";
        else if (append)
            mode_str = read ? "ab+" : "ab";
        else if (create)
            mode_str = read ? "wb+" : "ab";
        else
            mode_str = "rb+";
    }
    else
        mode_str = "rb";

    FILE *f = fopen(target.c_str(), mode_str);
    if (!f)
        return nullptr;

    return new host_file(f);
}

result<string> host_fs::get_native_path(string_view path)
{
    auto impl           = static_cast<host_fs_internal *>(m_data);
    auto [safe, target] = resolve_safe(impl->root, path);
    if (!safe)
        return report_error(error_code::invalid_path);
    return string{target.string().c_str()};
}

result<string> host_fs::get_virtual_path(string_view native_path)
{
    auto impl = static_cast<host_fs_internal *>(m_data);

    std::error_code ec;
    auto p = std_fs::absolute(to_path(native_path), ec);
    if (ec)
        return report_error(error_code::invalid_path);

    auto rel = std_fs::relative(p, impl->root, ec);

    // If error, or empty, it's not in this FS
    if (ec || rel.empty())
        return report_error(error_code::file_not_found);

    // If relative path starts with "..", it's outside the root
    if (rel.begin() != rel.end() && *rel.begin() == "..")
        return report_error(error_code::file_not_found);

    // If result is ".", it means it IS the root.
    if (rel == ".")
        return string("");

    return string(rel.generic_string().c_str());
}

} // namespace zabato::fs
