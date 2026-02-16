#include <editor/editor.hpp>
#include <editor/windows/console.hpp>
#include <zabato/imgui.hpp>

namespace zabato::editor
{

void console_window::render(editor_app &app)
{
    if (ImGui::Begin("Console"))
    {
        if (ImGui::Button("Clear"))
        {
            m_console.clear();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &m_auto_scroll);

        ImGui::Separator();

        ImGui::BeginChild("ScrollingRegion",
                          ImVec2(0, 0),
                          false,
                          ImGuiWindowFlags_HorizontalScrollbar);

        const auto &history = m_console.entries();
        for (const auto &item : history)
        {
            ImVec4 color;
            switch (item.level)
            {
            case log_level::error:
                color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                break;
            case log_level::warning:
                color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
                break;
            case log_level::info:
                color = ImVec4(0.4f, 0.4f, 1.0f, 1.0f);
                break;
            case log_level::success:
                color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
                break;
            case log_level::debug:
                color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                break;
            default:
                color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                break;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(item.message.c_str());
            ImGui::PopStyleColor();
        }

        if (m_scroll_to_bottom ||
            (m_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f);
        m_scroll_to_bottom = false;

        ImGui::EndChild();
    }
    ImGui::End();
}

} // namespace zabato::editor
