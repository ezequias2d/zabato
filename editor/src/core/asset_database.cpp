#include <editor/asset_database.hpp>
#include <editor/editor.hpp>

#include <imgui.h>
#include <tinyxml2.h>
#include <zabato/animation.hpp>
#include <zabato/asset_bundle.hpp>
#include <zabato/controller.hpp>
#include <zabato/mesh.hpp>
#include <zabato/node.hpp>
#include <zabato/object.hpp>
#include <zabato/object_resource.hpp>
#include <zabato/resource.hpp>
#include <zabato/world.hpp>

namespace zabato::editor
{

void asset_database::init(resource_manager *rm, editor_app *app)
{
    m_rm  = rm;
    m_fs  = rm ? rm->get_file_system() : nullptr;
    m_app = app;
    if (m_fs)
    {
        load_metadata();
    }
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

    if (m_fs)
    {
        scan_directory(".");
        if (m_metadata_dirty)
            save_metadata();
    }
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
            // Check for bundled resources first
            string ext;
            size_t last_dot = entry.name.rfind('.');
            if (last_dot != string::npos)
            {
                ext = entry.name.substr(last_dot + 1);
                for (char &c : ext)
                    if (c >= 'A' && c <= 'Z')
                        c += ('a' - 'A');
            }

            auto importer = importer_registry::find_importer(string(".") + ext);
            if (importer && importer->is_resource_type(asset_bundle::TYPE))
            {
                process_bundle_asset(full_path, entry.name);
                continue;
            }

            process_standard_asset(full_path, entry.name);
        }
    }
}

static asset_type get_asset_type_from_resource(resource *res)
{
    if (!res)
        return asset_type::unknown;
    if (res->is_derived(mesh::TYPE))
        return asset_type::mesh;
    if (res->is_derived(animation::TYPE))
        return asset_type::animation;
    if (res->is_derived(material::TYPE))
        return asset_type::material;
    if (res->is_derived(texture::TYPE))
        return asset_type::texture;
    if (res->is_derived(world::TYPE))
        return asset_type::scene;
    if (res->is_derived(shader_asset::TYPE))
        return asset_type::shader;
    if (res->is_derived(script::TYPE))
        return asset_type::script;

    if (res->is_derived(object_resource::TYPE))
    {
        auto obj_res = static_cast<object_resource *>(res);
        if (auto obj = obj_res->get_object())
        {
            if (obj->is_derived(world::TYPE))
                return asset_type::scene;
            if (obj->is_derived(material::TYPE))
                return asset_type::material;
        }
    }

    return asset_type::unknown;
}

void asset_database::process_bundle_asset(const string &full_path,
                                          const string &filename)
{
    uint64_t current_hash =
        m_fs ? m_fs->get_info(full_path).last_modified_time : 0;
    bool needs_import = true;

    // Check cache
    cached_bundle cb;
    if (m_bundle_cache.try_get_value(full_path, cb))
    {
        if (cb.hash == current_hash)
        {
            needs_import = false;
            for (const auto &info : cb.sub_assets)
            {
                m_all_assets.push_back(info);
                int idx = (int)info.type;
                if (idx >= (int)m_typed_assets.size())
                    m_typed_assets.resize(idx + 1);
                m_typed_assets[idx].push_back(info);
            }
        }
    }

    if (needs_import && m_rm)
    {
        resource_ref bundle_ref(full_path, m_rm);
        auto bundle = bundle_ref.get<asset_bundle>();
        if (bundle)
        {
            cached_bundle new_cache;
            new_cache.hash = current_hash;

            for (const auto &sub : bundle->get_resources())
            {
                asset_type sub_type =
                    get_asset_type_from_resource(sub.res.get());

                if (sub_type != asset_type::unknown)
                {
                    asset_info info;
                    info.path = full_path + "@" + sub.name;
                    info.name = sub.name;
                    info.type = sub_type;

                    // Register info into standard db
                    m_all_assets.push_back(info);
                    int idx = (int)sub_type;
                    if (idx >= (int)m_typed_assets.size())
                        m_typed_assets.resize(idx + 1);
                    m_typed_assets[idx].push_back(info);

                    // Register into cache array
                    new_cache.sub_assets.push_back(info);
                }
            }

            // Also add bundle file itself as mesh cache
            asset_info bundle_info;
            bundle_info.path = full_path;
            bundle_info.name = filename;
            bundle_info.type = asset_type::bundle;
            new_cache.sub_assets.push_back(bundle_info);

            m_all_assets.push_back(bundle_info);
            int idx = (int)asset_type::bundle;
            if (idx >= (int)m_typed_assets.size())
                m_typed_assets.resize(idx + 1);
            m_typed_assets[idx].push_back(bundle_info);

            m_bundle_cache.add_or_set(full_path, new_cache);
            m_metadata_dirty = true;
        }
    }
}

void asset_database::process_standard_asset(const string &full_path,
                                            const string &filename)
{
    uint64_t current_hash =
        m_fs ? m_fs->get_info(full_path).last_modified_time : 0;
    bool needs_import = true;
    asset_type type   = asset_type::unknown;

    // Check cache
    cached_bundle cb;
    if (m_bundle_cache.try_get_value(full_path, cb))
    {
        if (cb.hash == current_hash && !cb.sub_assets.empty())
        {
            needs_import = false;
            type         = cb.sub_assets[0].type;

            // Re-add to database
            asset_info info = cb.sub_assets[0];
            m_all_assets.push_back(info);
            int idx = (int)info.type;
            if (idx >= (int)m_typed_assets.size())
                m_typed_assets.resize(idx + 1);
            m_typed_assets[idx].push_back(info);
        }
    }

    if (needs_import)
    {
        if (m_rm)
        {
            resource_ref res_ref(full_path, m_rm);
            if (auto res = res_ref.get<resource>())
                type = get_asset_type_from_resource(res.get());
        }
    }

    if (type != asset_type::unknown)
    {
        asset_info info;
        info.path = full_path;
        info.name = filename;
        info.type = type;

        m_all_assets.push_back(info);

        // Add to specific type bucket
        int idx = (int)type;
        if (idx >= (int)m_typed_assets.size())
            m_typed_assets.resize(idx + 1);
        m_typed_assets[idx].push_back(info);

        cached_bundle new_cache;
        new_cache.hash = current_hash;
        new_cache.sub_assets.push_back(info);
        m_bundle_cache.add_or_set(full_path, new_cache);
        m_metadata_dirty = true;
    }
}

asset_type asset_database::get_asset_type(const string &path) const
{
    asset_type best = asset_type::unknown;
    for (const auto &a : m_all_assets)
    {
        if (a.path == path)
            return a.type;

        if (a.path.size() >= path.size() &&
            a.path.substr(a.path.size() - path.size()) == path)
            best = a.type;
    }
    return best;
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

pointer<world> asset_database::get_world() const { return m_app->get_world(); }

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

bool draw_object_selector(const char *label,
                          pointer<object> &current,
                          const asset_database *db,
                          delegate<bool(object *)> filter)
{
    ImGui::PushID(label);

    if (ImGui::Button("Select"))
        ImGui::OpenPopup(label);

    bool changed = false;
    if (ImGui::BeginPopup(label))
    {
        vector<object *> world_objects;
        auto w = db->get_world();
        if (w)
        {
            auto collect = [&](spatial *s, auto &collect_ref) -> void
            {
                if (!s)
                    return;
                world_objects.push_back(s);
                for (auto ctrl : s->get_controllers())
                    if (ctrl)
                        world_objects.push_back(ctrl.get());
                if (s->is_derived(node::TYPE))
                {
                    node *n = static_cast<node *>(s);
                    for (int i = 0; i < n->quantity(); ++i)
                        collect_ref(n->child_at(i).get(), collect_ref);
                }
            };
            world_objects.push_back(w);
            for (auto ctrl : w->get_controllers())
                if (ctrl)
                    world_objects.push_back(ctrl.get());
            collect(w->get_scene_root().get(), collect);
        }

        for (auto obj : world_objects)
        {
            if (filter(obj))
            {
                string display_name = obj->name_view();
                if (display_name.empty())
                    display_name = obj->id().to_string();

                if (ImGui::Selectable(display_name.c_str(),
                                      current.get() == obj))
                {
                    current = obj;
                    changed = true;
                }
            }
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();

    return changed;
}

string asset_type_to_string(asset_type type)
{
    switch (type)
    {
    case asset_type::mesh:
        return "mesh";
    case asset_type::animation:
        return "animation";
    case asset_type::material:
        return "material";
    case asset_type::texture:
        return "texture";
    case asset_type::scene:
        return "scene";
    case asset_type::shader:
        return "shader";
    case asset_type::script:
        return "script";
    case asset_type::bundle:
        return "bundle";
    case asset_type::audio:
        return "audio";
    case asset_type::native_controller:
        return "native_controller";
    default:
        return "unknown";
    }
}

asset_type string_to_asset_type(const string &t)
{
    if (t == "mesh")
        return asset_type::mesh;
    if (t == "animation")
        return asset_type::animation;
    if (t == "material")
        return asset_type::material;
    if (t == "texture")
        return asset_type::texture;
    if (t == "scene")
        return asset_type::scene;
    if (t == "shader")
        return asset_type::shader;
    if (t == "script")
        return asset_type::script;
    if (t == "bundle")
        return asset_type::bundle;
    if (t == "audio")
        return asset_type::audio;
    if (t == "native_controller")
        return asset_type::native_controller;
    return asset_type::unknown;
}

void asset_database::load_metadata()
{
    m_bundle_cache.clear();
    m_metadata_dirty = false;

    if (!m_fs->exists(".metadata.xml"))
        return;

    auto xml_content = m_fs->read_all_text(".metadata.xml");
    if (xml_content.empty())
        return;

    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml_content.c_str()) != tinyxml2::XML_SUCCESS)
        return;

    tinyxml2::XMLElement *root = doc.FirstChildElement("AssetIndex");
    if (!root)
        return;

    for (tinyxml2::XMLElement *bundle_el = root->FirstChildElement("Bundle");
         bundle_el;
         bundle_el = bundle_el->NextSiblingElement("Bundle"))
    {
        const char *bundle_path     = bundle_el->Attribute("path");
        const char *bundle_hash_str = bundle_el->Attribute("hash");

        if (!bundle_path || !bundle_hash_str)
            continue;

        cached_bundle cb;
        cb.hash = std::stoull(bundle_hash_str);

        for (tinyxml2::XMLElement *asset_el =
                 bundle_el->FirstChildElement("Asset");
             asset_el;
             asset_el = asset_el->NextSiblingElement("Asset"))
        {
            const char *a_path = asset_el->Attribute("path");
            const char *a_name = asset_el->Attribute("name");
            const char *a_type = asset_el->Attribute("type");

            if (a_path && a_name && a_type)
            {
                asset_info info;
                info.path = a_path;
                info.name = a_name;

                info.type = string_to_asset_type(a_type);

                cb.sub_assets.push_back(info);
            }
        }

        m_bundle_cache.add_or_set(bundle_path, cb);
    }

    for (tinyxml2::XMLElement *asset_el = root->FirstChildElement("Asset");
         asset_el;
         asset_el = asset_el->NextSiblingElement("Asset"))
    {
        const char *a_path     = asset_el->Attribute("path");
        const char *a_hash_str = asset_el->Attribute("hash");
        const char *a_name     = asset_el->Attribute("name");
        const char *a_type     = asset_el->Attribute("type");

        if (a_path && a_hash_str && a_name && a_type)
        {
            cached_bundle cb;
            cb.hash = std::stoull(a_hash_str);

            asset_info info;
            info.path = a_path;
            info.name = a_name;

            info.type = string_to_asset_type(a_type);

            cb.sub_assets.push_back(info);
            m_bundle_cache.add_or_set(a_path, cb);
        }
    }
}

void asset_database::save_metadata()
{
    if (!m_metadata_dirty || !m_fs)
        return;

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *root = doc.NewElement("AssetIndex");
    doc.InsertFirstChild(root);

    for (const auto &pair : m_bundle_cache)
    {
        bool is_bundle = false;
        for (const auto &sub : pair.value.sub_assets)
        {
            if (sub.type == asset_type::bundle)
                is_bundle = true;
        }

        if (is_bundle)
        {
            tinyxml2::XMLElement *bundle_el = doc.NewElement("Bundle");
            bundle_el->SetAttribute("path", pair.key.c_str());
            bundle_el->SetAttribute("hash",
                                    std::to_string(pair.value.hash).c_str());
            root->InsertEndChild(bundle_el);

            for (const auto &sub : pair.value.sub_assets)
            {
                tinyxml2::XMLElement *asset_el = doc.NewElement("Asset");
                asset_el->SetAttribute("path", sub.path.c_str());
                asset_el->SetAttribute("name", sub.name.c_str());

                asset_el->SetAttribute("type",
                                       asset_type_to_string(sub.type).c_str());
                bundle_el->InsertEndChild(asset_el);
            }
        }
        else if (pair.value.sub_assets.size() == 1)
        {
            const auto &sub                = pair.value.sub_assets[0];
            tinyxml2::XMLElement *asset_el = doc.NewElement("Asset");
            asset_el->SetAttribute("path", sub.path.c_str());
            asset_el->SetAttribute("hash",
                                   std::to_string(pair.value.hash).c_str());
            asset_el->SetAttribute("name", sub.name.c_str());

            asset_el->SetAttribute("type",
                                   asset_type_to_string(sub.type).c_str());
            root->InsertEndChild(asset_el);
        }
    }

    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);

    m_fs->write_all_text(".metadata.xml", printer.CStr());
    m_metadata_dirty = false;
}

} // namespace zabato::editor
