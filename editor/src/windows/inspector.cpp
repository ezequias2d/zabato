#include <imgui.h>
#include <tinyxml2.h>
#include <zabato/error.hpp>
#include <zabato/fs.hpp>
#include <zabato/object.hpp>

#include <zabato/asset_bundle.hpp>
#include <zabato/controller.hpp>
#include <zabato/imgui.hpp>
#include <zabato/math.hpp>
#include <zabato/reflection.hpp>
#include <zabato/rtti.hpp>
#include <zabato/script.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/spatial.hpp>
#include <zabato/symbol.hpp>
#include <zabato/transformation.hpp>

#include <editor/asset_database.hpp>
#include <editor/core/editor_registry.hpp>
#include <editor/core/property_grid.hpp>
#include <editor/editor.hpp>
#include <editor/windows/inspector.hpp>

#include <zabato/mesh.hpp>
#include <zabato/node.hpp>
#include <zabato/renderer.hpp>
#include <zabato/stb/image_utils.hpp>
#include <zabato/world.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato::editor
{
void inspector_window::render_object_properties(object *obj, real dtime)
{
    if (!obj)
        return;

    // Check if there is a custom editor for this type
    auto editor_func = editor_registry::find_editor(obj->type());
    if (editor_func)
        editor_func(obj, m_app, dtime);
    else
        property_grid::render_object(obj,
                                     obj->type(),
                                     m_app.get_asset_database(),
                                     [&](const string &path)
                                     {
                                         game_message msg;
                                         msg.msg_id    = "cmd_reveal_asset";
                                         msg.sender_id = uuid();
                                         msg.data      = value(path);
                                         m_app.send_message(msg);
                                     });
}

void inspector_window::object_inspector(object *obj, real dtime)
{

    const char *obj_type_name = obj->type().name();
    ImGui::Text("Type: %s", obj_type_name);
    ImGui::Separator();

    render_object_properties(obj, dtime);

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::Text("Controllers");
    ImGui::Separator();

    const auto &controllers = m_selected_object->get_controllers();
    for (const auto &ctrl : controllers)
    {
        if (ctrl && ImGui::CollapsingHeader(ctrl->type().name(),
                                            ImGuiTreeNodeFlags_DefaultOpen))
            render_object_properties(ctrl, dtime);
    }

    auto db     = m_app.get_asset_database();
    string path = "";
    if (db &&
        draw_asset_selector("Add Controller", path, asset_type::script, db))
    {
        script_system *sys = m_app.get_script_system();
        if (sys)
        {
            if (path.size() > 2 && path[0] == '.' && path[1] == '/')
                path = path.substr(2);

            script_instance *inst =
                sys->load_script(path.c_str(), m_selected_object->id());
            if (inst)
            {
                m_selected_object->add_controller(inst);
            }
            else
            {
                string err = "Failed to load script: " + path;
                m_app.get_console().log_error(err);
            }
        }
    }

    string native_path = "";
    if (db && draw_asset_selector("Add Native Controller",
                                  native_path,
                                  asset_type::native_controller,
                                  db))
    {
        auto rm = m_app.get_resource_manager();
        if (rm)
        {
            object *new_ctrl = object::create_default(native_path, *rm);
            if (new_ctrl)
            {
                pointer<controller> ctrl = c_dynamic_cast<controller>(new_ctrl);
                if (ctrl)
                {
                    m_selected_object->add_controller(ctrl);
                }
                else
                {
                    delete new_ctrl;
                    m_app.get_console().log_error(
                        "Failed to cast native object to controller: " +
                        native_path);
                }
            }
            else
            {
                m_app.get_console().log_error(
                    "Failed to create native controller: " + native_path);
            }
        }
    }
}

void inspector_window::asset_inspector(real dtime)
{
    ImGui::Text("Asset %s", m_selected_asset_path.c_str());

    if (m_selected_resource)
    {
        resource_ref ref{m_selected_asset_path, m_app.get_resource_manager()};
        auto &resource_type = m_selected_resource->type();

        if (resource_type.is_derived(asset_bundle::TYPE))
        {
            auto bundle =
                static_pointer_cast<asset_bundle>(m_selected_resource);

            if (ImGui::TreeNodeEx("Bundle Contents",
                                  ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (const auto &sub : bundle->get_resources())
                {
                    if (ImGui::TreeNode(sub.name.c_str()))
                    {
                        ImGui::Text("Type: %s", sub.res->type().name());

                        string sub_path =
                            m_selected_asset_path + "@" + sub.name;
                        resource_ref sub_ref{sub_path,
                                             m_app.get_resource_manager()};

                        auto preview_callback =
                            editor_registry::find_preview(sub.res->type());
                        if (preview_callback)
                            preview_callback(&sub_ref, m_app, dtime);

                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
        else
        {
            auto preview_callback =
                editor_registry::find_preview(resource_type);
            if (preview_callback)
                preview_callback(&ref, m_app, dtime);
        }
    }

    if (m_current_importer)
    {
        string name = m_current_importer->name();
        ImGui::Text("Importer: %s", name.c_str());
        render_importer_options(m_current_options);
    }
}

void inspector_window::render_importer_options(
    vector<struct importer_option> &options)
{
    if (options.empty())
        return;

    ImGui::Separator();
    ImGui::TextDisabled("Import Settings");

    // Gather groups
    vector<string> groups;
    for (const auto &opt : options)
    {
        bool found = false;
        for (const auto &g : groups)
        {
            if (g == opt.group)
            {
                found = true;
                break;
            }
        }
        if (!found)
            groups.push_back(opt.group);
    }

    for (const auto &g : groups)
    {
        bool show_group = true;

        if (!g.empty())
        {
            show_group =
                ImGui::TreeNodeEx(g.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
        }

        if (show_group)
        {
            for (auto &opt : options)
            {
                if (opt.group != g)
                    continue;

                ImGui::PushID(opt.name.c_str());
                const char *label = opt.display_name.empty()
                                        ? opt.name.c_str()
                                        : opt.display_name.c_str();

                if (opt.current_value.is_bool())
                {
                    bool b = opt.current_value.as_bool();
                    if (ImGui::Checkbox(label, &b))
                    {
                        opt.current_value = value(b);
                    }
                }
                else if (opt.current_value.is_number())
                {
                    float f = (float)opt.current_value.as_number();
                    if (ImGui::DragFloat(label, &f))
                    {
                        opt.current_value = value((double)f);
                    }
                }
                else if (opt.current_value.is_string())
                {
                    char buffer[256];
                    string_view s = opt.current_value.as_string();
                    strncpy(buffer, s.data(), sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = 0;
                    if (ImGui::InputText(label, buffer, sizeof(buffer)))
                    {
                        opt.current_value = value(buffer);
                    }
                }

                if (!opt.description.empty())
                {
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", opt.description.c_str());
                }
                ImGui::PopID();
            }
            if (!g.empty())
                ImGui::TreePop();
        }
    }

    if (ImGui::Button("Apply"))
        store_importer_options(options, m_selected_asset_path);
}

void inspector_window::render(editor_app &app, real dtime)
{
    ImGui::Begin("Inspector");

    if (m_selected_object)
        object_inspector(m_selected_object, dtime);
    else if (!m_selected_asset_path.empty())
    {
        asset_inspector(dtime);
        ImGui::Separator();
        render_asset_info();
    }
    else if (!m_selected_file_info.name.empty())
        render_asset_info();
    else
        ImGui::Text("No object selected.");

    ImGui::End();
}

void inspector_window::on_message(const game_message &msg)
{
    if (msg.msg_id == cmd_select)
    {
        object::s_in_use.try_get_value(msg.receiver_id, m_selected_object);
    }
    else if (msg.msg_id == cmd_deselect)
    {
        if (msg.receiver_id == uuid::null() ||
            (m_selected_object && m_selected_object->id() == msg.receiver_id))
        {
            m_selected_object = nullptr;
        }
    }
    else if (msg.msg_id == evt_scene_change)
    {
        m_selected_object = nullptr;
        m_selected_asset_path.clear();
        m_current_importer = nullptr;
        m_current_options.clear();
        m_selected_resource.reset();
        m_selected_resource_valid = false;
    }
    else if (msg.msg_id == cmd_inspect_asset)
    {
        if (msg.data.is_string())
            on_inspect_asset(msg.data.as_string());
    }
}

void inspector_window::on_inspect_asset(string &&path)
{
    m_selected_object         = nullptr; // Deselect object
    m_selected_asset_path     = path;
    m_current_importer        = nullptr;
    m_selected_resource_valid = false;
    m_current_options.clear();

    size_t dot_pos = m_selected_asset_path.rfind('.');
    string ext;
    if (dot_pos != string::npos)
        ext = m_selected_asset_path.substr(dot_pos);

    m_selected_resource.reset();
    m_current_importer = importer_registry::find_importer(ext);

    auto rm = m_app.get_resource_manager();
    auto fs = rm->get_file_system();

    if (fs->is_file(path))
    {
        report(report_type::info, "Importing resource: %s", path.c_str());
        auto res_result = rm->import_resource(path);
        if (res_result)
            m_selected_resource = res_result.value;
        else
        {
            report(report_type::error,
                   "Failed to import resource: %s",
                   path.c_str());
            m_app.get_console().log_error("Failed to import resource: " + path);
        }
        load_importer_options(m_current_options, m_selected_asset_path);
    }

    if (fs)
    {
        report(report_type::info, "Getting path info: %s", path.c_str());
        m_selected_file_info = fs->get_info(m_selected_asset_path);
    }
    else
    {
        report(report_type::error,
               "Failed to get file info, missing FS: %s",
               path.c_str());
        m_selected_file_info = {};
    }

    m_selected_resource_valid = true;
}

void inspector_window::load_importer_options(
    vector<struct importer_option> &options,
    const string &path)
{
    options.clear();

    auto rm = m_app.get_resource_manager();
    if (!rm)
        return;

    auto fs = rm->get_file_system();
    if (!fs)
        return;

    string xml_path = path + ".xml";
    string xml      = fs->read_all_text(xml_path);
    if (xml.empty())
    {
        // No XML file
        if (m_current_importer)
            options = m_current_importer->get_options(*rm, path, nullptr);
        return;
    }

    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml.c_str(), xml.size()) != tinyxml2::XML_SUCCESS)
    {
        report(report_type::error,
               "Failed to parse XML file: %s",
               xml_path.c_str());
        if (m_current_importer)
            options = m_current_importer->get_options(*rm, path, nullptr);
        return;
    }

    auto root                           = doc.FirstChildElement("import");
    tinyxml2::XMLElement *settings_node = nullptr;
    if (root)
        settings_node = root->FirstChildElement("settings");

    if (m_current_importer)
        options = m_current_importer->get_options(*rm, path, settings_node);
}

void inspector_window::store_importer_options(
    const vector<struct importer_option> &options,
    const string &path)
{
    tinyxml2::XMLDocument doc;

    auto root = doc.NewElement("import");
    doc.InsertEndChild(root);

    auto importer_el = doc.NewElement("importer");
    importer_el->SetText(string(m_current_importer->name()).c_str());
    root->InsertEndChild(importer_el);

    auto settings_el = doc.NewElement("settings");
    root->InsertEndChild(settings_el);

    for (const auto &opt : options)
    {
        auto param = doc.NewElement("param");
        param->SetAttribute("name", opt.name.c_str());
        if (opt.current_value.is_bool())
            param->SetAttribute("value", opt.current_value.as_bool());
        else if (opt.current_value.is_number())
            param->SetAttribute("value", opt.current_value.as_number());
        else if (opt.current_value.is_string())
            param->SetAttribute("value",
                                string(opt.current_value.as_string()).c_str());
        settings_el->InsertEndChild(param);
    }

    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);

    string xml_path = m_selected_asset_path + ".xml";

    auto rm = m_app.get_resource_manager();
    if (!rm)
        return;

    auto fs = rm->get_file_system();
    if (!fs)
        return;

    if (!fs->write_all_text(xml_path, printer.CStr()))
    {
        m_app.get_console().log_error("Failed to write import settings");
        return;
    }

    m_app.get_console().log_success("Saved import settings");
}

void inspector_window::render_asset_info()
{
    if (!m_selected_file_info.name.empty())
    {
        ImGui::Text("Path: %s", m_selected_file_info.name.c_str());

        const char *suffixes[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
        int suffix_index       = 0;
        double size            = (double)m_selected_file_info.size;
        while (size >= 1024 && suffix_index < 6)
        {
            size /= 1024;
            suffix_index++;
        }

        ImGui::Text("Size: %.2f %s", size, suffixes[suffix_index]);
        ImGui::Text("Read-only: %s",
                    m_selected_file_info.is_read_only ? "Yes" : "No");
    }
}

} // namespace zabato::editor
