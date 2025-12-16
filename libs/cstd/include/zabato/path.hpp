#pragma once

#include <zabato/string.hpp>

namespace zabato::fs
{

constexpr bool is_separator(char c) { return c == '/'; }

constexpr bool is_absolute(string_view path)
{
    return !path.empty() && is_separator(path[0]);
}

constexpr bool is_relative(string_view path) { return !is_absolute(path); }

/**
 * @brief extracts the filename component from the path.
 * @param path the path to extract from.
 * @return the filename component.
 */
constexpr string_view filename(string_view path)
{
    if (path.empty() || is_separator(path.back()))
        return {};

    size_t pos = path.rfind('/');
    return (pos == string_view::npos) ? path : path.substr(pos + 1);
}

/**
 * @brief extracts the parent path.
 * @param path the path to extract from.
 * @return the parent path.
 */
constexpr string_view parent_path(string_view path)
{
    if (path.empty())
        return {};

    size_t len = path.size();
    while (len > 0 && is_separator(path[len - 1]))
        len--;

    if (len == 0)
        return "/";

    string_view stripped = path.substr(0, len);
    size_t pos           = stripped.rfind('/');

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
constexpr string_view extension(string_view path)
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
constexpr string_view stem(string_view path)
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
inline string join(string_view a, string_view b)
{
    if (a.empty())
        return string(b);
    if (b.empty())
        return string(a);

    if (b.front() == '/')
        b = b.substr(1);
    if (a.back() == '/')
        a = a.substr(0, a.size() - 1);

    string result;
    result.reserve(a.size() + b.size() + 1);
    result += a;
    result += '/';
    result += b;
    return result;
}

/**
 * @brief normalizes the path (resolves "." and "..").
 * @param path the path to normalize.
 * @return the normalized path string.
 */
inline string normalize(string_view path)
{
    if (path.empty())
        return "";

    string res;
    res.reserve(path.size());

    bool absolute = is_absolute(path);
    if (absolute)
        res += '/';

    size_t start = 0;
    while (start < path.size())
    {
        while (start < path.size() && is_separator(path[start]))
            start++;
        if (start >= path.size())
            break;

        size_t end = path.find('/', start);
        if (end == string_view::npos)
            end = path.size();

        string_view token = path.substr(start, end - start);
        start             = end;

        if (token == ".")
            continue;

        if (token == "..")
        {
            if (absolute)
            {
                if (res.size() > 1)
                {
                    size_t last_sep = res.rfind('/');
                    res.resize((last_sep == 0) ? 1 : last_sep);
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
                        res += '/';
                    res += "..";
                }
                else
                {
                    size_t last_sep = res.rfind('/');
                    if (last_sep == string::npos)
                        res.clear();
                    else
                        res.resize(last_sep);
                }
            }
        }
        else
        {
            if ((absolute && res.size() > 1) || (!absolute && !res.empty()))
                res += '/';
            res += token;
        }
    }

    return (res.empty() && !absolute) ? "." : res;
}

} // namespace zabato::fs
