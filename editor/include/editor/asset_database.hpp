#pragma once

#include <zabato/delegate.hpp>
#include <zabato/fs.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/resource.hpp>
#include <zabato/string.hpp>
#include <zabato/vector.hpp>

namespace zabato::editor
{

enum class asset_type
{
    unknown,
    script,
    mesh,
    texture,
    audio,
    scene,
    shader,
    material,
    native_controller,
};

struct asset_info
{
    string path;
    string name;
    asset_type type;
};

class asset_database
{
public:
    asset_database()  = default;
    ~asset_database() = default;

    void init(resource_manager *rm);
    void refresh();

    const vector<asset_info> &get_assets(asset_type type) const;
    const vector<asset_info> &get_all_assets() const { return m_all_assets; }

    vector<asset_info> query(const string &term) const
    {
        vector<asset_info> res;
        for (const auto &a : m_all_assets)
        {
            if (a.name.find(term) != string::npos ||
                a.path.find(term) != string::npos)
                res.push_back(a);
        }
        return res;
    }

private:
    void scan_directory(const string &path);
    asset_type determine_type(const string &path);

    resource_manager *m_rm = nullptr;
    fs::file_system *m_fs  = nullptr;
    vector<asset_info> m_all_assets;
    vector<vector<asset_info>> m_typed_assets;
};

bool draw_asset_selector(const char *label,
                         string &current_path,
                         asset_type type,
                         const asset_database *db,
                         delegate<void(const string &)> on_locate = nullptr);
} // namespace zabato::editor
