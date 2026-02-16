#pragma once
#include "error.hpp"
#include "fs.hpp"

namespace zabato::fs
{

class host_fs : public file_system
{
public:
    static result<host_fs *> create(string_view root_path);

    ~host_fs() override;

private:
    host_fs(string_view root_path);

    file *open(string_view path, open_mode mode) override;
    vector<file_info> ls(string_view path) override;
    file_info get_info(string_view path) override;
    bool remove(string_view path) override;
    bool rename(string_view old_path, string_view new_path) override;
    bool mkdir(string_view path) override;
    bool exists(string_view path) override;
    bool is_dir(string_view path) override;
    bool is_file(string_view path) override;
    bool is_read_only(string_view path) override;
    result<string> get_native_path(string_view path) override;
    result<string> get_virtual_path(string_view native_path) override;

private:
    void *m_data;
};

} // namespace zabato::fs