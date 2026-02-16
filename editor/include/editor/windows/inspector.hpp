#pragma once

#include <zabato/fs.hpp>
#include <zabato/game_message.hpp>
#include <zabato/importer.hpp>
#include <zabato/mesh.hpp>
#include <zabato/object.hpp>
#include <zabato/string.hpp>
#include <zabato/vector.hpp>

#include <zabato/base64.hpp>
#include <zabato/camera.hpp>
#include <zabato/gpu.hpp>
#include <zabato/math.hpp>
#include <zabato/shared_ptr.hpp>

namespace zabato::editor
{
class editor_app;

class inspector_window
{
public:
    inspector_window(editor_app &app) : m_app(app) {}
    void render(editor_app &app, real dtime);
    void on_message(const game_message &msg);

private:
    editor_app &m_app;
    object *m_selected_object                = nullptr;
    shared_ptr<resource> m_selected_resource = nullptr;
    fs::file_info m_selected_file_info;
    shared_ptr<class importer> m_current_importer;
    vector<struct importer_option> m_current_options;
    bool m_selected_resource_valid = false;

    symbol_ref cmd_select        = "cmd_select";
    symbol_ref cmd_deselect      = "cmd_deselect";
    symbol_ref evt_scene_change  = "evt_scene_change";
    symbol_ref cmd_inspect_asset = "cmd_inspect_asset";

    string m_selected_asset_path;

    void on_inspect_asset(string &&path);
    void load_importer_options(vector<struct importer_option> &options,
                               const string &path);
    void store_importer_options(const vector<struct importer_option> &options,
                                const string &path);

    void object_inspector(object *obj, real dtime);
    void render_object_properties(object *obj, real dtime);

    void asset_inspector(real dtime);
    void render_importer_options(vector<struct importer_option> &options);

    void render_asset_info();
};

} // namespace zabato::editor
