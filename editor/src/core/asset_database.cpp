#include <editor/asset_database.hpp>
#include <imgui.h>
#include <zabato/controller.hpp>
#include <zabato/object.hpp>
#include <zabato/resource.hpp>
#include <zabato/script.hpp>

namespace zabato::editor
{

void asset_database::init(resource_manager *rm)
{
    m_rm = rm;
    m_fs = rm ? rm->get_file_system() : nullptr;
    refresh();
}

void asset_database::refresh()
{
    if (!m_fs)
        return;

    m_all_assets.clear();
    for (auto &vec : m_typed_assets)
        vec.clear();

    // Add Builtin Assets
    auto add_builtin = [&](const char *name, asset_type type)
    {
        asset_info info;
        info.path = name;
        info.name = name;
        info.type = type;
        m_all_assets.push_back(info);
        int idx = (int)type;
        if (idx >= (int)m_typed_assets.size())
            m_typed_assets.resize(idx + 1);
        m_typed_assets[idx].push_back(info);
    };

    add_builtin("builtin:cube", asset_type::mesh);
    add_builtin("builtin:sphere", asset_type::mesh);
    add_builtin("builtin:plane", asset_type::mesh);

    // Add Native Controllers
    for (const auto &f : *object::s_factory)
    {
        if (f.value.type && f.value.type->is_derived(controller::TYPE) &&
            !f.value.type->is_derived(script_instance::TYPE))
        {
            add_builtin(f.key.c_str(), asset_type::native_controller);
        }
    }

    scan_directory(".");
}

void asset_database::scan_directory(const string &path)
{
    if (!m_fs->exists(path))
        return;

    auto entries = m_fs->ls(path);
    for (const auto &entry : entries)
    {
        string full_path = path;
        if (full_path != "." && !full_path.empty() && full_path.back() != '/')
            full_path += "/";

        if (path == ".")
            full_path = "";

        full_path += entry.name;

        if (entry.is_dir)
        {
            if (entry.name != "." && entry.name != "..")
            {
                scan_directory(full_path);
            }
        }
        else
        {
            asset_type type = determine_type(entry.name);
            if (type != asset_type::unknown)
            {
                asset_info info;
                info.path = full_path;
                info.name = entry.name;
                info.type = type;

                m_all_assets.push_back(info);

                // Add to specific type bucket
                int idx = (int)type;
                if (idx >= (int)m_typed_assets.size())
                {
                    m_typed_assets.resize(idx + 1);
                }
                m_typed_assets[idx].push_back(info);
            }
        }
    }
}

asset_type asset_database::determine_type(const string &filename)
{
    string ext;
    size_t last_dot = filename.rfind('.');
    if (last_dot != string::npos)
    {
        ext = filename.substr(last_dot + 1);
        // Simple tolower
        for (char &c : ext)
            if (c >= 'A' && c <= 'Z')
                c += ('a' - 'A');
    }

    if (ext == "lua")
        return asset_type::script;
    if (ext == "obj" || ext == "gltf" || ext == "glb" || ext == "fbx")
        return asset_type::mesh;
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "tga" ||
        ext == "bmp")
        return asset_type::texture;
    if (ext == "wav" || ext == "mp3" || ext == "ogg")
        return asset_type::audio;
    if (ext == "zfile")
        return asset_type::scene;
    if (ext == "zmaterial")
        return asset_type::material;
    if (ext == "zshader")
        return asset_type::shader;

    return asset_type::unknown;
}

const vector<asset_info> &asset_database::get_assets(asset_type type) const
{
    // Ensure index is valid
    int idx = (int)type;
    if (idx >= 0 && idx < (int)m_typed_assets.size())
    {
        return m_typed_assets[idx];
    }
    static vector<asset_info> empty;
    return empty;
}

bool draw_asset_selector(const char *label,
                         string &current_path,
                         asset_type type,
                         const asset_database *db,
                         delegate<void(const string &)> on_locate)
{
    if (!db)
        return false;

    ImGui::PushID(label);

    // Use a button to toggle popup
    if (ImGui::Button("Select"))
    {
        ImGui::OpenPopup(label);
    }

    if (on_locate)
    {
        ImGui::SameLine();
        if (ImGui::Button("Locate"))
        {
            on_locate(current_path);
        }
    }

    bool changed = false;
    if (ImGui::BeginPopup(label))
    {
        // TODO: Search filter could be added here

        const auto &assets = db->get_assets(type);
        for (const auto &asset : assets)
        {
            if (ImGui::Selectable(asset.name.c_str(),
                                  asset.path == current_path))
            {
                current_path = asset.path;
                changed      = true;
            }
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();

    return changed;
}
} // namespace zabato::editor
