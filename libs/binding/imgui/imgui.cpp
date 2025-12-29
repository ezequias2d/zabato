#include <zabato/gpu.hpp>
#include <zabato/imgui.hpp>
#include <zabato/vector.hpp>
#include <zabato/window.hpp>

namespace
{
zabato::window *g_window                    = nullptr;
zabato::gpu *g_gpu                          = nullptr;
zabato::texture *g_font_texture             = nullptr;
uint64_t g_time                             = 0;
zabato::window_flags g_backend_window_flags = zabato::window_flags::none;

void update_modifiers(zabato::modifier_keys mods)
{
    ImGuiIO &io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl,
                   (mods & zabato::modifier_keys::control) !=
                       zabato::modifier_keys::none);
    io.AddKeyEvent(ImGuiMod_Shift,
                   (mods & zabato::modifier_keys::shift) !=
                       zabato::modifier_keys::none);
    io.AddKeyEvent(ImGuiMod_Alt,
                   (mods & zabato::modifier_keys::alt) !=
                       zabato::modifier_keys::none);
    io.AddKeyEvent(ImGuiMod_Super,
                   (mods & zabato::modifier_keys::super) !=
                       zabato::modifier_keys::none);
}

ImGuiKey key_to_imgui(zabato::key_code key)
{
    switch (key)
    {
    case zabato::key_code::tab:
        return ImGuiKey_Tab;
    case zabato::key_code::left:
        return ImGuiKey_LeftArrow;
    case zabato::key_code::right:
        return ImGuiKey_RightArrow;
    case zabato::key_code::up:
        return ImGuiKey_UpArrow;
    case zabato::key_code::down:
        return ImGuiKey_DownArrow;
    case zabato::key_code::page_up:
        return ImGuiKey_PageUp;
    case zabato::key_code::page_down:
        return ImGuiKey_PageDown;
    case zabato::key_code::home:
        return ImGuiKey_Home;
    case zabato::key_code::end:
        return ImGuiKey_End;
    case zabato::key_code::insert:
        return ImGuiKey_Insert;
    case zabato::key_code::del:
        return ImGuiKey_Delete;
    case zabato::key_code::backspace:
        return ImGuiKey_Backspace;
    case zabato::key_code::space:
        return ImGuiKey_Space;
    case zabato::key_code::enter:
        return ImGuiKey_Enter;
    case zabato::key_code::escape:
        return ImGuiKey_Escape;
    case zabato::key_code::apostrophe:
        return ImGuiKey_Apostrophe;
    case zabato::key_code::comma:
        return ImGuiKey_Comma;
    case zabato::key_code::minus:
        return ImGuiKey_Minus;
    case zabato::key_code::period:
        return ImGuiKey_Period;
    case zabato::key_code::slash:
        return ImGuiKey_Slash;
    case zabato::key_code::semicolon:
        return ImGuiKey_Semicolon;
    case zabato::key_code::equal:
        return ImGuiKey_Equal;
    case zabato::key_code::left_bracket:
        return ImGuiKey_LeftBracket;
    case zabato::key_code::backslash:
        return ImGuiKey_Backslash;
    case zabato::key_code::right_bracket:
        return ImGuiKey_RightBracket;
    case zabato::key_code::grave_accent:
        return ImGuiKey_GraveAccent;
    case zabato::key_code::caps_lock:
        return ImGuiKey_CapsLock;
    case zabato::key_code::scroll_lock:
        return ImGuiKey_ScrollLock;
    case zabato::key_code::num_lock:
        return ImGuiKey_NumLock;
    case zabato::key_code::print_screen:
        return ImGuiKey_PrintScreen;
    case zabato::key_code::pause:
        return ImGuiKey_Pause;
    case zabato::key_code::kp_0:
        return ImGuiKey_Keypad0;
    case zabato::key_code::kp_1:
        return ImGuiKey_Keypad1;
    case zabato::key_code::kp_2:
        return ImGuiKey_Keypad2;
    case zabato::key_code::kp_3:
        return ImGuiKey_Keypad3;
    case zabato::key_code::kp_4:
        return ImGuiKey_Keypad4;
    case zabato::key_code::kp_5:
        return ImGuiKey_Keypad5;
    case zabato::key_code::kp_6:
        return ImGuiKey_Keypad6;
    case zabato::key_code::kp_7:
        return ImGuiKey_Keypad7;
    case zabato::key_code::kp_8:
        return ImGuiKey_Keypad8;
    case zabato::key_code::kp_9:
        return ImGuiKey_Keypad9;
    case zabato::key_code::kp_decimal:
        return ImGuiKey_KeypadDecimal;
    case zabato::key_code::kp_divide:
        return ImGuiKey_KeypadDivide;
    case zabato::key_code::kp_multiply:
        return ImGuiKey_KeypadMultiply;
    case zabato::key_code::kp_subtract:
        return ImGuiKey_KeypadSubtract;
    case zabato::key_code::kp_add:
        return ImGuiKey_KeypadAdd;
    case zabato::key_code::kp_enter:
        return ImGuiKey_KeypadEnter;
    case zabato::key_code::kp_equal:
        return ImGuiKey_KeypadEqual;
    case zabato::key_code::left_shift:
        return ImGuiKey_LeftShift;
    case zabato::key_code::left_control:
        return ImGuiKey_LeftCtrl;
    case zabato::key_code::left_alt:
        return ImGuiKey_LeftAlt;
    case zabato::key_code::left_super:
        return ImGuiKey_LeftSuper;
    case zabato::key_code::right_shift:
        return ImGuiKey_RightShift;
    case zabato::key_code::right_control:
        return ImGuiKey_RightCtrl;
    case zabato::key_code::right_alt:
        return ImGuiKey_RightAlt;
    case zabato::key_code::right_super:
        return ImGuiKey_RightSuper;
    case zabato::key_code::menu:
        return ImGuiKey_Menu;
    case zabato::key_code::num_0:
        return ImGuiKey_0;
    case zabato::key_code::num_1:
        return ImGuiKey_1;
    case zabato::key_code::num_2:
        return ImGuiKey_2;
    case zabato::key_code::num_3:
        return ImGuiKey_3;
    case zabato::key_code::num_4:
        return ImGuiKey_4;
    case zabato::key_code::num_5:
        return ImGuiKey_5;
    case zabato::key_code::num_6:
        return ImGuiKey_6;
    case zabato::key_code::num_7:
        return ImGuiKey_7;
    case zabato::key_code::num_8:
        return ImGuiKey_8;
    case zabato::key_code::num_9:
        return ImGuiKey_9;
    case zabato::key_code::a:
        return ImGuiKey_A;
    case zabato::key_code::b:
        return ImGuiKey_B;
    case zabato::key_code::c:
        return ImGuiKey_C;
    case zabato::key_code::d:
        return ImGuiKey_D;
    case zabato::key_code::e:
        return ImGuiKey_E;
    case zabato::key_code::f:
        return ImGuiKey_F;
    case zabato::key_code::g:
        return ImGuiKey_G;
    case zabato::key_code::h:
        return ImGuiKey_H;
    case zabato::key_code::i:
        return ImGuiKey_I;
    case zabato::key_code::j:
        return ImGuiKey_J;
    case zabato::key_code::k:
        return ImGuiKey_K;
    case zabato::key_code::l:
        return ImGuiKey_L;
    case zabato::key_code::m:
        return ImGuiKey_M;
    case zabato::key_code::n:
        return ImGuiKey_N;
    case zabato::key_code::o:
        return ImGuiKey_O;
    case zabato::key_code::p:
        return ImGuiKey_P;
    case zabato::key_code::q:
        return ImGuiKey_Q;
    case zabato::key_code::r:
        return ImGuiKey_R;
    case zabato::key_code::s:
        return ImGuiKey_S;
    case zabato::key_code::t:
        return ImGuiKey_T;
    case zabato::key_code::u:
        return ImGuiKey_U;
    case zabato::key_code::v:
        return ImGuiKey_V;
    case zabato::key_code::w:
        return ImGuiKey_W;
    case zabato::key_code::x:
        return ImGuiKey_X;
    case zabato::key_code::y:
        return ImGuiKey_Y;
    case zabato::key_code::z:
        return ImGuiKey_Z;
    case zabato::key_code::f1:
        return ImGuiKey_F1;
    case zabato::key_code::f2:
        return ImGuiKey_F2;
    case zabato::key_code::f3:
        return ImGuiKey_F3;
    case zabato::key_code::f4:
        return ImGuiKey_F4;
    case zabato::key_code::f5:
        return ImGuiKey_F5;
    case zabato::key_code::f6:
        return ImGuiKey_F6;
    case zabato::key_code::f7:
        return ImGuiKey_F7;
    case zabato::key_code::f8:
        return ImGuiKey_F8;
    case zabato::key_code::f9:
        return ImGuiKey_F9;
    case zabato::key_code::f10:
        return ImGuiKey_F10;
    case zabato::key_code::f11:
        return ImGuiKey_F11;
    case zabato::key_code::f12:
        return ImGuiKey_F12;
    case zabato::key_code::f13:
        return ImGuiKey_F13;
    case zabato::key_code::f14:
        return ImGuiKey_F14;
    case zabato::key_code::f15:
        return ImGuiKey_F15;
    case zabato::key_code::f16:
        return ImGuiKey_F16;
    case zabato::key_code::f17:
        return ImGuiKey_F17;
    case zabato::key_code::f18:
        return ImGuiKey_F18;
    case zabato::key_code::f19:
        return ImGuiKey_F19;
    case zabato::key_code::f20:
        return ImGuiKey_F20;
    case zabato::key_code::f21:
        return ImGuiKey_F21;
    case zabato::key_code::f22:
        return ImGuiKey_F22;
    case zabato::key_code::f23:
        return ImGuiKey_F23;
    case zabato::key_code::f24:
        return ImGuiKey_F24;
    default:
        return ImGuiKey_None;
    }
}

void key_cb(zabato::window *w,
            zabato::key_code key,
            int scancode,
            zabato::button_state action,
            zabato::modifier_keys mods)
{
    ImGuiIO &io = ImGui::GetIO();
    update_modifiers(mods);
    ImGuiKey imgui_key = key_to_imgui(key);
    io.AddKeyEvent(imgui_key,
                   action == zabato::button_state::press ||
                       action == zabato::button_state::repeat);
}

void mouse_button_cb(zabato::window *w,
                     zabato::mouse_button button,
                     zabato::button_state action,
                     zabato::modifier_keys mods)
{
    ImGuiIO &io = ImGui::GetIO();
    update_modifiers(mods);
    int imgui_button = -1;
    if (button == zabato::mouse_button::left)
        imgui_button = 0;
    else if (button == zabato::mouse_button::right)
        imgui_button = 1;
    else if (button == zabato::mouse_button::middle)
        imgui_button = 2;

    std::cout << "[ImGui] Mouse Button: " << (int)button
              << " Action: " << (int)action << " ImGuiBtn: " << imgui_button
              << " Pos: " << io.MousePos.x << "," << io.MousePos.y << std::endl;

    if (imgui_button != -1)
        io.AddMouseButtonEvent(imgui_button,
                               action == zabato::button_state::press);
}

void scroll_cb(zabato::window *w, zabato::real xoffset, zabato::real yoffset)
{
    ImGuiIO &io = ImGui::GetIO();
    io.AddMouseWheelEvent((float)xoffset, (float)yoffset);
}

void text_input_cb(zabato::window *w, const char *text)
{
    ImGuiIO &io = ImGui::GetIO();
    io.AddInputCharactersUTF8(text);
}

} // namespace

namespace zabato::imgui
{
void init(zabato::window *win)
{
    std::cout << "[ImGui] Init called with window: " << win << std::endl;
    g_window = win;
    g_gpu    = zabato::init_gpu();

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendPlatformName = "imgui_impl_zabato";

    g_window->add_key_callback(key_cb);
    g_window->add_mouse_button_callback(mouse_button_cb);
    g_window->add_scroll_callback(scroll_cb);
    g_window->add_text_input_callback(text_input_cb);

    g_time = zabato::get_time();
}

void shutdown()
{
    if (g_window)
    {
        g_window->remove_key_callback(key_cb);
        g_window->remove_mouse_button_callback(mouse_button_cb);
        g_window->remove_scroll_callback(scroll_cb);
        g_window->remove_text_input_callback(text_input_cb);
    }

    if (g_font_texture)
    {
        g_font_texture->destroy();
        g_font_texture = nullptr;
    }
    ImGui::DestroyContext();
    g_window = nullptr;
    g_gpu    = nullptr;
}

void new_frame()
{
    ImGuiIO &io = ImGui::GetIO();

    // Setup display size
    auto size      = g_window->get_size();
    auto fb_size   = g_window->get_framebuffer_size();
    io.DisplaySize = ImVec2(size.x, size.y);
    if (size.x > 0 && size.y > 0)
        io.DisplayFramebufferScale =
            ImVec2(fb_size.x / size.x, fb_size.y / size.y);

    // Setup time step
    uint64_t current_time = zabato::get_time();
    io.DeltaTime =
        (float)((real)(current_time - g_time) * (real(1.0f) / real(1000.0f)));

    if (io.DeltaTime <= 0.0f)
        io.DeltaTime = 0.00001f;
    g_time = current_time;

    // Update mouse pos
    auto mouse_pos = g_window->get_cursor_pos();
    io.AddMousePosEvent((float)mouse_pos.x, (float)mouse_pos.y);

    // Font texture
    if (!g_font_texture)
    {
        std::cout << "[ImGui] Creating font texture..." << std::endl;
        if (!g_gpu)
        {
            std::cerr << "[ImGui] Error: GPU is null during texture creation!"
                      << std::endl;
        }
        else
        {
            unsigned char *pixels;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

            vector<uint16_t> tex_data(width * height);
            for (int i = 0; i < width * height; ++i)
            {
                uint8_t r = pixels[i * 4 + 0];
                uint8_t g = pixels[i * 4 + 1];
                uint8_t b = pixels[i * 4 + 2];
                uint8_t a = pixels[i * 4 + 3];

                color c(color8888(r, g, b, a));
                tex_data[i] = zabato::color4444(c).value;
            }

            g_font_texture = g_gpu->create_texture(
                width, height, zabato::color_format::rgba4444);
            if (g_font_texture)
            {
                std::cout << "[ImGui] Texture created: " << g_font_texture
                          << std::endl;
                g_font_texture->load(width,
                                     height,
                                     zabato::color_format::rgba4444,
                                     tex_data.size() * sizeof(uint16_t),
                                     tex_data.data());
                io.Fonts->SetTexID((ImTextureID)g_font_texture);
            }
            else
            {
                std::cerr << "[ImGui] Error: Failed to create texture object!"
                          << std::endl;
            }
        }
    }

    ImGui::NewFrame();
}

void render_draw_data(ImDrawData *draw_data)
{
    if (!g_gpu)
        return;

    // Verify font texture creation delayed
    if (!g_font_texture)
    {
        ImGuiIO &io = ImGui::GetIO();
        unsigned char *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        vector<uint16_t> tex_data(width * height);
        for (int i = 0; i < width * height; ++i)
        {
            uint8_t r = pixels[i * 4 + 0];
            uint8_t g = pixels[i * 4 + 1];
            uint8_t b = pixels[i * 4 + 2];
            uint8_t a = pixels[i * 4 + 3];
            color c(color8888(r, g, b, a));

            tex_data[i] = zabato::color4444(c).value;
        }

        g_font_texture = g_gpu->create_texture(
            width, height, zabato::color_format::rgba4444);
        g_font_texture->load(width,
                             height,
                             zabato::color_format::rgba4444,
                             tex_data.size() * sizeof(uint16_t),
                             tex_data.data());
        io.Fonts->SetTexID((ImTextureID)g_font_texture);
    }

    int fb_width =
        (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height =
        (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    g_gpu->viewport(fb_width, fb_height);
    g_gpu->set_matrix_mode(zabato::matrix_mode::projection);
    g_gpu->push_matrix();
    g_gpu->load_identity();
    g_gpu->ortho(draw_data->DisplayPos.x,
                 draw_data->DisplayPos.x + draw_data->DisplaySize.x,
                 draw_data->DisplayPos.y + draw_data->DisplaySize.y,
                 draw_data->DisplayPos.y,
                 -1.0f,
                 +1.0f);

    g_gpu->set_matrix_mode(zabato::matrix_mode::modelview);
    g_gpu->push_matrix();
    g_gpu->load_identity();

    g_gpu->enable_lighting(false);
    g_gpu->enable_fog(false);
    g_gpu->set_shade_model(zabato::shade_model::smooth);

    g_gpu->enable_blend(true);
    g_gpu->set_blend_func(zabato::blend_factor::src_alpha,
                          zabato::blend_factor::one_minus_src_alpha);
    g_gpu->enable_depth_test(false);
    g_gpu->enable_lighting(false);
    g_gpu->enable_scissor_test(true);

    // Calculate Clipping Scale and Offset
    zabato::vec2<int> fb_size     = g_window->get_framebuffer_size();
    zabato::vec2<real> clip_scale = {(real)fb_size.x / draw_data->DisplaySize.x,
                                     (real)fb_size.y /
                                         draw_data->DisplaySize.y};
    zabato::vec2<real> clip_off   = {draw_data->DisplayPos.x,
                                     draw_data->DisplayPos.y};

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList *cmd_list   = draw_data->CmdLists[n];
        const ImDrawVert *vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx *idx_buffer  = cmd_list->IdxBuffer.Data;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Apply Clipping
                zabato::vec2<real> clip_min = {
                    ((real)pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                    ((real)pcmd->ClipRect.y - clip_off.y) * clip_scale.y};
                zabato::vec2<real> clip_max = {
                    ((real)pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                    ((real)pcmd->ClipRect.w - clip_off.y) * clip_scale.y};

                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Scissor Y is from bottom
                g_gpu->set_scissor((int)clip_min.x,
                                   (int)((real)fb_size.y - clip_max.y),
                                   (int)(clip_max.x - clip_min.x),
                                   (int)(clip_max.y - clip_min.y));

                // Bind texture
                zabato::texture *tex = (zabato::texture *)pcmd->GetTexID();
                if (tex)
                    g_gpu->bind_texture(tex);
                else
                    g_gpu->unbind_texture();

                g_gpu->begin(zabato::primitive_type::triangles);
                for (unsigned int i = 0; i < pcmd->ElemCount; i++)
                {
                    ImDrawIdx idx       = idx_buffer[pcmd->IdxOffset + i];
                    const ImDrawVert &v = vtx_buffer[idx];

                    real r = (real)((v.col >> IM_COL32_R_SHIFT) & 0xFF) *
                             (real(1.0f) / real(255.0f));
                    real g = (real)((v.col >> IM_COL32_G_SHIFT) & 0xFF) *
                             (real(1.0f) / real(255.0f));
                    real b = (real)((v.col >> IM_COL32_B_SHIFT) & 0xFF) *
                             (real(1.0f) / real(255.0f));
                    real a = (real)((v.col >> IM_COL32_A_SHIFT) & 0xFF) *
                             (real(1.0f) / real(255.0f));

                    g_gpu->color(color(r, g, b, a));
                    g_gpu->tex_coord(v.uv.x, v.uv.y);
                    g_gpu->vertex(v.pos.x, v.pos.y, 0.0f);
                }
                g_gpu->end();
            }
        }
    }

    g_gpu->enable_scissor_test(false);
    g_gpu->set_matrix_mode(zabato::matrix_mode::projection);
    g_gpu->pop_matrix();
    g_gpu->set_matrix_mode(zabato::matrix_mode::modelview);
    g_gpu->pop_matrix();
}

} // namespace zabato::imgui