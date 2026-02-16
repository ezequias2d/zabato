#include <editor/editor.hpp>
#include <editor/windows/asset_browser.hpp>

#include <zabato/error.hpp>
#include <zabato/imgui.hpp>
#include <zabato/platform.hpp>

namespace zabato::editor
{

static void *AssetBrowserSettings_ReadOpen(void *, const char *name)
{
    if (strcmp(name, "Window") != 0)
        return nullptr;
    return (void *)1;
}

static void
AssetBrowserSettings_ReadLine(void *user_data, void *, const char *line)
{
    auto *window = (asset_browser_window *)user_data;
    int mode     = 0;
    if (sscanf(line, "ViewMode=%d", &mode) == 1)
    {
        if (mode == 0)
            window->set_view_mode(asset_browser_window::view_mode::grid);
        else if (mode == 1)
            window->set_view_mode(asset_browser_window::view_mode::list);
    }
}

static void AssetBrowserSettings_WriteAll(void *user_data, ImGuiTextBuffer *buf)
{
    auto *window = (asset_browser_window *)user_data;
    buf->appendf("[AssetBrowser][Window]\n");
    int mode =
        (window->get_view_mode() == asset_browser_window::view_mode::grid) ? 0
                                                                           : 1;
    buf->appendf("ViewMode=%d\n", mode);
    buf->appendf("\n");
}

void asset_browser_window::init(resource_manager *res_mgr)
{
    m_res_mgr      = res_mgr;
    m_current_path = "assets";
    refresh();

    imgui::settings_handler handler;
    handler.type_name    = "AssetBrowser";
    handler.type_hash    = imgui::hash_string("AssetBrowser");
    handler.user_data    = this;
    handler.read_open_fn = AssetBrowserSettings_ReadOpen;
    handler.read_line_fn = AssetBrowserSettings_ReadLine;
    handler.write_all_fn = AssetBrowserSettings_WriteAll;
    imgui::add_settings_handler(handler);
}

void asset_browser_window::refresh()
{
    if (!m_res_mgr || !m_res_mgr->get_file_system())
        return;

    // Ensure path exists, defaults to root
    if (!m_res_mgr->get_file_system()->exists(m_current_path))
    {
        m_current_path = ".";
    }

    m_entries = get_filtered_entries(m_current_path);

    // Update Path Buf
    memset(m_path_buf, 0, sizeof(m_path_buf));
    strncpy(m_path_buf, m_current_path.c_str(), sizeof(m_path_buf) - 1);
}

void asset_browser_window::navigate(const string &path)
{
    m_current_path = path;
    refresh();
}

void asset_browser_window::reveal(const string &path)
{
    if (path.empty())
        return;

    // Navigate to parent if file
    size_t last_slash = path.rfind('/');
    if (last_slash != string::npos)
    {
        string parent  = path.substr(0, last_slash);
        m_current_path = parent;
    }
    else
    {
        m_current_path = ".";
    }

    m_highlight_path = path;
    refresh();
}

void asset_browser_window::on_message(const game_message &msg)
{
    if (msg.msg_id == "cmd_reveal_asset")
    {
        if (msg.data.is_string())
        {
            reveal(string(msg.data.as_string()));
        }
    }
}

void asset_browser_window::render(editor_app &app)
{
    bool is_visible = ImGui::Begin("Asset Browser");

    if (is_visible)
    {
        if (!m_res_mgr)
        {
            ImGui::Text("Resource Manager not connected.");
        }
        else
        {
            // Header Layout
            real window_width = ImGui::GetContentRegionAvail().x;
            real button_size  = ImGui::GetFrameHeight();
            real spacing      = ImGui::GetStyle().ItemSpacing.x;

            const char *view_label =
                (m_view_mode == view_mode::grid) ? "List" : "Grid";
            real view_btn_width = ImGui::CalcTextSize(view_label).x +
                                  ImGui::GetStyle().FramePadding.x * 2.0f;

            real fixed_width =
                (button_size * 3) + view_btn_width + (spacing * 5);

            real available_for_inputs = window_width - fixed_width;
            if (available_for_inputs < 50.0f)
                available_for_inputs = 50.0f;

            real path_width   = available_for_inputs * 0.6f;
            real search_width = available_for_inputs * 0.4f;

            // Back Button
            if (ImGui::Button("<", ImVec2((float)button_size, 0)))
            {
                // Go up
                size_t last_slash = m_current_path.rfind('/');
                if (last_slash != string::npos && last_slash > 0)
                {
                    navigate(m_current_path.substr(0, last_slash));
                }
                else
                {
                    navigate(".");
                }
            }
            ImGui::SameLine();

            // Refresh Button
            if (ImGui::Button("R", ImVec2((float)button_size, 0)))
            {
                refresh();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Refresh");
            ImGui::SameLine();

            // Path Input
            ImGui::PushItemWidth((float)path_width);
            if (ImGui::InputText("##Path",
                                 m_path_buf,
                                 sizeof(m_path_buf),
                                 ImGuiInputTextFlags_EnterReturnsTrue))
            {
                navigate(m_path_buf);
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();

            // Search Input
            ImGui::PushItemWidth((float)search_width);
            static char search_buf[256] = "";
            if (ImGui::InputTextWithHint(
                    "##Search", "Search...", search_buf, sizeof(search_buf)))
            {
                m_search_query = search_buf;
                m_is_searching = !m_search_query.empty();
                refresh();
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();

            // Create Button
            if (ImGui::Button("+", ImVec2((float)button_size, 0)))
            {
                ImGui::OpenPopup("CreateAssetPopup");
            }
            if (ImGui::BeginPopup("CreateAssetPopup"))
            {
                create_context_menu_content(app);
                ImGui::EndPopup();
            }
            ImGui::SameLine();

            // View Mode Button
            if (ImGui::Button(view_label))
            {
                m_view_mode = (m_view_mode == view_mode::grid)
                                  ? view_mode::list
                                  : view_mode::grid;
            }

            ImGui::Separator();

            if (ImGui::BeginChild("AssetBrowserContent"))
            {
                // Background Context Menu
                if (ImGui::BeginPopupContextWindow(
                        nullptr,
                        ImGuiPopupFlags_MouseButtonRight |
                            ImGuiPopupFlags_NoOpenOverItems))
                {
                    if (ImGui::BeginMenu("Create"))
                    {
                        create_context_menu_content(app);
                        ImGui::EndMenu();
                    }
                    ImGui::EndPopup();
                }

                if (m_view_mode == view_mode::grid)
                    render_grid(app);
                else
                    render_list(app, m_current_path);
            }
            ImGui::EndChild();
        }

        // Modal Popups
        action_delete_ui();
    }
    ImGui::End();
}

void asset_browser_window::create_context_menu_content(
    editor_app &app,
    const string &parent_path)
{
    if (ImGui::MenuItem("Folder"))
        create_folder("NewFolder", parent_path);
    if (ImGui::MenuItem("Material"))
    {
        const char *content =
            "<material>\n"
            "    <shader path=\"embedded/scripts/shaders/Lit.zshader\"/>\n"
            "    <state z_write=\"true\" z_test=\"less\" cull=\"back\" "
            "blend=\"false\"/>\n"
            "</material>";
        create_file("NewMaterial.zmaterial", content, parent_path);
    }
    if (ImGui::MenuItem("Physics Material"))
    {
        const char *content = "<physics_material>\n"
                              "    <friction value=\"0.5\"/>\n"
                              "    <restitution value=\"0.0\"/>\n"
                              "</physics_material>";
        create_file("NewPhysicsMaterial.zpm", content, parent_path);
    }
    if (ImGui::MenuItem("Character Config"))
    {
        const char *content =
            "<character_config>\n"
            "    <mass value=\"500.0\"/>\n"
            "    <max_slope_angle value=\"0.8\"/>\n"
            "    <max_strength value=\"100.0\"/>\n"
            "    <shape type=\"capsule\" radius=\"0.2\" height=\"1.8\"/>\n"
            "</character_config>";
        create_file("NewCharacterConfig.zcc", content, parent_path);
    }
    if (ImGui::MenuItem("Rigid Body Config"))
    {
        const char *content =
            "<rigid_body_config>\n"
            "    <body_type value=\"Dynamic\"/>\n"
            "    <motion_quality value=\"Discrete\"/>\n"
            "    <mass value=\"1.0\"/>\n"
            "    <shape type=\"box\">\n"
            "        <half_extents x=\"0.5\" y=\"0.5\" z=\"0.5\"/>\n"
            "    </shape>\n"
            "</rigid_body_config>";
        create_file("NewRigidBodyConfig.zrb", content, parent_path);
    }
    if (ImGui::MenuItem("Shader"))
    {
        const char *content =
            "return {\n"
            "    name = \"NewShader\",\n"
            "    uniforms = {\n"
            "        u_Color = \"vec4\"\n"
            "    },\n"
            "    vertex = function(ctx)\n"
            "        local pos = ctx.Input.Position()\n"
            "        local mvp = ctx.Matrix.MVP()\n"
            "        ctx.Output.Position(ctx.mul(mvp, pos))\n"
            "    end,\n"
            "    fragment = function(ctx)\n"
            "        local col = ctx.uniform(\"u_Color\", \"vec4\")\n"
            "        ctx.Output.Color(col)\n"
            "    end\n"
            "}";
        create_file("NewShader.zshader", content, parent_path);
    }
    if (ImGui::MenuItem("Script"))
    {
        const char *content = "-- New Script\n"
                              "function start()\n"
                              "end\n\n"
                              "function update(dt)\n"
                              "end";
        create_file("NewScript.lua", content, parent_path);
    }
}

void asset_browser_window::create_folder(const string &dirname,
                                         const string &parent_path)
{
    if (!m_res_mgr)
        return;
    auto *fs = m_res_mgr->get_file_system();
    if (!fs)
        return;

    string target_path = parent_path.empty() ? m_current_path : parent_path;
    string full_path   = target_path;
    full_path += "/";
    full_path += dirname;

    // Find unique name
    int idx     = 1;
    string base = dirname;
    while (fs->exists(full_path))
    {
        full_path = target_path;
        full_path += "/";
        full_path += base;
        full_path += "_";
        full_path += std::to_string(idx).c_str();
        idx++;
    }

    if (fs->mkdir(full_path))
    {
        refresh();
        refresh();
        action_rename_trigger(full_path);
    }
}

void asset_browser_window::create_file(const string &filename,
                                       const string &content,
                                       const string &parent_path)
{
    if (!m_res_mgr)
        return;
    auto *fs = m_res_mgr->get_file_system();
    if (!fs)
        return;

    string target_path = parent_path.empty() ? m_current_path : parent_path;
    string full_path   = target_path;
    full_path += "/";
    full_path += filename;

    // Find unique name
    int idx        = 1;
    size_t ext_pos = filename.rfind('.');
    string base =
        (ext_pos != string::npos) ? filename.substr(0, ext_pos) : filename;
    string ext = (ext_pos != string::npos) ? filename.substr(ext_pos) : "";

    while (fs->exists(full_path))
    {
        full_path = target_path;
        full_path += "/";
        full_path += base;
        full_path += "_";
        full_path += std::to_string(idx).c_str();
        full_path += ext;
        idx++;
    }

    if (fs->write_all_text(full_path, content))
    {
        refresh();
        refresh();
        action_rename_trigger(full_path);
    }
}

void asset_browser_window::action_rename_trigger(const string &full_path)
{
    m_rename_path = full_path;
    // Extract filename
    size_t last_slash = full_path.rfind('/');
    string name       = (last_slash != string::npos)
                            ? full_path.substr(last_slash + 1)
                            : full_path;
    memset(m_rename_buf, 0, sizeof(m_rename_buf));
    strncpy(m_rename_buf, name.c_str(), sizeof(m_rename_buf) - 1);

    m_is_renaming = true;
}

void asset_browser_window::action_delete_trigger(const string &full_path)
{
    m_delete_path       = full_path;
    m_delete_popup_open = true;
}

void asset_browser_window::action_console_inline_rename()
{
    if (m_res_mgr && m_res_mgr->get_file_system())
    {
        // Construct new path
        size_t last_slash = m_rename_path.rfind('/');
        string parent     = (last_slash != string::npos)
                                ? m_rename_path.substr(0, last_slash)
                                : "";

        string new_path = parent;
        if (!new_path.empty())
            new_path += "/";
        new_path += m_rename_buf;

        if (new_path != m_rename_path)
        {
            if (m_res_mgr->get_file_system()->rename(m_rename_path, new_path))
            {
                // Success
            }
        }
        refresh();
    }
    m_is_renaming = false;
    m_rename_path = "";
}

void asset_browser_window::action_delete_ui()
{
    if (m_delete_popup_open)
    {
        ImGui::OpenPopup("Delete Asset");
        m_delete_popup_open = false;
    }

    if (ImGui::BeginPopupModal(
            "Delete Asset", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Are you sure you want to delete this asset?\nThis "
                    "operation cannot be undone!");
        ImGui::Text("%s", m_delete_path.c_str());

        if (ImGui::Button("Delete", ImVec2(120, 0)))
        {
            if (m_res_mgr && m_res_mgr->get_file_system())
            {
                m_res_mgr->get_file_system()->remove(m_delete_path);
                refresh();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void asset_browser_window::action_move(const string &src, const string &dst)
{
    if (m_res_mgr && m_res_mgr->get_file_system())
    {
        size_t last_slash = src.rfind('/');
        string filename =
            (last_slash != string::npos) ? src.substr(last_slash + 1) : src;

        string full_dst = dst;
        if (full_dst.back() != '/')
            full_dst += "/";
        full_dst += filename;

        m_res_mgr->get_file_system()->rename(src, full_dst);
        refresh();
    }
}

void asset_browser_window::render_grid(editor_app &app)
{
    float padding     = 16.0f;
    float thumbnail_s = 64.0f;
    float cell_size   = thumbnail_s + padding;

    float panel_width = ImGui::GetContentRegionAvail().x;
    int column_count  = (int)(panel_width / cell_size);
    if (column_count < 1)
        column_count = 1;

    ImGui::Columns(column_count, 0, false);

    // Get Resources
    auto *resources = app.get_resources();

    // Defer navigation to avoid iterator invalidation
    string navigate_to = "";

    for (const auto &entry : m_entries)
    {
        ImGui::PushID(entry.name.c_str());

        // Select Icon
        editor_icon icon_id = editor_icon::file;
        if (entry.is_dir)
            icon_id = editor_icon::folder;
        else
            icon_id = resources->get_icon_id_for_file(entry.name);

        const char *icon_str = resources->get_icon_str(icon_id);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1)); // No tint
        ImGui::SetWindowFontScale(3.0f);
        ImGui::Button(icon_str, ImVec2(thumbnail_s, thumbnail_s));
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor(); // Text

        // Construct full path
        string full_path = m_current_path;
        if (full_path == ".")
            full_path = entry.name;
        else
        {
            if (full_path.back() != '/')
                full_path += "/";
            full_path += entry.name;
        }

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(
                "ASSET_PATH", full_path.c_str(), full_path.length() + 1);
            ImGui::Text("%s", entry.name.c_str());
            ImGui::EndDragDropSource();
        }

        if (entry.is_dir)
        {
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload *payload =
                        ImGui::AcceptDragDropPayload("ASSET_PATH"))
                {
                    const char *src_path = (const char *)payload->Data;
                    action_move(src_path, full_path);
                }
                ImGui::EndDragDropTarget();
            }
        }

        ImGui::PopStyleColor();

        if (ImGui::IsItemClicked(0))
        {
            action_inspect(app, full_path);
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            if (entry.is_dir)
                navigate_to = full_path;
            else
                action_open(app, full_path);
        }

        if (ImGui::BeginPopupContextItem())
        {
            entry_context_menu(entry, app, m_current_path);
            ImGui::EndPopup();
        }

        // Inline Rename Logic
        if (m_is_renaming && full_path == m_rename_path)
        {
            ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##Rename",
                                 m_rename_buf,
                                 sizeof(m_rename_buf),
                                 ImGuiInputTextFlags_EnterReturnsTrue |
                                     ImGuiInputTextFlags_AutoSelectAll))
            {
                action_console_inline_rename();
            }
            // Check for cancellation (Escape or Focus Loss)
            if (!ImGui::IsItemActive() &&
                (ImGui::IsMouseClicked(0) ||
                 ImGui::IsKeyPressed(ImGuiKey_Escape)))
            {
                if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    m_is_renaming = false;
                }
                else if (ImGui::IsMouseClicked(0))
                {
                    action_console_inline_rename();
                }
            }
        }
        else
        {
            // TODO: add some time?
            bool highlight = false;
            if (m_highlight_path == full_path)
            {
                highlight = true;
                ImGui::SetScrollHereY();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
                m_highlight_path.clear();
            }

            ImGui::TextWrapped("%s", entry.name.c_str());

            if (highlight)
                ImGui::PopStyleColor();
        }

        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);

    if (!navigate_to.empty())
        navigate(navigate_to);
}

void asset_browser_window::render_list(editor_app &app, const string &path)
{
    vector<fs::file_info> entries;
    if (path == m_current_path)
        entries = m_entries;
    else
        entries = get_filtered_entries(path);

    // Get Resources
    auto *resources = app.get_resources();

    for (const auto &entry : entries)
    {
        ImGui::PushID(entry.name.c_str());

        // Construct full path for this entry
        string full_path = path;
        if (full_path == ".")
            full_path = entry.name;
        else
        {
            full_path += "/";
            full_path += entry.name;
        }

        if (entry.is_dir)
        {
            // Folder Node
            string node_id = "##";
            node_id += entry.name;
            bool open = ImGui::TreeNodeEx(node_id.c_str(),
                                          ImGuiTreeNodeFlags_SpanAvailWidth);

            // Context Menu
            if (ImGui::BeginPopupContextItem())
            {
                entry_context_menu(entry, app, path);
                ImGui::EndPopup();
            }

            // Drag Drop Source
            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload(
                    "ASSET_PATH", full_path.c_str(), full_path.length() + 1);
                ImGui::Text("%s", entry.name.c_str());
                ImGui::EndDragDropSource();
            }

            // Drag Drop Target
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload *payload =
                        ImGui::AcceptDragDropPayload("ASSET_PATH"))
                {
                    const char *src_path = (const char *)payload->Data;
                    action_move(src_path, full_path);
                }
                ImGui::EndDragDropTarget();
            }

            // Manually draw Icon and Text
            ImGui::SameLine();

            editor_icon icon_id  = editor_icon::folder;
            const char *icon_str = resources->get_icon_str(icon_id);

            // Inline Rename
            if (m_is_renaming && full_path == m_rename_path)
            {
                ImGui::Text("%s", icon_str);
                ImGui::SameLine();
                ImGui::SetKeyboardFocusHere();
                if (ImGui::InputText("##Rename",
                                     m_rename_buf,
                                     sizeof(m_rename_buf),
                                     ImGuiInputTextFlags_EnterReturnsTrue |
                                         ImGuiInputTextFlags_AutoSelectAll))
                {
                    action_console_inline_rename();
                }
                if (!ImGui::IsItemActive())
                {
                    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                        m_is_renaming = false;
                    else if (ImGui::IsMouseClicked(0))
                        action_console_inline_rename();
                }
            }
            else
            {
                string icon_text = string_view(icon_str);
                icon_text += entry.name;
                ImGui::Text("%s", icon_text.c_str());
            }

            if (open)
            {
                render_list(app, full_path);
                ImGui::TreePop();
            }
        }
        else
        {
            // File Rendering
            editor_icon icon_id  = resources->get_icon_id_for_file(entry.name);
            const char *icon_str = resources->get_icon_str(icon_id);

            // Use TreeNodeEx with Leaf flag
            string node_id = "##";
            node_id += entry.name;
            ImGui::TreeNodeEx(node_id.c_str(),
                              ImGuiTreeNodeFlags_Leaf |
                                  ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                  ImGuiTreeNodeFlags_SpanAvailWidth);

            // Construct full path for actions
            string full_path = path;
            if (full_path == ".")
                full_path = entry.name;
            else
            {
                if (full_path.back() != '/')
                    full_path += "/";
                full_path += entry.name;
            }

            if (ImGui::BeginPopupContextItem())
            {
                entry_context_menu(entry, app, path);
                ImGui::EndPopup();
            }

            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload(
                    "ASSET_PATH", full_path.c_str(), full_path.length() + 1);
                ImGui::Text("%s", entry.name.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::IsItemClicked())
                action_inspect(app, full_path);

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                action_open(app, full_path);

            ImGui::SameLine();

            if (m_is_renaming && full_path == m_rename_path)
            {
                ImGui::SetKeyboardFocusHere();
                if (ImGui::InputText("##Rename",
                                     m_rename_buf,
                                     sizeof(m_rename_buf),
                                     ImGuiInputTextFlags_EnterReturnsTrue |
                                         ImGuiInputTextFlags_AutoSelectAll))
                {
                    action_console_inline_rename();
                }
                if (!ImGui::IsItemActive())
                {
                    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                        m_is_renaming = false;
                    else if (ImGui::IsMouseClicked(0))
                        action_console_inline_rename();
                }
            }
            else
            {
                string icon_text = string_view(icon_str);
                icon_text += entry.name;
                ImGui::Text("%s", icon_text.c_str());
            }
        }
        ImGui::PopID();
    }
}

void asset_browser_window::entry_context_menu(const fs::file_info &entry,
                                              editor_app &app,
                                              const string &parent_path)
{
    string full_path = parent_path;
    if (full_path != ".")
    {
        if (full_path.back() != '/')
            full_path += "/";
    }
    else
        full_path = "";
    full_path += entry.name;

    if (ImGui::MenuItem("Open in Explorer"))
    {
        if (m_res_mgr && m_res_mgr->get_file_system())
        {
            auto native_path =
                m_res_mgr->get_file_system()->get_native_path(full_path);
            if (!native_path.has_error())
            {
                zabato::platform::reveal_file(native_path.value);
            }
        }
    }

    if (!entry.is_dir)
    {
        if (ImGui::MenuItem("Open on default"))
        {
            if (m_res_mgr && m_res_mgr->get_file_system())
            {
                auto native_path =
                    m_res_mgr->get_file_system()->get_native_path(full_path);
                if (!native_path.has_error())
                {
                    zabato::platform::open_file_default(native_path.value);
                }
            }
        }
    }

    ImGui::Separator();

    if (entry.is_dir)
    {
        if (ImGui::BeginMenu("Create"))
        {
            create_context_menu_content(app, full_path);
            ImGui::EndMenu();
        }
    }

    if (ImGui::MenuItem("Open"))
    {
        if (entry.is_dir)
            navigate(full_path);
        else
            action_open(app, full_path);
    }

    if (ImGui::MenuItem("Rename"))
    {
        action_rename_trigger(full_path);
    }

    if (ImGui::MenuItem("Delete"))
    {
        action_delete_trigger(full_path);
    }

    if (!entry.is_dir)
    {
        if (ImGui::MenuItem("Import Asset"))
        {
            action_import(full_path);
        }
    }
}

vector<fs::file_info>
asset_browser_window::get_filtered_entries(const string &path)
{
    if (!m_res_mgr || !m_res_mgr->get_file_system())
        return {};

    vector<fs::file_info> raw;

    if (m_is_searching && !m_search_query.empty())
    {
        raw = m_res_mgr->get_file_system()->ls(path);
    }
    else
    {
        raw = m_res_mgr->get_file_system()->ls(path);
    }

    vector<fs::file_info> filtered;
    filtered.reserve(raw.size());

    for (const auto &entry : raw)
    {
        bool hide = false;

        // Filter by Search
        if (m_is_searching && !m_search_query.empty())
        {
            // If manual filter needed
            if (entry.name.find(m_search_query) == string::npos)
                hide = true;
        }

        if (!entry.is_dir && !hide)
        {
            // Hide .ext.xml if .ext exists
            if (entry.name.size() > 4 &&
                entry.name.substr(entry.name.size() - 4) == ".xml")
            {
                string base = entry.name.substr(0, entry.name.size() - 4);
                // Check if base exists
                for (const auto &check : raw)
                {
                    if (check.name == base)
                    {
                        hide = true;
                        break;
                    }
                }
            }
        }
        if (!hide)
            filtered.push_back(entry);
    }
    return filtered;
}

void asset_browser_window::action_inspect(editor_app &app,
                                          const string &full_path)
{
    game_message msg;
    msg.msg_id      = "cmd_inspect_asset";
    msg.sender_id   = uuid();
    msg.receiver_id = uuid::null();
    msg.data        = value(full_path);
    app.send_message(msg);
}

void asset_browser_window::action_open(editor_app &app, const string &full_path)
{
    string ext = (full_path.length() > 6)
                     ? full_path.substr(full_path.length() - 6)
                     : "";
    if (ext == ".zfile")
    {
        game_message msg;
        msg.msg_id      = "cmd_load_scene";
        msg.sender_id   = uuid();
        msg.receiver_id = uuid::null();
        msg.data        = value(full_path.c_str());
        app.send_message(msg);
    }
}

void asset_browser_window::action_import(const string &full_path)
{
    if (!m_res_mgr)
        return;

    auto res = m_res_mgr->import_resource(full_path);

    if (res.has_error())
        report(report_type::error,
               "[Editor] Import failed: %s",
               get_error_message(res.error));
    else
        report(report_type::info,
               "[Editor] Import successful: %s",
               full_path.c_str());
}

} // namespace zabato::editor