#include <filesystem>

#include <zabato/assimp/importer.hpp>
#include <zabato/jolt_physics_world.hpp>
#include <zabato/material_importer.hpp>
#include <zabato/path.hpp>
#include <zabato/platform.hpp>
#include <zabato/shader_importer.hpp>
#include <zabato/stb/importer.hpp>

#include <zabato/camera.hpp>
#include <zabato/error.hpp>
#include <zabato/fs.hpp>
#include <zabato/gpu.hpp>
#include <zabato/host_fs.hpp>
#include <zabato/imgui.hpp>
#include <zabato/lua/script_system.hpp>
#include <zabato/mesh.hpp>
#include <zabato/model.hpp>
#include <zabato/node.hpp>
#include <zabato/object_importer.hpp>
#include <zabato/object_resource.hpp>
#include <zabato/primitives.hpp>
#include <zabato/renderer.hpp>
#include <zabato/resource.hpp>
#include <zabato/window.hpp>
#include <zabato/world.hpp>

#include <editor/core/editor_registry.hpp>
#include <editor/editor.hpp>
#include <editor/editor_camera.hpp>
#include <editor/windows/hierarchy.hpp>
#include <editor/windows/inspector.hpp>
#include <editor/windows/viewport.hpp>

using namespace zabato;

int main(int argc, char **argv)
{
    report(report_type::info, "Starting Zabato Editor (Modular + Docking)...");

    init_window_system();
    window *window =
        create_window(100, 100, 1280, 720, "Zabato Editor", window_flags::none);
    make_context_current(window);

    gpu *gpu       = init_gpu();
    auto last_time = get_time();

    resource_manager *res_mgr = new resource_manager();

    assimp::register_importer();
    stb::register_importer();
    material_importer::register_importer();
    shader_importer::register_importer();
    register_object_importer(res_mgr);

    editor::register_material_inspector();
    editor::register_mesh_inspector();
    editor::register_texture_inspector();
    editor::register_object_inspector();

    console console;
    pointer<world> world = new class world();
    physics::physics_world *phys_world =
        new physics::jolt::jolt_physics_world();
    world->set_physics(phys_world);
    phys_world->set_gravity({real(0), real(-9.81), real(0)});

    // Register Built-in Meshes
    auto cube_mesh   = primitives::create_cube();
    auto sphere_mesh = primitives::create_sphere(1, 6, 11);
    auto plane_mesh  = primitives::create_plane(10, 1);

    res_mgr->add_resource("builtin:cube", cube_mesh);
    res_mgr->add_resource("builtin:sphere", sphere_mesh);
    res_mgr->add_resource("builtin:plane", plane_mesh);

    { // Create first scene
        pointer<node> root = new node();
        root->set_name("Root");
        world->set_scene_root(root.get());

        // Create Camera
        pointer<camera> cam = new camera();
        cam->set_name("Main Camera");
        cam->set_perspective(
            to_rad(real(45)), real(800.0 / 600.0), real(0.1), real(100.0));
        cam->look_at({real(0), real(0), real(5)},
                     {real(0), real(0), real(0)},
                     {real(0), real(1), real(0)});
        root->attach_child(cam);

        // Initial Cube Model
        model *cube = new model();
        cube->set_name("Cube");
        transformation t;
        t.make_identity();
        t.set_translate({0, 0, 0});
        cube->set_local(t);
        cube->set_resource_manager(res_mgr);
        cube->set_mesh("builtin:cube");
        root->attach_child(cube);
        world->register_model(cube);
    }

    zabato::platform::initialize();

    auto exe_dir        = zabato::fs::get_exe_dir_path();
    string project_path = zabato::fs::get_current_dir_path();

    auto projects_dir = zabato::fs::join(exe_dir, "projects");
    std::filesystem::create_directory(projects_dir.c_str());

    string selected = zabato::platform::open_folder_dialog(projects_dir);
    if (!selected.empty())
        project_path = selected;

    zabato::fs::virtual_fs fs;

    auto project_fs = zabato::fs::host_fs::create(project_path);
    if (project_fs.has_error())
    {
        report(report_type::error,
               "Failed to open project: %s",
               project_path.c_str());
        return -1;
    }

    auto scripts_dir         = zabato::fs::join(exe_dir, "scripts");
    auto embedded_scripts_fs = zabato::fs::host_fs::create(scripts_dir);
    if (embedded_scripts_fs.has_error())
    {
        report(report_type::error,
               "Failed to create Virtual File System for scripts.");
        return -1;
    }

    auto embedded_assets_dir = zabato::fs::join(exe_dir, "assets");
    auto embedded_assets_fs  = zabato::fs::host_fs::create(embedded_assets_dir);
    if (embedded_assets_fs.has_error())
    {
        report(report_type::error,
               "Failed to create Virtual File System for assets.");
        return -1;
    }

    fs.mount("/", project_fs.value);
    fs.mount("/embedded/scripts", embedded_scripts_fs.value);
    fs.mount("/embedded/assets", embedded_assets_fs.value);

    res_mgr->set_file_system(&fs);
    lua_script_system lua_sys(fs, console);

    if (lua_sys.initialize())
        report(report_type::info, "Lua System Initialized.");
    else
        report(report_type::error, "Failed to initialize Lua System.");

    // Imgui
    imgui::init(window, fs);
    editor::editor_resources::install_custom_icons();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Editor App
    controller::context ctx;
    ctx.resources = res_mgr;
    ctx.logger    = &console;
    ctx.window    = window;
    ctx.scripts   = &lua_sys;
    world->set_context(ctx);

    editor::editor_app editor(console);
    forward_renderer rnd(*gpu, lua_sys);
    editor.init(window, res_mgr, gpu, &rnd);
    editor.set_script_system(&lua_sys);

    editor.set_world(world);

    const uint32_t FPS        = 60;
    const uint32_t frameDelay = 1000 / FPS;
    while (!window->should_close())
    {
        poll_events();
        lua_sys.tick(); // GC tick

        auto current_time = get_time();
        auto diff_time    = current_time - last_time;
        real delta_time =
            (real)(current_time - last_time) * (real(1) / real(1000));
        last_time = current_time;

        imgui::new_frame();

        if (editor.should_simulate())
            world->update(delta_time);

        editor.update(delta_time);
        editor.render(rnd, *gpu, delta_time);

        gpu->new_frame();
        gpu->clear({0.243, 0.1, 0.15, 1.0}, 1.0);

        ImGui::Render();
        imgui::render_draw_data(ImGui::GetDrawData());

        window->swap_buffers();

        if (diff_time < frameDelay)
            zabato::sleep(frameDelay - diff_time);
    }

    editor.shutdown();
    zabato::imgui::shutdown();
    zabato::platform::shutdown();

    return 0;
}
