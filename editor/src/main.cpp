#include <zabato/gpu.hpp>
#include <zabato/imgui.hpp>
#include <zabato/window.hpp>

#include <iostream>

using namespace zabato;

int main(int argc, char **argv)
{
    std::cout << "hello world!" << std::endl;

    init_window_system();
    window *window =
        create_window(100, 100, 320, 240, "Zabato Editor", window_flags::none);
    make_context_current(window);

    gpu *gpu = init_gpu();
    imgui::init(window);

    auto last_time = get_time();
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

        gpu->new_frame();
        gpu->clear({0.243, 0.1, 0.15, 1.0}, 1.0);

        ImGui::Render();
        imgui::render_draw_data(ImGui::GetDrawData());

        window->swap_buffers();
    }

    zabato::imgui::shutdown();

    return 0;
}
