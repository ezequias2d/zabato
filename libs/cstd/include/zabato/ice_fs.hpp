#pragma once
#include "fs.hpp"
#include "ice.hpp"
#include "stream.hpp"
#include "string.hpp"

namespace zabato::fs
{

class ice_fs : public file_system
{
public:
    ice_fs(const char *ice);
    ~ice_fs() override;

    bool mount(const char *ice);
    bool unmount();

    file *open(string_view path, open_mode mode) override;
    bool exists(string_view path) override;
    vector<file_info> ls(string_view path) override;

    bool is_dir(string_view path) override;
    bool is_file(string_view path) override;
    bool is_read_only(string_view path) override;

    bool remove(string_view path) override;
    bool mkdir(string_view path) override;

private:
    file_stream m_stream;
    ice_reader m_reader;
    ice_writer m_writer;

    vector<ICE_INDEX_ENTRY> m_entries;
    vector<char> m_strings;

    // Internal helpers
    int64_t find_entry_index(string_view path);
    const char *get_path(const ICE_INDEX_ENTRY &entry);
};

} // namespace zabato::fs
