#pragma once

#include <zabato/delegate.hpp>
#include <zabato/fs.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/object.hpp>
#include <zabato/resource.hpp>
#include <zabato/string.hpp>
#include <zabato/vector.hpp>

namespace zabato::editor
{
class editor_app;

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
    bundle,
    animation,
    native_controller,
};

string asset_type_to_string(asset_type type);
asset_type string_to_asset_type(const string &t);

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

    void init(resource_manager *rm, editor_app *app);
    void refresh();

    const vector<asset_info> &get_assets(asset_type type) const;
    const vector<asset_info> &get_all_assets() const { return m_all_assets; }

    asset_type get_asset_type(const string &path) const;

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

    pointer<world> get_world() const;

private:
    struct cached_bundle
    {
        uint64_t hash;
        vector<asset_info> sub_assets;
    };

    void load_metadata();
    void save_metadata();
    void scan_directory(const string &path);
    void process_bundle_asset(const string &full_path, const string &filename);
    void process_standard_asset(const string &full_path,
                                const string &filename);
    asset_type determine_type(const string &path) const;

    resource_manager *m_rm = nullptr;
    fs::file_system *m_fs  = nullptr;
    vector<asset_info> m_all_assets;
    vector<vector<asset_info>> m_typed_assets;
    editor_app *m_app = nullptr;

    hash_map<string, cached_bundle> m_bundle_cache;
    bool m_metadata_dirty = false;
};

bool draw_asset_selector(
    const char *label,
    string &current_path,
    asset_type type,
    const asset_database *db,
    const delegate<void(const string &)> &on_locate = nullptr);

bool draw_object_selector(const char *label,
                          pointer<object> &current,
                          const asset_database *db,
                          const delegate<bool(object *)> &filter);
} // namespace zabato::editor
