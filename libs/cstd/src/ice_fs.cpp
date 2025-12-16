#include <zabato/error.hpp>
#include <zabato/fs.hpp>
#include <zabato/ice.hpp>
#include <zabato/ice_fs.hpp>
#include <zabato/span.hpp>
#include <zabato/stream.hpp>

#include <stdio.h>
#include <string.h>

namespace zabato::fs
{

class ice_fs_file : public file
{
public:
    ice_fs_file(file_stream &stream, uint64_t offset, uint64_t size)
        : m_stream(stream), m_start(offset), m_size(size), m_pos(0)
    {
    }

    size_t read(buffer buffer) override
    {
        if (m_pos >= m_size)
            return 0;

        size_t to_read = min(buffer.size(), m_size - m_pos);
        m_stream.pos(m_start + m_pos);

        auto sub      = buffer.subspan(0, to_read);
        size_t readed = m_stream.read(sub);

        m_pos += readed;
        return readed;
    }

    size_t write(const_buffer buffer) override { return 0; }

    void close() override {}

    bool seek(int64_t offset, origin origin) override
    {
        int64_t target_pos = 0;
        switch (origin)
        {
        case origin::begin:
            target_pos = offset;
            break;
        case origin::current:
            target_pos = m_pos + offset;
            break;
        case origin::end:
            target_pos = m_size + offset;
            break;
        }

        if (target_pos < 0 || target_pos > (int64_t)m_size)
            return false;

        m_pos = target_pos;
        return true;
    }

    bool eof() const override { return m_pos >= m_size; }

    uint64_t tell() const override { return m_pos; }

private:
    file_stream m_stream;
    uint64_t m_start;
    uint64_t m_size;
    uint64_t m_pos;
};

ice_fs::ice_fs(const char *ice)
    : m_stream(nullptr), m_writer(m_stream), m_reader(m_stream)
{
    mount(ice);
}

ice_fs::~ice_fs() { unmount(); }

bool ice_fs::mount(const char *ice)
{
    FILE *file = fopen(ice, "rb");
    if (!file)
        return false;

    m_stream = file_stream(file);
    m_writer = ice_writer(m_stream); // Needed? Only if writing supported later
    m_reader = ice_reader(m_stream);

    // Read Pack Header
    auto result = m_reader.find_chunk(CHUNK_PACK);
    if (result.has_error())
        return false;

    ICE_PACK pack;
    if (m_reader.read(pack) != sizeof(pack))
        return false;

    // Read Index Chunk
    // Pack header now points to start of Index Chunk HEADER
    m_stream.pos(pack.root_dir_offset);

    chunk_header chunk_h;
    if (m_reader.read(chunk_h) != sizeof(chunk_h))
        return false;

    if (chunk_h.id != CHUNK_INDEX)
        return false;

    // Read Index Payload
    // Count
    uint64_t count = 0;
    if (m_reader.read(count) != sizeof(count))
        return false;

    m_entries.resize(count);
    size_t entries_size = count * sizeof(ICE_INDEX_ENTRY);
    if (m_reader.read(m_entries.data(), entries_size) != entries_size)
        return false;

    // Remaining is string block
    // Payload size - uint64 count - entries
    if (chunk_h.size < sizeof(uint64_t) + entries_size)
        return false;
    size_t string_block_size = chunk_h.size - sizeof(uint64_t) - entries_size;

    m_strings.resize(string_block_size);
    if (m_reader.read(m_strings.data(), string_block_size) != string_block_size)
        return false;

    return true;
}

bool ice_fs::unmount()
{
    FILE *file = m_stream.get_file();
    if (!file)
        return false;

    fclose(file);
    m_stream = file_stream(nullptr);
    m_entries.clear();
    m_strings.clear();
    return true;
}

const char *ice_fs::get_path(const ICE_INDEX_ENTRY &entry)
{
    if (entry.path_offset >= m_strings.size())
        return "";
    return &m_strings[entry.path_offset];
}

// Binary search for entry
// Returns index if found, or -1.
// Since we might want lower_bound for prefix search (ls), let's implement that
// logic within specific methods or here. Actually lower_bound returns first
// element >= val.
int64_t ice_fs::find_entry_index(string_view path)
{
    // Strip leading slash if present
    string_view p = path;
    if (p.starts_with('/'))
        p.remove_prefix(1);
    if (p.ends_with('/'))
        p.remove_suffix(1); // Standardize: no trailing slash for index lookups

    int64_t left  = 0;
    int64_t right = m_entries.size() - 1;

    while (left <= right)
    {
        int64_t mid          = left + (right - left) / 2;
        const char *mid_path = get_path(m_entries[mid]);

        // Compare
        int cmp        = 0;
        size_t mp_len  = strlen(mid_path);
        size_t p_len   = p.length();
        size_t min_len = min(mp_len, p_len);

        cmp = memcmp(mid_path, p.data(), min_len);
        if (cmp == 0)
        {
            if (mp_len < p_len)
                cmp = -1;
            else if (mp_len > p_len)
                cmp = 1;
        }

        if (cmp == 0)
            return mid;
        if (cmp < 0)
            left = mid + 1;
        else
            right = mid - 1;
    }

    return -1;
}

file *ice_fs::open(string_view path, open_mode mode)
{
    if ((mode & open_mode::write) == open_mode::write ||
        (mode & open_mode::create) == open_mode::create ||
        (mode & open_mode::truncate) == open_mode::truncate ||
        (mode & open_mode::append) == open_mode::append)
        return nullptr;

    int64_t idx = find_entry_index(path);
    if (idx == -1)
        return nullptr;

    const auto &entry = m_entries[idx];
    return new ice_fs_file(m_stream, entry.data_offset, entry.size);
}

bool ice_fs::exists(string_view path)
{
    if (path == "/" || path.empty())
        return true;
    if (find_entry_index(path) != -1)
        return true;

    return is_dir(path);
}

bool ice_fs::is_dir(string_view path)
{
    if (path == "/" || path.empty())
        return true;

    string p_str(path);
    if (p_str.starts_with('/'))
        p_str = p_str.substr(1);
    if (!p_str.ends_with('/'))
        p_str += '/';

    // Binary search for first entry >= "path/"
    // If that entry starts with "path/", then implicit directory exists.
    int64_t l = 0;
    int64_t r = m_entries.size();
    while (l < r)
    {
        int64_t mid          = l + (r - l) / 2;
        const char *mid_path = get_path(m_entries[mid]);
        if (strcmp(mid_path, p_str.c_str()) < 0)
            l = mid + 1;
        else
            r = mid;
    }

    if (l < (int64_t)m_entries.size())
    {
        const char *found_path = get_path(m_entries[l]);
        // Check if starts with p_str
        if (strncmp(found_path, p_str.c_str(), p_str.length()) == 0)
            return true;
    }

    return false;
}

bool ice_fs::is_file(string_view path)
{
    int64_t idx = find_entry_index(path);
    return idx != -1;
}

bool ice_fs::is_read_only(string_view path) { return true; }

bool ice_fs::remove(string_view path) { return false; }

bool ice_fs::mkdir(string_view path) { return false; }

vector<file_info> ice_fs::ls(string_view path)
{
    // Binary search for first entry starting with "path/"
    string p_str(path);
    if (p_str.starts_with('/'))
        p_str = p_str.substr(1);
    if (!p_str.empty() && !p_str.ends_with('/'))
        p_str += '/';

    // We need custom lower_bound to find first entry >= p_str
    int64_t left      = 0;
    int64_t right     = m_entries.size(); // exclusive
    int64_t start_idx = -1;

    // Standard lower_bound implementation
    while (left < right)
    {
        int64_t mid          = left + (right - left) / 2;
        const char *mid_path = get_path(m_entries[mid]);

        if (strcmp(mid_path, p_str.c_str()) < 0) // mid < value
            left = mid + 1;
        else
            right = mid;
    }
    start_idx = left;

    vector<file_info> results;
    if (start_idx >= (int64_t)m_entries.size())
        return results;

    // Iterate from start_idx
    for (size_t i = start_idx; i < m_entries.size(); ++i)
    {
        const char *entry_path = get_path(m_entries[i]);
        string_view sv(entry_path);

        // Check if still matches prefix
        if (!p_str.empty() && !sv.starts_with(p_str))
            break;

        // Extract immediate child name
        string_view relative = sv;
        if (!p_str.empty())
            relative.remove_prefix(p_str.length());

        // Skip hidden/empty (shouldn't happen if packer clean)
        if (relative.empty())
            continue;

        size_t slash = relative.find('/');
        if (slash != string_view::npos)
        {
            // This is a subdirectory or file in subdirectory
            // But we only want immediate children.
            // If we found "subdir/deep/file", and input is "", we want
            // "subdir". Since it's sorted, multiple "subdir/..." come together.
            // We can deduplicate.
            string subdir_name(relative.substr(0, slash));

            if (results.empty() || results.back().name != subdir_name)
            {
                results.push_back({subdir_name, 0, true, true});
            }
        }
        else
        {
            // Immediate file
            results.push_back({string(relative),
                               m_entries[i].size,
                               (bool)(m_entries[i].flags & 1),
                               true});
        }
    }

    return results;
}

} // namespace zabato::fs