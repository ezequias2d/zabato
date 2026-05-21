#include <editor/editor.hpp>
#include <editor/gizmo_registry.hpp>
#include <imgui.h>
#include <zabato/error.hpp>
#include <zabato/object.hpp>

#include <zabato/animator.hpp>
#include <zabato/animator_graph.hpp>
#include <zabato/animator_state.hpp>
#include <zabato/camera.hpp>
#include <zabato/controller.hpp>
#include <zabato/fs.hpp>
#include <zabato/gizmos.hpp>
#include <zabato/host_fs.hpp>
#include <zabato/imgui.hpp>
#include <zabato/light.hpp>
#include <zabato/model.hpp>
#include <zabato/node.hpp>
#include <zabato/platform.hpp>
#include <zabato/script.hpp>
#include <zabato/serializer.hpp>
#include <zabato/stb/image_utils.hpp>
#include <zabato/stream.hpp>
#include <zabato/symbol.hpp>
#include <zabato/xml_serializer.hpp>

#include <zabato/jolt/jolt_character_controller.hpp>
#include <zabato/jolt/jolt_rigid_body_controller.hpp>
// #include <zabato/jolt/jolt_vehicle_controller.hpp>
#include <zabato/jolt_physics_world.hpp>

namespace zabato::editor
{

namespace
{
void merge_xml(tinyxml2::XMLElement *dst, const tinyxml2::XMLElement *src)
{
    // Merge Attributes
    for (const tinyxml2::XMLAttribute *attr = src->FirstAttribute(); attr;
         attr                               = attr->Next())
    {
        dst->SetAttribute(attr->Name(), attr->Value());
    }

    // Merge Children
    for (const tinyxml2::XMLElement *srcChild = src->FirstChildElement();
         srcChild;
         srcChild = srcChild->NextSiblingElement())
    {
        const char *prefab_id          = srcChild->Attribute("prefab_id");
        tinyxml2::XMLElement *dstMatch = nullptr;

        if (prefab_id)
        {
            for (auto d = dst->FirstChildElement(); d;
                 d      = d->NextSiblingElement())
            {
                const char *dId = d->Attribute("id");
                if (dId && strcmp(prefab_id, dId) == 0)
                {
                    dstMatch = d;
                    break;
                }
            }
        }

        if (dstMatch)
        {
            merge_xml(dstMatch, srcChild);
        }
        else
        {
            // Match by name
            const char *name = srcChild->Attribute("name");
            if (name)
            {
                for (auto d = dst->FirstChildElement(); d;
                     d      = d->NextSiblingElement())
                {
                    const char *dName = d->Attribute("name");
                    if (dName && strcmp(name, dName) == 0)
                    {
                        dstMatch = d;
                        break;
                    }
                }
            }

            // Match by tag name
            if (!dstMatch && !name)
            {
                const char *tagName = srcChild->Name();
                for (auto d = dst->FirstChildElement(); d;
                     d      = d->NextSiblingElement())
                {
                    if (strcmp(d->Name(), tagName) == 0)
                    {
                        dstMatch = d;
                        break;
                    }
                }
            }

            if (dstMatch)
            {
                merge_xml(dstMatch, srcChild);
            }
            else
            {
                tinyxml2::XMLNode *clone =
                    srcChild->DeepClone(dst->GetDocument());
                dst->InsertEndChild(clone);
            }
        }
    }
}

struct FactoryHelper
{
    template <typename T>
    static object *create(xml_serializer &s, tinyxml2::XMLElement &el)
    {
        const char *prefab_path = el.Attribute("prefab");
        if (prefab_path)
        {
            // Load Prefab DOM
            tinyxml2::XMLDocument prefabDoc;

            auto fs    = s.get_manager()->get_file_system();
            string xml = fs->read_all_text(prefab_path);

            tinyxml2::XMLError err = prefabDoc.Parse(xml.c_str(), xml.size());

            // Try assets/ prefix if failed
            if (err != tinyxml2::XML_SUCCESS)
            {
                xml = fs->read_all_text(string{"assets/"} + prefab_path);
                err = prefabDoc.Parse(xml.c_str(), xml.size());
            }

            if (err == tinyxml2::XML_SUCCESS)
            {
                tinyxml2::XMLElement *prefabRoot = prefabDoc.RootElement();
                if (prefabRoot)
                {
                    // MERGE Scene Overrides (el) INTO Prefab Root (prefabRoot)
                    merge_xml(prefabRoot, &el);

                    // Create and Load
                    object *obj = new T();
                    // Use the main serializer 's' but with the Merged Element
                    obj->load_xml(s, *prefabRoot);
                    return obj;
                }
            }
            report(report_type::error,
                   "Editor Factory: Failed to load prefab %s",
                   prefab_path);
        }

        // Normal Case: Create and Load
        object *obj = new T();
        obj->load_xml(s, el);
        return obj;
    }
};
} // namespace

editor_app::editor_app(zabato::console &console)
    : m_game_view("Game View"), m_console(console), m_console_win(console),
      m_notifications(console), m_inspector_win(*this), m_world(nullptr)
{
    // Ensure factory is initialized
    object::initialize_factory();
    register_script_importer();

    object::register_type<node>();
    object::register_type<camera>();
    object::register_type<model>();
    object::register_type<light>();
    object::register_type<animator>();
    object::register_type<animator_clip_state>();
    object::register_type<animator_graph>();

    object::register_type<physics::jolt::jolt_rigid_body_controller>();
    object::register_type<physics::jolt::jolt_character_controller>();
}

editor_app::~editor_app() {}

void editor_app::init(window *win,
                      resource_manager *res_mgr,
                      gpu *gpu,
                      class renderer *rnd)
{
    m_window   = win;
    m_res_mgr  = res_mgr;
    m_gpu      = gpu;
    m_renderer = rnd;

    // Init Resources
    res_mgr->set_gpu(gpu);
    m_resources.init(res_mgr, gpu);

    m_asset_db.init(res_mgr, this);

    // Init Windows
    m_game_view.init();
    m_scene_win.init(win);
    m_asset_browser.init(res_mgr, &m_asset_db);

    // Game View only needs overlay, scene render is basic game view
    m_initialized = true;
}

void editor_app::shutdown()
{
    m_scene_win.shutdown();
    m_resources.shutdown();
}

void editor_app::update(real delta_time)
{
    process_messages();
    m_scene_win.update(delta_time);
    m_notifications.update(delta_time);

    // Shortcuts
    if (!ImGui::GetIO().WantTextInput && !m_scene_win.is_capturing_input())
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Q))
        {
            game_message msg;
            msg.msg_id    = cmd_change_tool;
            msg.sender_id = uuid();
            msg.data      = (int64_t)0; // select
            send_message(msg);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_W))
        {
            game_message msg;
            msg.msg_id    = cmd_change_tool;
            msg.sender_id = uuid();
            msg.data      = (int64_t)1; // move
            send_message(msg);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_E))
        {
            game_message msg;
            msg.msg_id    = cmd_change_tool;
            msg.sender_id = uuid();
            msg.data      = (int64_t)2; // rotate
            send_message(msg);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R))
        {
            game_message msg;
            msg.msg_id    = cmd_change_tool;
            msg.sender_id = uuid();
            msg.data      = (int64_t)3; // scale
            send_message(msg);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F))
        {
            game_message msg;
            msg.msg_id    = cmd_focus_selection;
            msg.sender_id = uuid();
            send_message(msg);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        {
            game_message msg;
            msg.msg_id    = cmd_delete_object;
            msg.sender_id = uuid();
            send_message(msg);
        }
    }
}

void editor_app::draw_main_menu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene"))
            {
                game_message msg;
                msg.msg_id    = cmd_new_scene;
                msg.sender_id = uuid();
                check_unsaved_changes(msg);
            }

            if (ImGui::MenuItem("Load Scene"))
            {
                zabato::vector<zabato::platform::file_filter> filters;
                filters.push_back({"Scene Files", "zfile"});

                string default_path = "";
                if (m_res_mgr->get_file_system())
                {
                    auto res =
                        m_res_mgr->get_file_system()->get_native_path("scenes");
                    if (!res.has_error())
                        default_path = res.value;
                }

                string path =
                    zabato::platform::open_file_dialog(default_path, filters);
                if (!path.empty())
                {
                    game_message msg;
                    msg.msg_id    = cmd_load_scene;
                    msg.sender_id = uuid();
                    msg.data      = value(path);
                    check_unsaved_changes(msg);
                }
            }

            if (ImGui::MenuItem("Save Scene"))
            {
                if (!m_current_scene_path.empty())
                {
                    game_message msg;
                    msg.msg_id    = cmd_save_scene;
                    msg.sender_id = uuid();
                    msg.data      = value(m_current_scene_path);
                    send_message(msg);
                }
                else
                {
                    zabato::vector<zabato::platform::file_filter> filters;
                    filters.push_back({"Scene Files", "zfile"});

                    string default_path = "";
                    if (m_res_mgr->get_file_system())
                    {
                        auto res =
                            m_res_mgr->get_file_system()->get_native_path(
                                "scenes");
                        if (!res.has_error())
                            default_path = res.value;
                    }

                    string path = zabato::platform::save_file_dialog(
                        default_path, "main.zfile", filters);
                    if (!path.empty())
                    {
                        game_message msg;
                        msg.msg_id    = cmd_save_scene;
                        msg.sender_id = uuid();
                        msg.data      = value(path);
                        send_message(msg);
                    }
                }
            }

            if (ImGui::MenuItem("Save Scene As..."))
            {
                zabato::vector<zabato::platform::file_filter> filters;
                filters.push_back({"Scene Files", "zfile"});

                string default_path = "";
                if (m_res_mgr->get_file_system())
                {
                    auto res =
                        m_res_mgr->get_file_system()->get_native_path("scenes");
                    if (!res.has_error())
                        default_path = res.value;
                }

                string path = zabato::platform::save_file_dialog(
                    default_path, "main.zfile", filters);
                if (!path.empty())
                {
                    game_message msg;
                    msg.msg_id    = cmd_save_scene;
                    msg.sender_id = uuid();
                    msg.data      = value(path);
                    send_message(msg);
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Open Project..."))
            {
                if (m_res_mgr->get_file_system())
                {
                    // Attempt to cast to virtual_fs
                    auto *vfs = static_cast<zabato::fs::virtual_fs *>(
                        m_res_mgr->get_file_system());

                    // Get current native path to use as default?
                    string default_path = "";
                    auto current_native = vfs->get_native_path("/");
                    if (!current_native.has_error())
                        default_path = current_native.value;

                    string new_path =
                        zabato::platform::open_folder_dialog(default_path);

                    if (!new_path.empty())
                    {
                        printf("Open Project Dialog returned: %s\n",
                               new_path.c_str());
                        game_message msg;
                        msg.msg_id    = cmd_open_project;
                        msg.sender_id = uuid();
                        msg.data      = value(new_path);
                        check_unsaved_changes(msg);
                    }
                    else
                    {
                        printf("Open Project Dialog cancelled or returned "
                               "empty.\n");
                    }
                }

                if (ImGui::MenuItem("Open Project Folder"))
                {
                    if (m_res_mgr->get_file_system())
                    {
                        auto res =
                            m_res_mgr->get_file_system()->get_native_path("/");
                        if (!res.has_error())
                        {
                            game_message msg;
                            msg.msg_id    = cmd_open_project_folder;
                            msg.sender_id = uuid();
                            send_message(msg);
                        }
                    }
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                {
                    game_message msg;
                    msg.msg_id    = cmd_exit_editor;
                    msg.sender_id = uuid();
                    check_unsaved_changes(msg);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Delete", "Del"))
            {
                game_message msg;
                msg.msg_id    = cmd_delete_object;
                msg.sender_id = uuid();
                send_message(msg);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Assets"))
        {
            if (ImGui::MenuItem("Refresh"))
            {
                m_asset_db.refresh();
                m_asset_browser.refresh();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Object"))
        {
            if (ImGui::MenuItem("Create Node"))
            {
                game_message msg;
                msg.msg_id    = cmd_create_node;
                msg.sender_id = uuid();
                send_message(msg);
            }
            if (ImGui::MenuItem("Create Model"))
            {
                game_message msg;
                msg.msg_id    = cmd_create_model;
                msg.sender_id = uuid();
                send_message(msg);
            }
            if (ImGui::BeginMenu("Light"))
            {
                if (ImGui::MenuItem("Directional"))
                {
                    game_message msg;
                    msg.msg_id    = cmd_create_light;
                    msg.sender_id = uuid();
                    msg.data      = (int64_t)light_type::directional;
                    send_message(msg);
                }
                if (ImGui::MenuItem("Point"))
                {
                    game_message msg;
                    msg.msg_id    = cmd_create_light;
                    msg.sender_id = uuid();
                    msg.data      = (int64_t)light_type::point;
                    send_message(msg);
                }
                if (ImGui::MenuItem("Spot"))
                {
                    game_message msg;
                    msg.msg_id    = cmd_create_light;
                    msg.sender_id = uuid();
                    msg.data      = (int64_t)light_type::spot;
                    send_message(msg);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::MenuItem("Animator Graph"))
                m_animator_graph_win.open_empty(*this);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            ImGui::Text("Zabato Engine Editor v0.1");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void editor_app::render(renderer &renderer, gpu &gpu, real dtime)
{
    // Intercept Window Close
    if (m_window->should_close() && !m_force_exit)
    {
        m_window->set_should_close(false);
        game_message msg;
        msg.msg_id    = cmd_exit_editor;
        msg.sender_id = uuid();
        check_unsaved_changes(msg);
    }

    draw_unsaved_changes_popup();

    draw_main_menu();
    draw_toolbar();
    setup_dockspace();

    m_hierarchy_win.render(m_world->get_scene_root(), *this);
    m_inspector_win.render(*this, dtime);
    m_asset_browser.render(*this);
    m_console_win.render(*this);
    m_animator_graph_win.render(*this, dtime);

    pointer<camera> game_cam = m_world->find_camera();
    m_game_view.render(*m_world, renderer, game_cam, gpu);
    m_scene_win.render(*m_world, renderer, gpu, *this);

    m_notifications.render();
}

void editor_app::setup_dockspace()
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    }
}

void editor_app::send_message(const game_message &msg)
{
    m_editor_queue.push(msg);
}

void editor_app::process_messages()
{
    game_message msg;
    while (m_editor_queue.pop(msg))
    {
        // Handle Editor Commands
        if (msg.msg_id == cmd_create_node)
        {
            m_scene_dirty   = true;
            pointer<node> n = new node();
            n->set_name("New Node");

            object *parent_obj = nullptr;
            if (msg.receiver_id != uuid::null())
                object::s_in_use.try_get_value(msg.receiver_id, parent_obj);

            pointer<node> parent =
                parent_obj
                    ? c_dynamic_cast<node>(parent_obj)
                    : c_dynamic_cast<node>(m_world->get_scene_root().get());
            if (parent)
                parent->attach_child(n);
        }
        else if (msg.msg_id == cmd_exit_editor)
        {
            m_scene_dirty          = false;
            m_last_saved_scene_xml = get_scene_xml();
            m_force_exit           = true;
            m_window->set_should_close(true);
        }
        else if (msg.msg_id == cmd_load_scene)
        {
            if (msg.data.type() == value_type::STRING)
                load_scene(msg.data.as_string());
        }
        else if (msg.msg_id == cmd_save_scene)
        {
            string path = msg.data.type() == value_type::STRING
                              ? msg.data.as_string()
                              : string_view{m_current_scene_path};
            if (!path.empty())
                save_scene(path);
        }
        else if (msg.msg_id == cmd_open_project)
        {
            printf("Processing cmd_open_project message.\n");
            if (msg.data.type() == value_type::STRING)
            {
                string new_path = msg.data.as_string();
                printf("Cmd Open Project Path: %s\n", new_path.c_str());
                if (m_res_mgr->get_file_system())
                {
                    auto *vfs = static_cast<zabato::fs::virtual_fs *>(
                        m_res_mgr->get_file_system());
                    if (vfs)
                    {
                        auto new_fs_res = zabato::fs::host_fs::create(new_path);
                        if (!new_fs_res.has_error())
                        {
                            vfs->unmount("/");
                            vfs->mount("/", new_fs_res.value);
                            m_asset_db.refresh();
                            m_asset_browser.refresh();

                            // New Scene
                            game_message new_scene_msg;
                            new_scene_msg.msg_id    = cmd_new_scene;
                            new_scene_msg.sender_id = uuid();
                            send_message(new_scene_msg);
                        }
                    }
                }
            }
        }
        else if (msg.msg_id == cmd_open_project_folder)
        {
            if (m_res_mgr->get_file_system())
            {
                auto res = m_res_mgr->get_file_system()->get_native_path("/");
                if (!res.has_error())
                {
                    zabato::platform::open_folder(res.value);
                }
            }
        }
        else if (msg.msg_id == cmd_create_light)
        {
            m_scene_dirty = true;
            light *l      = new light();
            l->set_name("New Light");

            if (msg.data.is_int())
            {
                light_data d = l->get_data();
                d.type       = (light_type)msg.data.as_int();
                l->set_data(d);
            }

            add_to_world(l, msg.receiver_id);
            // m_world->register_light(l);
        }
        else if (msg.msg_id == cmd_create_model)
        {
            m_scene_dirty = true;
            model *m      = new model();
            m->set_name("New Model");
            m->set_resource_manager(m_res_mgr);

            add_to_world(m, msg.receiver_id);
            // world.register_model(m);
        }
        else if (msg.msg_id == cmd_new_scene)
        {
            dispatch_to_windows(game_message(cmd_deselect));
            m_world->clean();

            pointer<node> root = new node();
            root->set_name("Root");
            m_world->set_scene_root(root.get());

            pointer<camera> cam = new camera();
            cam->set_name("Main Camera");
            cam->set_perspective(
                to_rad(real(45)), real(800.0 / 600.0), real(0.1), real(100.0));
            cam->look_at({real(0), real(0), real(5)},
                         {real(0), real(0), real(0)},
                         {real(0), real(1), real(0)});
            root->attach_child(cam);
            m_world->set_active_camera(cam);

            pointer<light> l = new light();
            l->set_name("Directional Light");
            light_data d = l->get_data();
            d.type       = light_type::directional;
            l->set_data(d);

            pointer<camera> temp_cam = new camera();
            temp_cam->look_at(
                vec3<real>(5, 5, 5), vec3<real>(0, 0, 0), vec3<real>(0, 1, 0));
            l->set_local(temp_cam->get_local());
            root->attach_child(l);
            m_world->register_light(l);

            m_console.log_success("New Scene Created");
            m_current_scene_path   = "";
            m_scene_dirty          = false;
            m_last_saved_scene_xml = get_scene_xml();

            // Broadcast scene change
            game_message msg;
            msg.msg_id      = evt_scene_change;
            msg.sender_id   = uuid();
            msg.receiver_id = uuid::null();
            send_message(msg);
        }
        else if (msg.msg_id == cmd_load_scene)
        {
            if (msg.data.is_string())
            {
                load_scene(msg.data.as_string());
            }
        }
        else if (msg.msg_id == cmd_instantiate_prefab)
        {
            if (msg.data.is_string())
            {
                string path = msg.data.as_string();
                xml_serializer s;
                s.set_manager(m_res_mgr);
                s.set_remap_ids(true);
                pointer<object> obj =
                    s.load(*m_res_mgr->get_file_system(), path.c_str());
                pointer<spatial> spt = c_dynamic_cast<spatial>(obj.get());
                if (!spt)
                {
                    m_console.log_error("Cannot instantiate a prefab that do "
                                        "not have a spatial root node.");
                    continue;
                }

                add_to_world(spt, msg.receiver_id);
                register_prefab_instance(obj, path);

                /*
                auto reg_func = [&](spatial *s, auto &&self) -> void
                {
                    if (!s)
                        return;
                    if (auto m = c_dynamic_cast<model>(s))
                        m_world->register_model(m);
                    if (auto l = c_dynamic_cast<light>(s))
                        m_world->register_light(l);
                    if (auto n = c_dynamic_cast<node>(s))
                    {
                        for (int i = 0; i < n->quantity(); ++i)
                            self(n->child_at(i), self);
                    }
                };
                reg_func(spt, reg_func);
                */
            }
        }
        else if (msg.msg_id == cmd_delete_object)
        {
            if (msg.receiver_id != uuid::null())
            {
                object *obj = nullptr;
                if (object::s_in_use.try_get_value(msg.receiver_id, obj))
                {
                    // Force Deselect to avoid dangling pointers
                    game_message deselect_msg;
                    deselect_msg.msg_id      = cmd_deselect;
                    deselect_msg.receiver_id = msg.receiver_id;
                    dispatch_to_windows(deselect_msg);

                    /*

                    // Detach and Unregister
                    auto unreg = [&](spatial *s, auto &&self) -> void
                    {
                        if (!s)
                            return;
                        if (auto m = c_dynamic_cast<model>(s))
                            m_world->unregister_model(m);
                        if (auto l = c_dynamic_cast<light>(s))
                            m_world->unregister_light(l);

                        // Controllers?
                        m_world->unregister_controllers_recursive(s);

                        if (auto n = c_dynamic_cast<node>(s))
                        {
                            for (int i = 0; i < n->quantity(); ++i)
                                self(n->child_at(i), self);
                        }
                    };
                    */

                    if (pointer<spatial> s = c_dynamic_cast<spatial>(obj))
                    {
                        // Unregister recursively
                        // unreg(s, unreg);

                        // Detach from parent
                        if (s->parent())
                        {
                            if (pointer<node> p =
                                    c_dynamic_cast<node>(s->parent()))
                            {
                                p->detach_child(s);
                            }
                        }
                    }
                }
            }
        }
        dispatch_to_windows(msg);

        // Forward to Game World
        m_world->send_message(msg);
    }
}

void editor_app::add_to_world(spatial *spatial, uuid to)
{
    object *parent_obj = nullptr;
    if (to != uuid::null())
        object::s_in_use.try_get_value(to, parent_obj);

    pointer<node> parent = nullptr;
    while (parent_obj != nullptr && parent == nullptr)
    {
        if (parent_obj->is_derived(node::TYPE))
            parent = c_dynamic_cast<node>(parent_obj);
        else if (parent_obj->is_derived(spatial::TYPE))
            parent_obj = static_cast<class spatial *>(parent_obj)->parent();
    }

    if (parent == nullptr)
        parent = c_dynamic_cast<node>(m_world->get_scene_root().get());

    if (parent)
        parent->attach_child(spatial);
}

void editor_app::dispatch_to_windows(const game_message &msg)
{
    m_scene_win.on_message(msg);
    m_inspector_win.on_message(msg);
    m_hierarchy_win.on_message(msg, *this);
    m_asset_browser.on_message(msg);
    m_animator_graph_win.on_message(msg, *this);
}

void editor_app::check_unsaved_changes(const game_message &pending_msg)
{
    if (is_scene_dirty())
    {
        m_pending_message       = pending_msg;
        m_has_pending_message   = true;
        m_trigger_unsaved_popup = true;
    }
    else
        send_message(pending_msg);
}

void editor_app::draw_unsaved_changes_popup()
{
    if (m_trigger_unsaved_popup)
    {
        ImGui::OpenPopup("Unsaved Changes");
        m_trigger_unsaved_popup = false;
    }

    if (ImGui::BeginPopupModal("Unsaved Changes",
                               nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoMove))
    {
        if (m_has_pending_message)
        {
            ImGui::Text("You have unsaved changes in the scene.\nDo you want "
                        "to save them?");
            ImGui::Separator();

            if (ImGui::Button("Save", ImVec2(120, 0)))
            {
                // Save Logic
                if (!m_current_scene_path.empty())
                {
                    save_scene(m_current_scene_path);
                    send_message(m_pending_message);
                }
                else
                {
                    // Save As
                    zabato::vector<zabato::platform::file_filter> filters;
                    filters.push_back({"Scene Files", "zfile"});

                    string default_path = "";
                    if (m_res_mgr->get_file_system())
                    {
                        auto res =
                            m_res_mgr->get_file_system()->get_native_path(
                                "scenes");
                        if (!res.has_error())
                            default_path = res.value;
                    }

                    string path = zabato::platform::save_file_dialog(
                        default_path, "main.zfile", filters);
                    if (!path.empty())
                    {
                        save_scene(path);
                        send_message(m_pending_message);
                    }
                    // Cancelled save -> Do nothing
                }
                ImGui::CloseCurrentPopup();
                m_has_pending_message = false;
            }

            ImGui::SameLine();
            if (ImGui::Button("Don't Save", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
                send_message(m_pending_message);
                m_has_pending_message = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
                m_has_pending_message = false;
            }
            ImGui::EndPopup();
        }
    }
}

static void
editor_print(script_system *sys, script_instance *inst, script_args *args)
{
    editor_app *app = (editor_app *)sys->get_user_data();
    if (!app)
    {
        args->type_error(
            "script_engine expect to have editor_app* as user data");
        return;
    }

    string msg;
    for (int i = 0; i < args->count(); ++i)
    {
        if (i > 0)
            msg += "\t";
        value v = args->get_value(i);
        msg += v.as_string();
    }

    app->get_console().log_info(msg);
}

void editor_app::set_script_system(script_system *sys)
{
    m_script_sys = sys;
    if (m_script_sys)
    {
        m_script_sys->set_user_data(this);
        m_script_sys->register_global_function("print", value(&editor_print));
        m_script_sys->register_global_function("log", value(&editor_print));
    }
}

void editor_app::draw_toolbar()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, 0.0f));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags =
        0 | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("Toolbar", nullptr, window_flags);
    ImGui::PopStyleVar();

    // Play Button
    bool is_playing = (m_state == editor_state::play);
    if (!is_playing)
    {
        if (ImGui::Button("Play"))
        {
            on_play();
        }
    }
    else
    {
        if (ImGui::Button("Pause"))
        {
            on_pause();
        }
    }

    ImGui::SameLine();

    // Resume Button if paused
    if (m_state == editor_state::paused)
    {
        if (ImGui::Button("Resume"))
        {
            m_state = editor_state::play;
        }
        ImGui::SameLine();
    }

    // Stop Button
    if (m_state != editor_state::edit)
    {
        if (ImGui::Button("Stop"))
        {
            on_stop();
        }
    }

    // Calculate height
    float height = ImGui::GetWindowSize().y;
    ImGui::End();

    viewport->WorkPos.y += height;
    viewport->WorkSize.y -= height;
}

void editor_app::on_play()
{
    if (m_state == editor_state::edit)
    {
        // Snapshot
        m_snapshot_buffer.clear();

        if (m_world->get_scene_root())
        {
            xml_serializer s;

            auto &doc = s.doc();
            tinyxml2::XMLElement *rootEl =
                doc.NewElement(m_world->get_scene_root()->type().name());
            doc.InsertEndChild(rootEl);
            m_world->get_scene_root()->save_xml(s, *rootEl);

            tinyxml2::XMLPrinter printer;
            doc.Accept(&printer);
            size_t len = printer.CStrSize();
            m_snapshot_buffer.resize(len);
            memcpy(m_snapshot_buffer.data(), printer.CStr(), len);
        }
        m_state = editor_state::play;
        m_console.log_info("Entered Play Mode");
    }
    else if (m_state == editor_state::paused)
    {
        m_state = editor_state::play;
    }
}

void editor_app::on_pause()
{
    if (m_state == editor_state::play)
    {
        m_state = editor_state::paused;
        m_console.log_info("Paused");
    }
}

void editor_app::on_stop()
{
    if (m_state == editor_state::edit)
        return;

    // Restore
    if (!m_snapshot_buffer.empty())
    {
        // Broadcast scene change to clear selections/state in windows
        send_message(game_message{
            .msg_id      = evt_scene_change,
            .sender_id   = uuid::null(),
            .receiver_id = uuid::null(),
            .data        = value(),
        });

        process_messages();

        m_world->set_scene_root(nullptr);
        m_world->set_active_camera(nullptr);
        m_world->clean();

        if (!m_snapshot_buffer.empty())
        {
            xml_serializer s;
            auto &doc = s.doc();

            if (doc.Parse((const char *)m_snapshot_buffer.data()) ==
                tinyxml2::XML_SUCCESS)
            {
                if (!doc.Error())
                {
                    s.set_manager(m_res_mgr);

                    tinyxml2::XMLElement *rootEl = doc.RootElement();
                    if (rootEl)
                    {
                        string_view name = rootEl->Name();
                        object *obj      = object::factory(name);
                        if (obj)
                        {
                            obj->load_xml(s, *rootEl);
                            obj->link(s, *rootEl);

                            if (obj->is_derived(spatial::TYPE))
                                m_world->set_scene_root((spatial *)obj);
                            else
                            {
                                m_console.log_error(
                                    "Restored root is not a spatial!");
                                delete obj;
                            }
                        }
                        else
                        {
                            m_console.log_error("Failed to create "
                                                "object from snapshot");
                        }
                    }
                }
                else
                {
                    m_console.log_error("Failed to parse scene snapshot XML");
                }
            }

            m_snapshot_buffer.clear();
        }
    }

    m_state = editor_state::edit;
    m_console.log_info("Stopped");
}

string editor_app::get_scene_xml()
{
    xml_serializer s;
    s.set_manager(m_res_mgr);

    tinyxml2::XMLPrinter printer;

    if (m_world->get_scene_root())
    {
        tinyxml2::XMLElement *root =
            s.doc().NewElement(m_world->get_scene_root()->type().name());
        s.doc().InsertEndChild(root);
        m_world->get_scene_root()->save_xml(s, *root);
    }

    s.doc().Accept(&printer);
    return string(printer.CStr());
}

bool editor_app::is_scene_dirty()
{
    if (m_scene_dirty)
        return true;
    string current_xml = get_scene_xml();
    return current_xml != m_last_saved_scene_xml;
}

void editor_app::register_prefab_instance(object *obj, const string &path)
{
    if (obj)
        m_prefab_instances.add(obj->id(), path);
}

void editor_app::on_prefab_load_callback(void *ud,
                                         object *obj,
                                         const char *path)
{
    if (ud)
        ((editor_app *)ud)->register_prefab_instance(obj, path);
}

static string
generate_thumbnail(gpu *m_gpu, renderer *m_renderer, world &world, object *root)
{
    if (!m_gpu || !m_renderer)
        return "";
    int thumb_size   = 256;
    framebuffer *fbo = m_gpu->create_framebuffer(thumb_size, thumb_size);
    if (fbo)
    {
        m_gpu->push_state();
        m_gpu->bind_framebuffer(fbo);
        m_gpu->viewport(thumb_size, thumb_size);
        m_gpu->enable_depth_test(true);
        m_gpu->enable_blend(true);
        m_gpu->bind_texture(nullptr);
        m_gpu->clear(color(0.1f, 0.1f, 0.1f, 1.0f), 1.0f);

        vec3<real> min_pt(1e9, 1e9, 1e9);
        vec3<real> max_pt(-1e9, -1e9, -1e9);

        if (root && root->is_derived(spatial::TYPE))
            spatial::get_global_bounds(
                static_cast<spatial *>(root), min_pt, max_pt);

        bool found_bounds = min_pt != vec3<real>(1e9, 1e9, 1e9) ||
                            max_pt != vec3<real>(-1e9, -1e9, -1e9);

        // Setup thumbnail camera
        pointer<camera> thumb_cam = new camera();

        if (found_bounds)
        {
            vec3<real> center = (min_pt + max_pt) * 0.5f;
            real radius       = length(max_pt - min_pt) * 0.5f;
            if (radius < 0.1f)
                radius = 1.0f;

            real fov  = to_rad(real(45));
            real dist = radius / tan(fov * 0.5f);
            dist *= 1.2f;

            // Position camera (Isometric-ish view)
            vec3<real> dir = normalize(vec3<real>(0.5f, 0.5f, 1.0f));
            thumb_cam->look_at(
                center + dir * dist, center, vec3<real>(0, 1, 0));
            thumb_cam->set_perspective(fov, 1.0f, 0.1f, dist * 10.0f);
        }
        else
        {
            // Default
            thumb_cam->set_perspective(to_rad(real(45)), 1.0f, 0.1f, 100.0f);
            thumb_cam->look_at({0, 2, 5}, {0, 0, 0}, {0, 1, 0});
        }
        thumb_cam->update_view_from_transform();

        m_renderer->begin(thumb_cam);
        world.render(*m_renderer, thumb_cam);
        m_renderer->end();

        vector<uint8_t> pixels(thumb_size * thumb_size * 4);
        m_gpu->read_pixels(0,
                           0,
                           thumb_size,
                           thumb_size,
                           color_format::rgba8888,
                           pixels.data());

        m_gpu->unbind_framebuffer();
        m_gpu->pop_state();
        fbo->destroy();
        delete fbo;

        vector<uint8_t> jpg_data;
        if (stb::write_jpg_to_memory(
                pixels, thumb_size, thumb_size, stb::comp::rgba, jpg_data, 20))
        {
            return base64::encode(jpg_data.data(), jpg_data.size());
        }
    }
    return "";
}

void editor_app::save_scene(const string &path)
{
    m_current_scene_path = path;
    m_scene_dirty        = false;
    xml_serializer s;
    s.set_manager(m_res_mgr);

    // Ensure directory exists
    if (m_res_mgr && m_res_mgr->get_file_system())
    {
        size_t last_slash = path.rfind('/');
        if (last_slash != string::npos)
        {
            string dir = path.substr(0, last_slash);
            m_res_mgr->get_file_system()->mkdir(dir);
        }
    }

    auto &doc = s.doc();
    doc.Clear();
    object *root = m_world->get_scene_root();
    if (root)
    {
        tinyxml2::XMLElement *rootEl = doc.NewElement(root->type().name());
        doc.InsertEndChild(rootEl);
        root->save_xml(s, *rootEl);

        // Capture Thumbnail
        string base64 = generate_thumbnail(m_gpu, m_renderer, *m_world, root);
        if (!base64.empty())
        {
            tinyxml2::XMLElement *meta  = doc.NewElement("metadata");
            tinyxml2::XMLElement *thumb = doc.NewElement("thumbnail");
            thumb->SetText(base64.c_str());
            meta->InsertEndChild(thumb);

            // Insert as first child of root
            if (rootEl->FirstChild())
                rootEl->InsertFirstChild(meta);
            else
                rootEl->InsertEndChild(meta);
        }

        // Post-process to add prefab attributes and prune defaults
        delegate<void(tinyxml2::XMLElement *, tinyxml2::XMLElement *)>
            compare_and_prune =
                [&](tinyxml2::XMLElement *sEl, tinyxml2::XMLElement *pEl)
        {
            if (!sEl || !pEl)
                return;

            // Prune matching attributes
            const tinyxml2::XMLAttribute *pAttr = pEl->FirstAttribute();
            while (pAttr)
            {
                const char *sVal = sEl->Attribute(pAttr->Name());
                if (sVal && strcmp(sVal, pAttr->Value()) == 0)
                {
                    if (strcmp(pAttr->Name(), "id") != 0 &&
                        strcmp(pAttr->Name(), "name") != 0)
                        sEl->DeleteAttribute(pAttr->Name());
                }
                pAttr = pAttr->Next();
            }

            // Prune matching children
            tinyxml2::XMLElement *sChild = sEl->FirstChildElement();
            while (sChild)
            {
                tinyxml2::XMLElement *nextSChild = sChild->NextSiblingElement();

                // Find matching prefab child by Name or Type.
                const char *sName = sChild->Attribute("name");

                tinyxml2::XMLElement *pChild =
                    pEl->FirstChildElement(sChild->Name());
                while (pChild)
                {
                    const char *pName = pChild->Attribute("name");
                    if (sName && pName && strcmp(sName, pName) == 0)
                        break; // Found match by name
                    if (!sName && !pName)
                        break; // Found match by type

                    pChild = pChild->NextSiblingElement(sChild->Name());
                }

                if (pChild)
                {
                    // Inject Prefab ID
                    const char *pId = pChild->Attribute("id");
                    if (pId)
                        sChild->SetAttribute("prefab_id", pId);

                    compare_and_prune(sChild, pChild);
                }

                sChild = nextSChild;
            }
        };

        delegate<void(tinyxml2::XMLElement *)> process_element =
            [&](tinyxml2::XMLElement *el)
        {
            const char *idStr = el->Attribute("id");
            if (idStr)
            {
                uuid id;
                if (uuid::try_parse(idStr, id))
                {
                    string prefab_path;
                    if (m_prefab_instances.try_get_value(id, prefab_path))
                    {
                        el->SetAttribute("prefab", prefab_path.c_str());

                        // Load prefab and prune
                        tinyxml2::XMLDocument pDoc;
                        string xml =
                            m_res_mgr->get_file_system()->read_all_text(
                                prefab_path);
                        if (pDoc.Parse(xml.c_str(), xml.size()) ==
                            tinyxml2::XML_SUCCESS)
                        {
                            tinyxml2::XMLElement *pRoot = pDoc.RootElement();
                            if (pRoot)
                            {
                                compare_and_prune(el, pRoot);
                            }
                        }
                    }
                }
            }

            tinyxml2::XMLElement *child = el->FirstChildElement();
            while (child)
            {
                process_element(child);
                child = child->NextSiblingElement();
            }
        };

        process_element(rootEl);

        tinyxml2::XMLPrinter printer;
        doc.Accept(&printer);
        string_view xml = printer.CStr();
        auto fs         = m_res_mgr->get_file_system();
        if (fs->write_all_text(path, xml))
        {
            m_console.log_success("Scene saved: " + path);
            m_asset_db.refresh();
        }
        else
        {
            auto error = "Failed to save scene: " + path;
            m_console.log_error(error);
            report(report_type::error, "%s", error.c_str());
        }
    }
}

void editor_app::load_scene(const string &path)
{
    m_current_scene_path = path;
    m_scene_dirty        = false;
    m_prefab_instances.clear();

    // Clear current scene
    send_message(game_message{
        .msg_id      = evt_scene_change,
        .sender_id   = uuid::null(),
        .receiver_id = uuid::null(),
        .data        = value(),
    });

    process_messages();
    m_world->set_scene_root(nullptr);

    xml_serializer s;
    auto &doc = s.doc();
    s.set_manager(m_res_mgr);

    // Load to doc first
    string xml = m_res_mgr->get_file_system()->read_all_text(path);
    if (doc.Parse(xml.c_str(), xml.size()) != tinyxml2::XML_SUCCESS)
    {
        m_console.log_error("Failed to load scene (XML Parse): " + path);
        return;
    }

    delegate<void(tinyxml2::XMLElement *)> process_load =
        [&](tinyxml2::XMLElement *el)
    {
        const char *prefab_path = el->Attribute("prefab");
        const char *idStr       = el->Attribute("id");
        if (prefab_path && idStr)
        {
            uuid id;
            if (uuid::try_parse(idStr, id))
            {
                m_prefab_instances.add(id, prefab_path);
            }
        }

        tinyxml2::XMLElement *child = el->FirstChildElement();
        while (child)
        {
            process_load(child);
            child = child->NextSiblingElement();
        }
    };

    tinyxml2::XMLElement *rootEl = s.doc().RootElement();
    if (rootEl)
        process_load(rootEl);

    // Create Root from Doc
    if (rootEl)
    {
        string_view name = rootEl->Name();
        object *root     = object::factory(name);
        if (root)
        {
            root->load_xml(s, *rootEl);
            root->link(s, *rootEl);

            pointer<spatial> s_root = c_dynamic_cast<spatial>(root);
            if (s_root)
            {
                m_world->set_scene_root(s_root);
                m_console.log_success("Scene loaded: " + path);
            }
            else
            {
                delete root;
                m_console.log_error(
                    "Loaded object is not a spatial (Scene Root)");
            }
        }
    }
}

void editor_app::save_prefab(object *obj, const string &path)
{
    if (!obj)
        return;
    xml_serializer s;
    s.set_manager(m_res_mgr);

    // Ensure directory exists
    if (m_res_mgr && m_res_mgr->get_file_system())
    {
        size_t last_slash = path.rfind('/');
        if (last_slash != string::npos)
        {
            string dir = path.substr(0, last_slash);
            m_res_mgr->get_file_system()->mkdir(dir);
        }
    }

    if (s.save(*m_res_mgr->get_file_system(), path.c_str(), obj))
    {
        register_prefab_instance(obj, path);
        m_console.log_success("Prefab saved: " + path);
        m_asset_db.refresh();
    }
    else
    {
        m_console.log_error("Failed to save prefab: " + path);
    }
}

bool editor_app::generate_and_save_thumbnail(const string &path)
{
    xml_serializer s;
    s.set_manager(m_res_mgr);

    string xml = m_res_mgr->get_file_system()->read_all_text(path);
    if (s.doc().Parse(xml.c_str(), xml.size()) != tinyxml2::XML_SUCCESS)
    {
        m_console.log_error("Failed to load scene file for thumbnail");
        return false;
    }

    tinyxml2::XMLElement *rootEl = s.doc().RootElement();
    if (!rootEl)
        return false;

    // Load scene
    string_view name = rootEl->Name();
    object *root     = object::factory(name);
    if (!root)
        return false;
    root->load_xml(s, *rootEl);
    root->link(s, *rootEl);

    // Create temporary world
    pointer<world> tmp_world = new world();
    if (root->is_derived(spatial::TYPE))
    {
        tmp_world->set_scene_root(static_cast<spatial *>(root));
    }

    tmp_world->update(0);

    string b64 = generate_thumbnail(m_gpu, m_renderer, *tmp_world, root);

    // Cleanup
    bool managed = false;
    if (tmp_world->get_scene_root() == root)
    {
        tmp_world->set_scene_root(nullptr);
        managed = true;
    }

    if (!managed)
        delete root;

    if (b64.empty())
        return false;

    // Inject metadata
    tinyxml2::XMLElement *meta = rootEl->FirstChildElement("metadata");
    if (!meta)
    {
        meta = s.doc().NewElement("metadata");
        rootEl->InsertFirstChild(meta);
    }
    tinyxml2::XMLElement *thumb = meta->FirstChildElement("thumbnail");
    if (!thumb)
    {
        thumb = s.doc().NewElement("thumbnail");
        meta->InsertEndChild(thumb);
    }
    thumb->SetText(b64.c_str());

    tinyxml2::XMLPrinter printer;
    s.doc().Accept(&printer);
    xml = printer.CStr();

    if (!m_res_mgr->get_file_system()->write_all_text(path, xml))
    {
        m_console.log_error("Failed to save thumbnail: " + path);
        return false;
    }

    m_console.log_success("Thumbnail generated: " + path);
    return true;
}

} // namespace zabato::editor
