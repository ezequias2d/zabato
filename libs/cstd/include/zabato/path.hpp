#pragma once

#include <zabato/string.hpp>

namespace zabato::fs
{

const char PATH_SEP = '/';

constexpr bool is_separator(char c) { return c == '/' || c == '\\'; }

inline bool is_absolute(string_view path)
{
    if (path.empty())
        return false;
    if (is_separator(path[0]))
        return true;
    if (path.size() >= 3 && path[1] == ':' && is_separator(path[2]))
    {
        char c = path[0];
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
    return false;
}

inline static bool is_relative(string_view path) { return !is_absolute(path); }

/**
 * @brief extracts the filename component from the path.
 * @param path the path to extract from.
 * @return the filename component.
 */
inline static string_view filename(string_view path)
{
    if (path.empty() || is_separator(path.back()))
        return {};

    size_t pos = path.rfind(PATH_SEP);
    return (pos == string_view::npos) ? path : path.substr(pos + 1);
}

/**
 * @brief extracts the parent path.
 * @param path the path to extract from.
 * @return the parent path.
 */
inline static string_view parent_path(string_view path)
{
    if (path.empty())
        return {};

    size_t len = path.size();
    while (len > 0 && is_separator(path[len - 1]))
        len--;

    if (len == 0)
        return "/";

    string_view stripped = path.substr(0, len);
    size_t pos           = stripped.rfind(PATH_SEP);

    if (pos == string_view::npos)
        return "";
    if (pos == 0)
        return "/";

    size_t parent_len = pos;
    while (parent_len > 0 && is_separator(stripped[parent_len - 1]))
        parent_len--;

    return (parent_len == 0) ? "/" : stripped.substr(0, parent_len);
}

/**
 * @brief extracts the file extension.
 * @param path the path to extract from.
 * @return the extension starting with dot, or empty if none.
 */
inline static string_view extension(string_view path)
{
    string_view name = filename(path);
    if (name.empty() || name == "." || name == "..")
        return {};

    size_t pos = name.rfind('.');
    return (pos == string_view::npos || pos == 0) ? "" : name.substr(pos);
}

/**
 * @brief extracts the filename without extension.
 * @param path the path to extract from.
 * @return the stem (filename without extension).
 */
inline static string_view stem(string_view path)
{
    string_view name = filename(path);
    if (name.empty() || name == "." || name == "..")
        return name;

    size_t pos = name.rfind('.');
    return (pos == string_view::npos || pos == 0) ? name : name.substr(0, pos);
}

/**
 * @brief joins two paths with a separator.
 * @param a the first path.
 * @param b the second path.
 * @return the joined path string.
 */
inline static string join(string_view a, string_view b)
{
    if (a.empty())
        return string(b);
    if (b.empty())
        return string(a);

    if (b.front() == PATH_SEP)
        b = b.substr(1);
    if (a.back() == PATH_SEP)
        a = a.substr(0, a.size() - 1);

    string result;
    result.reserve(a.size() + b.size() + 1);
    result += a;
    result += PATH_SEP;
    result += b;
    return result;
}

/**
 * @brief normalizes the path (resolves "." and "..").
 * @param path the path to normalize.
 * @return the normalized path string.
 */
inline static string normalize(string_view path)
{
    if (path.empty())
        return "";

    string res;
    res.reserve(path.size());

    bool absolute = is_absolute(path);
    size_t start = 0;

    // Handle Windows drive letter
    if (path.size() >= 2 && path[1] == ':')
    {
        char c = path[0];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        {
            res += c;
            res += ':';
            start = 2;
        }
    }

    if (absolute &&
        (res.empty() || (start < path.size() && is_separator(path[start]))))
    {
        res += PATH_SEP;
    }

    while (start < path.size())
    {
        while (start < path.size() && is_separator(path[start]))
            start++;
        if (start >= path.size())
            break;

        size_t end = start;
        while (end < path.size() && !is_separator(path[end]))
            end++;

        string_view token = path.substr(start, end - start);
        start             = end;

        if (token == ".")
            continue;

        if (token == "..")
        {
            if (absolute)
            {
                // Don't pop past root or drive letter
                size_t root_limit = (res.size() >= 2 && res[1] == ':') ? 2 : 0;
                if (res.size() > root_limit + 1)
                {
                    size_t last_sep = res.rfind(PATH_SEP);
                    if (last_sep != string::npos && last_sep >= root_limit)
                    {
                        res.resize((last_sep == root_limit && root_limit == 0)
                                       ? 1
                                       : last_sep);
                    }
                }
                else if (res.size() == root_limit + 1 &&
                         is_separator(res.back()))
                {
                    // Already at root, do nothing
                }
                else if (res.size() == root_limit)
                {
                    // Potential issue, but shouldn't happen with is_absolute
                }
            }
            else
            {
                bool top_is_parent =
                    (res == ".." ||
                     (res.size() >= 3 &&
                      string_view(res).substr(res.size() - 3) == "/.."));

                if (res.empty() || top_is_parent)
                {
                    if (!res.empty())
                        res += PATH_SEP;
                    res += "..";
                }
                else
                {
                    size_t last_sep = res.rfind(PATH_SEP);
                    if (last_sep == string::npos)
                        res.clear();
                    else
                        res.resize(last_sep);
                }
            }
        }
        else
        {
            if (!res.empty() && !is_separator(res.back()))
                res += PATH_SEP;
            res += token;
        }
    }

    if (res.empty())
    {
        return absolute ? "/" : ".";
    }

    return res;
}

} // namespace zabato::fs
