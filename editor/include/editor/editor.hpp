#pragma once

#include <zabato/game_message.hpp>
#include <zabato/gpu.hpp>
#include <zabato/renderer.hpp>
#include <zabato/script.hpp>
#include <zabato/window.hpp>
#include <zabato/world.hpp>

#include <editor/asset_database.hpp>
#include <editor/editor_resources.hpp>
#include <editor/notification_manager.hpp>
#include <editor/windows/asset_browser.hpp>
#include <editor/windows/console.hpp>
#include <editor/windows/hierarchy.hpp>
#include <editor/windows/inspector.hpp>
#include <editor/windows/scene_view.hpp>
#include <editor/windows/viewport.hpp>
#include <zabato/resource.hpp>

using namespace zabato;

namespace editor
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
    editor_app();
    ~editor_app();

    void init(window *win, resource_manager *res_mgr, gpu *gpu);
    void shutdown();

    void update(real delta_time, world &world);
    void render(world &world, renderer &renderer, camera &game_cam, gpu &gpu);

    void send_message(const game_message &msg);

    resource_manager *get_resource_manager() const { return m_res_mgr; }
    editor_resources *get_resources() { return &m_resources; }

    asset_database *get_asset_database() { return &m_asset_db; }

    void set_script_system(zabato::script_system *sys);
    zabato::script_system *get_script_system() const { return m_script_sys; }

    void add_notification(const zabato::string &msg, notification_type type)
    {
        m_notifications.add(msg, type);
    }

    notification_manager *get_notification_manager()
    {
        return &m_notifications;
    }

    bool should_simulate() const { return m_state == editor_state::play; }
    editor_state get_state() const { return m_state; }

private:
    void setup_dockspace();
    void process_messages(world &world);
    void draw_toolbar(world &world);

    void on_play(world &world);
    void on_pause();
    void on_stop(world &world);

    window *m_window = nullptr;

    // Windows
    hierarchy_window m_hierarchy_win;
    inspector_window m_inspector_win;
    viewport_window m_game_view;
    scene_view_window m_scene_win;
    asset_browser_window m_asset_browser;
    console_window m_console_win;

    // State
    editor_state m_state = editor_state::edit;
    vector<uint8_t> m_snapshot_buffer;

    bool m_initialized = false;

    // Message System
    zabato::game_message_queue m_editor_queue;

    resource_manager *m_res_mgr         = nullptr;
    zabato::script_system *m_script_sys = nullptr;
    asset_database m_asset_db;
    editor_resources m_resources;
    notification_manager m_notifications;
    hash_map<uuid, string> m_prefab_instances;

public:
    void load_scene(world &world, const string &path);
    void save_scene(world &world, const string &path);
    void save_prefab(object *obj, const string &path);
    void register_prefab_instance(object *obj, const string &path);
    static void
    on_prefab_load_callback(void *ud, object *obj, const char *path);
};

} // namespace editor
