#include <editor/editor.hpp>
#include <editor/windows/hierarchy.hpp>
#include <zabato/imgui.hpp>
#include <zabato/light.hpp>
#include <zabato/model.hpp>
#include <zabato/node.hpp>
#include <zabato/platform.hpp>
#include <zabato/symbol.hpp>

namespace zabato::editor
{

void hierarchy_window::check_prefab_drop(editor_app &app, uuid parent_id)
{
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload =
                ImGui::AcceptDragDropPayload("ASSET_PATH"))
        {
            string path = (const char *)payload->Data;
            string ext =
                (path.length() > 7) ? path.substr(path.length() - 7) : "";
            string extShort =
                (path.length() > 5) ? path.substr(path.length() - 5) : "";

            if (ext == ".zfile")
            {
                game_message msg;
                msg.msg_id      = cmd_instantiate_prefab;
                msg.sender_id   = uuid();
                msg.receiver_id = parent_id;
                msg.data        = value(path.c_str());
                app.send_message(msg);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void hierarchy_window::render(spatial *root, editor_app &app)
{
    ImGui::Begin("Scene Hierarchy");

    if (ImGui::Button("Create Object..."))
        ImGui::OpenPopup("CreateObjectPopup");

    if (ImGui::BeginPopup("CreateObjectPopup"))
    {
        if (ImGui::MenuItem("Node"))
        {
            game_message msg;
            msg.msg_id    = cmd_create_node;
            msg.sender_id = uuid();
            msg.receiver_id =
                !m_selection.empty() ? m_selection.back()->id() : uuid::null();
            app.send_message(msg);
        }
        if (ImGui::BeginMenu("Light"))
        {
            if (ImGui::MenuItem("Directional Light"))
            {
                game_message msg;
                msg.msg_id      = cmd_create_light;
                msg.sender_id   = uuid();
                msg.receiver_id = !m_selection.empty()
                                      ? m_selection.back()->id()
                                      : uuid::null();
                msg.data        = (int64_t)light_type::directional;
                app.send_message(msg);
            }
            if (ImGui::MenuItem("Point Light"))
            {
                game_message msg;
                msg.msg_id      = cmd_create_light;
                msg.sender_id   = uuid();
                msg.receiver_id = !m_selection.empty()
                                      ? m_selection.back()->id()
                                      : uuid::null();
                msg.data        = (int64_t)light_type::point;
                app.send_message(msg);
            }
            if (ImGui::MenuItem("Spot Light"))
            {
                game_message msg;
                msg.msg_id      = cmd_create_light;
                msg.sender_id   = uuid();
                msg.receiver_id = !m_selection.empty()
                                      ? m_selection.back()->id()
                                      : uuid::null();
                msg.data        = (int64_t)light_type::spot;
                app.send_message(msg);
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Model (Empty)"))
        {
            game_message msg;
            msg.msg_id    = cmd_create_model;
            msg.sender_id = uuid();
            msg.receiver_id =
                !m_selection.empty() ? m_selection.back()->id() : uuid::null();
            app.send_message(msg);
        }
        ImGui::EndPopup();
    }

    if (root)
    {
        pointer<node> root_node = c_dynamic_cast<node>(root);
        if (root_node)
        {
            for (int i = 0; i < root_node->quantity(); ++i)
            {
                draw_node(root_node->child_at(i), app);
            }
        }
    }

    // invisible item to catch drops on window background
    ImGui::Dummy(ImGui::GetContentRegionAvail());
    check_prefab_drop(app, uuid::null());

    ImGui::End();
}

void hierarchy_window::on_message(const game_message &msg, editor_app &app)
{
    if (msg.msg_id == cmd_select)
    {
        object *obj = nullptr;
        if (object::s_in_use.try_get_value(msg.receiver_id, obj))
        {
            // Check if already selected
            bool found = false;
            for (auto *s : m_selection)
                if (s == obj)
                    found = true;

            if (!found)
                m_selection.push_back(obj);
        }
    }
    else if (msg.msg_id == cmd_deselect)
    {
        if (msg.receiver_id == uuid::null())
        {
            m_selection.clear();
        }
        else
        {
            // Remove specific
            for (auto it = m_selection.begin(); it != m_selection.end();)
            {
                if ((*it)->id() == msg.receiver_id)
                    it = m_selection.erase(it);
                else
                    ++it;
            }
        }
    }
    else if (msg.msg_id == evt_scene_change)
    {
        m_selection.clear();
    }
    else if (msg.msg_id == cmd_delete_object)
    {
        // If msg has no target, iterate selection
        if (msg.receiver_id == uuid::null() && !m_selection.empty())
        {
            auto selection_copy = m_selection;
            for (auto *obj : selection_copy)
            {
                game_message delete_msg;
                delete_msg.msg_id      = cmd_delete_object;
                delete_msg.sender_id   = uuid();
                delete_msg.receiver_id = obj->id();
                app.send_message(delete_msg);
            }
        }
    }
}

void hierarchy_window::draw_node(spatial *s, editor_app &app)
{
    bool is_selected = false;
    for (auto *sel : m_selection)
        if (sel == s)
        {
            is_selected = true;
            break;
        }

    ImGuiTreeNodeFlags flags = (is_selected ? ImGuiTreeNodeFlags_Selected : 0) |
                               ImGuiTreeNodeFlags_OpenOnArrow;

    // Check if node has children
    pointer<node> n = c_dynamic_cast<node>(s);
    bool is_leaf    = !n || (n->quantity() == 0);

    if (is_leaf)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    const char *name = s->name();
    if (!name || strlen(name) == 0)
        name = "Unnamed Object";

    bool opened = ImGui::TreeNodeEx((void *)s, flags, "%s", name);
    check_prefab_drop(app, s->id());

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Save as Prefab"))
        {
            auto fs = app.get_resource_manager()->get_file_system();

            vector<platform::file_filter> filters;
            filters.push_back({"Zabato Prefab", "zfile"});

            string default_path = fs->get_native_path("prefabs/");
            string path         = zabato::platform::save_file_dialog(
                "prefabs", string(name) + ".zfile", filters);

            if (!path.empty())
            {
                auto res = fs->get_virtual_path(path);
                if (!res.has_error())
                {
                    app.save_prefab(s, res.value);
                }
                else
                {
                    app.get_console().log_error(
                        "Selected path is not within the project directory.");
                }
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        game_message msg;
        msg.msg_id    = cmd_focus_selection;
        msg.sender_id = uuid();
        app.send_message(msg);
    }

    if (ImGui::IsItemClicked())
    {
        bool ctrl  = ImGui::GetIO().KeyCtrl;
        bool shift = ImGui::GetIO().KeyShift;

        if (!ctrl && !shift)
        {
            game_message clear_msg;
            clear_msg.msg_id      = cmd_deselect;
            clear_msg.sender_id   = uuid();
            clear_msg.receiver_id = uuid::null(); // Clear all
            app.send_message(clear_msg);

            // Add this one
            game_message msg;
            msg.msg_id      = cmd_select;
            msg.sender_id   = uuid();
            msg.receiver_id = s->id();
            app.send_message(msg);
        }
        else
        {
            // Toggle
            if (is_selected)
            {
                game_message msg;
                msg.msg_id      = cmd_deselect;
                msg.sender_id   = uuid();
                msg.receiver_id = s->id();
                app.send_message(msg);
            }
            else
            {
                game_message msg;
                msg.msg_id      = cmd_select;
                msg.sender_id   = uuid();
                msg.receiver_id = s->id();
                app.send_message(msg);
            }
        }
    }

    if (opened && !is_leaf)
    {
        if (n)
        {
            for (int i = 0; i < n->quantity(); ++i)
            {
                draw_node(n->child_at(i), app);
            }
        }
        ImGui::TreePop();
    }
}

} // namespace zabato::editor
