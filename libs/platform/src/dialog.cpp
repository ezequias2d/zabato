#include <filesystem>
#include <nfd.h>
#include <zabato/platform.hpp>
#include <zabato/string.hpp>

#if defined(_WIN32)
#include <windows.h>

#include <shellapi.h>
#endif

namespace zabato::platform
{

void initialize() { NFD_Init(); }

void shutdown() { NFD_Quit(); }

static vector<nfdu8filteritem_t>
convert_filters(const vector<file_filter> &filters)
{
    vector<nfdu8filteritem_t> nfd_filters;
    for (const auto &filter : filters)
    {
        nfd_filters.push_back({filter.name.c_str(), filter.spec.c_str()});
    }
    return nfd_filters;
}

string open_file_dialog(const string &default_path,
                        const vector<file_filter> &filters)
{
    auto nfd_filters      = convert_filters(filters);
    nfdu8char_t *out_path = nullptr;
    nfdresult_t result    = NFD_OpenDialogU8(&out_path,
                                          nfd_filters.data(),
                                          nfd_filters.size(),
                                          default_path.c_str());
    if (result == NFD_OKAY)
        return string(out_path);
    return "";
}

vector<string> open_files_dialog(const string &default_path,
                                 const vector<file_filter> &filters)
{
    const nfdpathset_t *outPathsSet      = nullptr;
    vector<nfdu8filteritem_t> nfdFilters = convert_filters(filters);
    const nfdu8char_t *defPath =
        default_path.empty() ? nullptr : default_path.c_str();

    nfdresult_t result =
        NFD_OpenDialogMultipleU8(&outPathsSet,
                                 nfdFilters.data(),
                                 (unsigned int)nfdFilters.size(),
                                 defPath);

    vector<string> paths;
    if (result == NFD_OKAY)
    {
        nfdpathsetsize_t count;
        NFD_PathSet_GetCount(outPathsSet, &count);
        for (nfdpathsetsize_t i = 0; i < count; ++i)
        {
            nfdu8char_t *outPath;
            NFD_PathSet_GetPathU8(outPathsSet, i, &outPath);
            paths.push_back(outPath);
            NFD_FreePathU8(outPath);
        }
        NFD_PathSet_Free(outPathsSet);
    }
    return paths;
}

string open_folder_dialog(const string &default_path)
{
    nfdu8char_t *outPath = nullptr;
    const nfdu8char_t *defPath =
        default_path.empty() ? nullptr : default_path.c_str();

    nfdresult_t result = NFD_PickFolderU8(&outPath, defPath);

    string path = "";
    if (result == NFD_OKAY)
    {
        path = outPath;
        NFD_FreePathU8(outPath);
    }
    return path;
}

string save_file_dialog(const string &default_path,
                        const string &default_name,
                        const vector<file_filter> &filters)
{
    nfdu8char_t *outPath = nullptr;
    auto nfd_filters     = convert_filters(filters);
    const nfdu8char_t *defPath =
        default_path.empty() ? nullptr : default_path.c_str();

    nfdresult_t result = NFD_SaveDialogU8(&outPath,
                                          nfd_filters.data(),
                                          (unsigned int)nfd_filters.size(),
                                          defPath,
                                          nullptr);

    string path = "";
    if (result == NFD_OKAY)
    {
        path = outPath;
        NFD_FreePathU8(outPath);
    }
    return path;
}

static string make_absolute(const string &path)
{
    try
    {
        auto s = std::filesystem::absolute(path.c_str()).string();
        return s.c_str();
    }
    catch (...)
    {
        return path;
    }
}

void reveal_file(const string &path)
{
    string full_path = make_absolute(path);

#if defined(_WIN32)
    ShellExecuteA(NULL,
                  NULL,
                  "explorer",
                  ("/select," + full_path).c_str(),
                  NULL,
                  SW_SHOWNORMAL);
#elif defined(__APPLE__)
    string cmd = "open -R \"" + full_path + "\"";
    std::system(cmd.c_str());
#else
    string cmd =
        "dbus-send --session --print-reply --dest=org.freedesktop.FileManager1 "
        "/org/freedesktop/FileManager1 org.freedesktop.FileManager1.ShowItems "
        "array:string:\"file://" +
        full_path + "\" string:\"\" > /dev/null 2>&1";

    // If DBus fails, fallback to opening the parent folder
    if (std::system(cmd.c_str()) != 0)
    {
        auto parent =
            std::filesystem::path(full_path.c_str()).parent_path().string();
        open_folder(string{parent.c_str()});
    }
#endif
}

void open_folder(const string &path)
{
    string full_path = make_absolute(path);

#if defined(_WIN32)
    ShellExecuteA(
        NULL, "explore", full_path.c_str(), NULL, NULL, SW_SHOWNORMAL);

#elif defined(__APPLE__)
    // macOS: open <path>
    auto cmd = "open \"" + full_path + "\"";
    std::system(cmd.c_str());

#else
    // Linux: xdg-open handles folders
    auto cmd = "xdg-open \"" + full_path + "\"";
    std::system(cmd.c_str());
#endif
}

void open_file_default(const string &file_path)
{
    string full_path = make_absolute(file_path);

#if defined(_WIN32)
    ShellExecuteA(NULL, "open", full_path.c_str(), NULL, NULL, SW_SHOWNORMAL);

#elif defined(__APPLE__)
    string cmd = "open \"" + full_path + "\"";
    std::system(cmd.c_str());

#else
    string cmd = "xdg-open \"" + full_path + "\"";
    std::system(cmd.c_str());
#endif
}

void open_file_with_app(const string &file_path, const string &app_path)
{
    string full_path = make_absolute(file_path);

#if defined(_WIN32)
    ShellExecuteA(
        NULL, "open", app_path.c_str(), full_path.c_str(), NULL, SW_SHOWNORMAL);

#elif defined(__APPLE__)
    string cmd = "open -a \"";
    cmd += app_path;
    cmd += "\" \"";
    cmd += full_path;
    cmd += "\"";
    std::system(cmd.c_str());
#else
    string cmd = "\"";
    cmd += app_path;
    cmd += "\" \"";
    cmd += full_path;
    cmd += "\" &";
    std::system(cmd.c_str());
#endif
}

} // namespace zabato::platform