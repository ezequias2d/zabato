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

    size_t read(buffer buffer) override
    {
        if (!m_file)
            return 0;
        return fread(buffer.data(), 1, buffer.size(), m_file);
    }

    size_t write(const_buffer buffer) override
    {
        if (!m_file)
            return 0;
        return fwrite(buffer.data(), 1, buffer.size(), m_file);
    }

    void close() override
    {
        if (m_file)
        {
            fclose(m_file);
            m_file = nullptr;
        }
    }

    bool seek(int64_t offset, origin origin) override
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

    bool eof() const override
    {
        if (!m_file)
            return true;
        return feof(m_file) != 0;
    }

    uint64_t tell() const override
    {
        if (!m_file)
            return 0;
        return ftell(m_file);
    }

private:
    FILE *m_file;
};

struct host_fs_internal
{
    std_fs::path root;
};

static std_fs::path to_path(const string_view &sv)
{
    return std_fs::path(std::string(sv.data(), sv.length()));
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

bool host_fs::remove(string_view path)
{
    auto impl           = static_cast<host_fs_internal *>(m_data);
    auto [safe, target] = resolve_safe(impl->root, path);
    if (!safe)
        return false;

    std::error_code ec;
    return std_fs::remove_all(target, ec) > 0;
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
    if ((mode & open_mode::write) == open_mode::write)
    {
        if ((mode & open_mode::append) == open_mode::append)
            mode_str = "ab";
        else if ((mode & open_mode::truncate) == open_mode::truncate)
            mode_str = "wb";
        else
            mode_str = "rb+";

        bool create   = (mode & open_mode::create) == open_mode::create;
        bool truncate = (mode & open_mode::truncate) == open_mode::truncate;
        bool append   = (mode & open_mode::append) == open_mode::append;

        if (create && truncate)
            mode_str = "wb";
        else if (create && append)
            mode_str = "ab";
        else if (append)
            mode_str = "ab";
        else if (create)
            mode_str = "w+b";
        else
            mode_str = "r+b";
    }

    FILE *f = fopen(target.c_str(), mode_str);
    if (!f)
        return nullptr;

    return new host_file(f);
}

} // namespace zabato::fs
