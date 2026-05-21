#include <editor/windows/viewport.hpp>
#include <zabato/imgui.hpp>

namespace zabato::editor
{
viewport_window::viewport_window(const char *title) : m_title(title) {}

viewport_window::~viewport_window()
{
    if (m_fbo)
    {
        m_fbo->destroy();
        delete m_fbo;
        m_fbo = nullptr;
    }
}

void viewport_window::init() {}

void viewport_window::render(world &world,
                             renderer &renderer,
                             pointer<camera> cam,
                             gpu &gpu)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(m_title);

    m_is_focused = ImGui::IsWindowFocused();
    m_is_hovered = ImGui::IsWindowHovered();

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    int width                = (int)viewportPanelSize.x;
    int height               = (int)viewportPanelSize.y;

    if (width <= 0)
        width = 1;
    if (height <= 0)
        height = 1;

    // Resize FBO if needed
    if (!m_fbo || m_width != width || m_height != height)
    {
        if (m_fbo)
        {
            m_fbo->resize(width, height);
        }
        else
        {
            m_fbo = gpu.create_framebuffer(width, height);
        }
        m_width  = width;
        m_height = height;

        // Update camera aspect
        cam->set_perspective(to_rad(real(45)),
                             real(width) / real(height),
                             real(0.1),
                             real(100.0));
    }

    // Render to FBO
    if (m_fbo)
    {
        gpu.push_state();
        gpu.bind_framebuffer(m_fbo);
        gpu.viewport(width, height);
        gpu.enable_depth_test(true);
        gpu.enable_blend(true);
        gpu.bind_texture(nullptr);
        gpu.clear({0.1, 0.1, 0.1, 1.0}, 1.0);

        if (cam)
        {
            cam->update_view_from_transform();
            renderer.begin(cam);
            world.render(renderer, cam);

            renderer.end();

            if (m_on_scene_render)
                m_on_scene_render(world, *cam, gpu);
        }

        gpu.unbind_framebuffer();
        gpu.pop_state();

        // Draw Image
        ImGui::Image((void *)m_fbo->get_texture(),
                     ImVec2((float)width, (float)height),
                     ImVec2(0, 1),
                     ImVec2(1, 0));

        if (!cam)
        {
            ImVec2 rectMin   = ImGui::GetItemRectMin();
            ImVec2 rectMax   = ImGui::GetItemRectMax();
            ImVec2 center    = ImVec2((rectMin.x + rectMax.x) * 0.5f,
                                   (rectMin.y + rectMax.y) * 0.5f);
            const char *text = "No Camera";
            ImVec2 textSize  = ImGui::CalcTextSize(text);
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(center.x - textSize.x * 0.5f,
                       center.y - textSize.y * 0.5f),
                IM_COL32(255, 255, 255, 255),
                text);
        }
        m_is_image_hovered = ImGui::IsItemHovered();

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload =
                    ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                if (m_on_drop)
                    m_on_drop((const char *)payload->Data);
            }
            ImGui::EndDragDropTarget();
        }

        if (m_on_overlay_render && cam)
            m_on_overlay_render(world, *cam);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace zabato::editor
