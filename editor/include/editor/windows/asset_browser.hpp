#pragma once

#include <editor/asset_database.hpp>
#include <editor/windows/viewport.hpp>
#include <zabato/fs.hpp>
#include <zabato/material.hpp>
#include <zabato/resource.hpp>
#include <zabato/vector.hpp>

namespace zabato::editor
{

class editor_app;

class asset_browser_window
{
public:
    enum class view_mode
    {
        list,
        grid
    };

    void init(resource_manager *res_mgr, asset_database *db);
    void render(editor_app &app);
    void refresh();

    view_mode get_view_mode() const { return m_view_mode; }
    void set_view_mode(view_mode mode) { m_view_mode = mode; }

    void reveal(const string &path);
    void on_message(const game_message &msg);

private:
    void navigate(const string &path);
    void entry_context_menu(const fs::file_info &entry,
                            editor_app &app,
                            const string &parent_path);
    void create_context_menu_content(editor_app &app,
                                     const string &parent_path = "");

    resource_manager *m_res_mgr = nullptr;
    asset_database *m_db        = nullptr;
    string m_current_path;
    string m_highlight_path;
    vector<fs::file_info> m_entries;
    view_mode m_view_mode = view_mode::grid;

    // Search
    string m_search_query;
    bool m_is_searching = false;

    // Actions State
    string m_rename_path;
    char m_rename_buf[256];
    bool m_is_renaming = false;

    string m_delete_path;
    bool m_delete_popup_open = false;

    // Path Input
    char m_path_buf[512];

    void render_grid(editor_app &app);
    void render_list(editor_app &app, const string &path);
    vector<fs::file_info> get_filtered_entries(const string &path);

    // Helpers
    void create_file(const string &filename,
                     const string &content,
                     const string &parent_path = "");
    void create_folder(const string &dirname, const string &parent_path = "");
    void action_delete_ui();

    // Actions
    void action_inspect(editor_app &app, const string &full_path);
    void action_open(editor_app &app, const string &full_path);
    void action_import(const string &full_path);
    void action_rename_trigger(const string &full_path);
    void action_console_inline_rename(); // Added
    void action_delete_trigger(const string &full_path);
    void action_move(const string &src, const string &dst);
};

} // namespace zabato::editor
