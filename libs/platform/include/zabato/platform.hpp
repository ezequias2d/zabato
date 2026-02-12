#pragma once

#include "zabato/delegate.hpp"
#include "zabato/vector.hpp"
#include <zabato/string.hpp>

namespace zabato::platform
{
struct file_filter
{
    string name; /**< e.g. "Script Files" */
    string spec; /** e.g. "*.lua;*.py" */
};

void reveal_file(const string &path);
void open_folder(const string &path);
void open_file_default(const string &file_path);
void open_file_with_app(const string &file_path, const string &app_path);

string open_file_dialog(const string &default_path         = "",
                        const vector<file_filter> &filters = {});

vector<string> open_files_dialog(const string &default_path         = "",
                                 const vector<file_filter> &filters = {});

string open_folder_dialog(const string &default_path = "");

string save_file_dialog(const string &default_path         = "",
                        const string &default_name         = "",
                        const vector<file_filter> &filters = {});

void initialize();
void shutdown();

} // namespace zabato::platform