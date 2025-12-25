#include <zabato/gpu.hpp>
#include <zabato/imgui.hpp>
#include <zabato/renderer.hpp>
#include <zabato/window.hpp>
#include <zabato/world.hpp>

#include <iostream>

using namespace zabato;

#include <zabato/camera.hpp>
#include <zabato/mesh.hpp>
#include <zabato/resource.hpp>

using namespace zabato;

shared_ptr<mesh> create_cube_mesh()
{
    mesh *m = new mesh();
    m->init(mesh_flags::color, primitive_type::triangles);

    // 8 vertices
    m->set_vertex_count(8);
    m->set_position(0, vec3<real>(-1, -1, 1));
    m->set_color(0, color(0, 0, 0, 1)); // Black
    m->set_position(1, vec3<real>(1, -1, 1));
    m->set_color(1, color(1, 0, 0, 1)); // Red
    m->set_position(2, vec3<real>(1, 1, 1));
    m->set_color(2, color(1, 1, 0, 1)); // Yellow
    m->set_position(3, vec3<real>(-1, 1, 1));
    m->set_color(3, color(0, 1, 0, 1)); // Green
    m->set_position(4, vec3<real>(-1, -1, -1));
    m->set_color(4, color(0, 0, 1, 1)); // Blue
    m->set_position(5, vec3<real>(1, -1, -1));
    m->set_color(5, color(1, 0, 1, 1)); // Magenta
    m->set_position(6, vec3<real>(1, 1, -1));
    m->set_color(6, color(1, 1, 1, 1)); // White
    m->set_position(7, vec3<real>(-1, 1, -1));
    m->set_color(7, color(0, 1, 1, 1)); // Cyan

    // 12 triangles
    m->set_primitive_count(12);

    // Front
    m->set_primitive(0, triangle_primitive{{0, 1, 2}});
    m->set_primitive(1, triangle_primitive{{2, 3, 0}});
    // Back
    m->set_primitive(2, triangle_primitive{{5, 4, 7}});
    m->set_primitive(3, triangle_primitive{{7, 6, 5}});
    // Left
    m->set_primitive(4, triangle_primitive{{4, 0, 3}});
    m->set_primitive(5, triangle_primitive{{3, 7, 4}});
    // Right
    m->set_primitive(6, triangle_primitive{{1, 5, 6}});
    m->set_primitive(7, triangle_primitive{{6, 2, 1}});
    // Top
    m->set_primitive(8, triangle_primitive{{3, 2, 6}});
    m->set_primitive(9, triangle_primitive{{6, 7, 3}});
    // Bottom
    m->set_primitive(10, triangle_primitive{{4, 5, 1}});
    m->set_primitive(11, triangle_primitive{{1, 0, 4}});

    return (shared_ptr<mesh>)m;
}

int main(int argc, char **argv)
{
    std::cout << "hello world!" << std::endl;

    init_window_system();
    window *window =
        create_window(100, 100, 800, 600, "Zabato Editor", window_flags::none);
    make_context_current(window);

    gpu *gpu = init_gpu();
    imgui::init(window);

    auto last_time = get_time();

    simple_renderer rnd(*gpu);
    resource_manager res_mgr;
    world world;
    node *root = new node();
    world.set_scene_root(root);

    // Create Camera
    camera *cam = new camera();
    cam->set_perspective(
        to_rad(real(45)), real(800.0 / 600.0), real(0.1), real(100.0));
    cam->look_at({real(0), real(0), real(5)},
                 {real(0), real(0), real(0)},
                 {real(0), real(1), real(0)});
    root->attach_child(cam);

    // Create Cube
    auto cube_mesh = create_cube_mesh();
    res_mgr.add_resource("cube", cube_mesh);

    model *cube = new model();
    transformation t;
    t.make_identity();
    t.set_translate({0, 0, 0});
    cube->set_local(t);
    cube->set_resource_manager(&res_mgr);
    cube->set_mesh("cube");
    root->attach_child(cube);
    world.register_model(cube);

    while (!window->should_close())
    {
        poll_events();

        imgui::new_frame();
        ImGui::ShowDemoWindow();

        ImGui::Begin("Hello Zabato");
        ImGui::Text("ImGui is working!");
        static int click_count = 0;
        if (ImGui::Button("Click Me"))
            click_count++;
        ImGui::Text("Clicks: %d", click_count);
        ImGui::End();

        auto current_time = get_time();
        real delta_time =
            (real)(current_time - last_time) * (real(1) / real(1000));
        last_time = current_time;

        // Rotate cube
        transformation t = cube->get_local();
        quat<real> q     = quat_from_axis_angle({real(0), real(1), real(1)},
                                            delta_time * real(3.0));
        t.set_rotate(t.rotate() * q);
        cube->set_local(t);

        world.update(delta_time);

        cam->update_view_from_transform();

        gpu->new_frame();
        gpu->clear({0.243, 0.1, 0.15, 1.0}, 1.0);

        rnd.begin(*cam);
        world.render(rnd, cam);
        rnd.end();

        ImGui::Render();
        imgui::render_draw_data(ImGui::GetDrawData());

        window->swap_buffers();
    }

    zabato::imgui::shutdown();

    return 0;
}
