#include <zabato/ice_fs.hpp>
#include <zabato/ice_packer.hpp>
#include <zabato/span.hpp>
#include <zabato/string.hpp>
#include <zabato/vector.hpp>

#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace zabato::fs
{

namespace
{

struct Entry
{
    string full_path; // Source file path on disk
    string ice_path;  // Relative path in archive (e.g. "subdir/file.txt")
    bool is_dir;
    uint64_t size;
    uint64_t data_offset;
};

// Helper for sorting by ice_path
int compare_entries(const void *a, const void *b)
{
    const Entry *ea = (const Entry *)a;
    const Entry *eb = (const Entry *)b;
    return strcmp(ea->ice_path.c_str(), eb->ice_path.c_str());
}

} // namespace

result<void> ice_packer::pack(const string &source_path)
{
    vector<Entry> entries;

#pragma region Scan entries
    std::filesystem::path base(source_path.c_str());
    if (!std::filesystem::exists(base) || !std::filesystem::is_directory(base))
        return error_code::path_not_found;

    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(base))
    {
        if (entry.is_regular_file())
        {
            // Get relative path
            const std::filesystem::path rel =
                std::filesystem::relative(entry.path(), base);
            const std::string rel_str  = rel.generic_string();
            const std::string full_str = entry.path().generic_string();
            const uint64_t file_size   = entry.file_size();

            entries.push_back({
                .full_path   = string(full_str.c_str()),
                .ice_path    = string(rel_str.c_str()),
                .is_dir      = false,
                .size        = file_size,
                .data_offset = 0,
            });
        }
    }

    if (!entries.empty())
        qsort(entries.data(), entries.size(), sizeof(Entry), compare_entries);

#pragma region Index Layout
    // Calculate size of Index Chunk
    // [Count U64] [EntryTable] [StringBlock]

    vector<uint64_t> name_offsets;
    name_offsets.reserve(entries.size());

    uint64_t current_name_offset = 0;
    for (const auto &e : entries)
    {
        name_offsets.push_back(current_name_offset);
        current_name_offset += e.ice_path.length() + 1;
    }

    const uint64_t string_block_size = current_name_offset;

    const uint64_t index_payload_size =
        sizeof(uint64_t) + (entries.size() * sizeof(ICE_INDEX_ENTRY)) +
        string_block_size;

    const uint64_t index_chunk_start = sizeof(chunk_header) + sizeof(ICE_PACK);
    const uint64_t index_chunk_end =
        index_chunk_start + sizeof(chunk_header) + index_payload_size;

#pragma endregion Index Layout

#pragma region Data Layout
    uint64_t current_data_offset = index_chunk_end;
    for (auto &e : entries)
    {
        e.data_offset = current_data_offset + sizeof(chunk_header);
        current_data_offset += sizeof(chunk_header) + e.size;
    }

#pragma endregion Data Layout

#pragma region Write
    m_writer.write_chunk_header(CHUNK_PACK, sizeof(ICE_PACK));
    ICE_PACK pack = {
        .version         = 1,
        .file_count      = (uint32_t)entries.size(),
        .root_dir_offset = index_chunk_start,
    };
    m_writer.write(&pack, sizeof(pack));

    m_writer.write_chunk_header(CHUNK_INDEX, index_payload_size);
    uint64_t count = entries.size();
    m_writer.write(&count, sizeof(count));

    // Write Entry Table
    for (size_t i = 0; i < entries.size(); ++i)
    {
        ICE_INDEX_ENTRY ie = {
            .path_offset = name_offsets[i],
            .data_offset = entries[i].data_offset,
            .size        = entries[i].size,
            .flags       = 0,
        };
        m_writer.write(&ie, sizeof(ie));
    }

    // Write String Block
    for (const auto &e : entries)
        m_writer.write(e.ice_path.c_str(), e.ice_path.length() + 1);

    // Write File Data
    uint8_t copy_buffer[4096];
    for (const auto &e : entries)
    {
        m_writer.write_chunk_header(CHUNK_FILE, e.size);
        if (e.size > 0)
        {
            FILE *f = fopen(e.full_path.c_str(), "rb");
            if (f)
            {
                size_t remaining = e.size;
                while (remaining > 0)
                {
                    size_t to_read = remaining > sizeof(copy_buffer)
                                         ? sizeof(copy_buffer)
                                         : remaining;
                    if (fread(copy_buffer, 1, to_read, f) != to_read)
                        break;
                    m_writer.write(copy_buffer, to_read);
                    remaining -= to_read;
                }
                fclose(f);
            }
            else
            {
                return report_error(error_code::unable_to_read);
            }
        }
    }

    return {};
}

} // namespace zabato::fs
