#pragma once

#include <zabato/console.hpp>
#include <zabato/game_message.hpp>
#include <zabato/gpu.hpp>
#include <zabato/material.hpp>
#include <zabato/renderer.hpp>
#include <zabato/script.hpp>
#include <zabato/shader_asset.hpp>
#include <zabato/window.hpp>
#include <zabato/world.hpp>

#include <editor/asset_database.hpp>
#include <editor/editor_resources.hpp>
#include <editor/notification_manager.hpp>
#include <editor/windows/animator_graph_window.hpp>
#include <editor/windows/asset_browser.hpp>
#include <editor/windows/console.hpp>
#include <editor/windows/hierarchy.hpp>
#include <editor/windows/inspector.hpp>
#include <editor/windows/scene_view.hpp>
#include <editor/windows/viewport.hpp>
#include <zabato/resource.hpp>

namespace zabato::editor
{

enum class editor_state
{
    edit,
    play,
    paused
};

class editor_app
{
public:
    editor_app(console &console);
    ~editor_app();

    void init(window *win, resource_manager *res_mgr, gpu *gpu, renderer *rnd);
    void shutdown();

    pointer<world> get_world() const { return m_world; }
    void set_world(pointer<world> w) { m_world = w; }

    void update(real delta_time);
    void render(renderer &renderer, gpu &gpu, real dtime);
    void draw_main_menu();

    void send_message(const game_message &msg);
    void dispatch_to_windows(const game_message &msg);

    resource_manager *get_resource_manager() const { return m_res_mgr; }
    editor_resources *get_resources() { return &m_resources; }
    gpu *get_gpu() const { return m_gpu; }
    class renderer *get_renderer() const { return m_renderer; }

    asset_database *get_asset_database() { return &m_asset_db; }

    void set_script_system(zabato::script_system *sys);
    zabato::script_system *get_script_system() const { return m_script_sys; }

    notification_manager *get_notification_manager()
    {
        return &m_notifications;
    }

    console &get_console() { return m_console; }

    bool should_simulate() const { return m_state == editor_state::play; }
    editor_state get_state() const { return m_state; }

private:
    void setup_dockspace();
    void process_messages();
    void draw_toolbar();
    void add_to_world(spatial *spatial, uuid to);

    void check_unsaved_changes(const game_message &pending_msg);
    void draw_unsaved_changes_popup();

    string get_scene_xml();
    bool is_scene_dirty();

    void on_play();
    void on_pause();
    void on_stop();

    window *m_window = nullptr;
    pointer<world> m_world;
    console &m_console;

    // Windows
    hierarchy_window m_hierarchy_win;
    inspector_window m_inspector_win;
    viewport_window m_game_view;
    scene_view_window m_scene_win;
    asset_browser_window m_asset_browser;
    console_window m_console_win;
    animator_graph_window m_animator_graph_win;

    // State
    editor_state m_state = editor_state::edit;
    vector<uint8_t> m_snapshot_buffer;
    string m_current_scene_path;
    bool m_scene_dirty = false;
    string m_last_saved_scene_xml;
    bool m_force_exit = false;

    game_message m_pending_message;
    bool m_has_pending_message   = false;
    bool m_trigger_unsaved_popup = false;

    bool m_initialized = false;

    game_message_queue m_editor_queue;

    resource_manager *m_res_mgr = nullptr;
    gpu *m_gpu                  = nullptr;
    renderer *m_renderer        = nullptr;
    script_system *m_script_sys = nullptr;

    asset_database m_asset_db;
    editor_resources m_resources;
    notification_manager m_notifications;
    hash_map<uuid, string> m_prefab_instances;

    symbol_ref cmd_create_node         = "cmd_create_node";
    symbol_ref cmd_select              = "cmd_select";
    symbol_ref cmd_deselect            = "cmd_deselect";
    symbol_ref evt_scene_change        = "evt_scene_change";
    symbol_ref cmd_change_tool         = "cmd_change_tool";
    symbol_ref cmd_focus_selection     = "cmd_focus_selection";
    symbol_ref cmd_delete_object       = "cmd_delete_object";
    symbol_ref cmd_save_scene          = "cmd_save_scene";
    symbol_ref cmd_new_scene           = "cmd_new_scene";
    symbol_ref cmd_load_scene          = "cmd_load_scene";
    symbol_ref cmd_instantiate_prefab  = "cmd_instantiate_prefab";
    symbol_ref cmd_create_model        = "cmd_create_model";
    symbol_ref cmd_create_light        = "cmd_create_light";
    symbol_ref cmd_inspect_asset       = "cmd_inspect_asset";
    symbol_ref cmd_exit_editor         = "cmd_exit_editor";
    symbol_ref cmd_open_project        = "cmd_open_project";
    symbol_ref cmd_open_project_folder = "cmd_open_project_folder";

public:
    void load_scene(const string &path);
    void save_scene(const string &path);
    void save_prefab(object *obj, const string &path);
    bool generate_and_save_thumbnail(const string &path);
    void register_prefab_instance(object *obj, const string &path);
    static void
    on_prefab_load_callback(void *ud, object *obj, const char *path);
};

} // namespace zabato::editor
