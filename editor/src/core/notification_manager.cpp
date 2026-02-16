#include <cstdio>
#include <editor/notification_manager.hpp>
#include <zabato/imgui.hpp>

namespace zabato::editor
{

notification_manager::notification_manager(console &console)
    : m_console(console)
{
    console.add_callback(
        delegate<void(log_level, const string_view &)>::
            from_method<notification_manager, &notification_manager::on_log>(
                this));
}

void notification_manager::on_log(log_level level, const string_view &msg)
{
    notification n;
    n.level        = level;
    n.message      = msg;
    n.time_to_live = 5.0f;
    n.initial_ttl  = 5.0f;
    m_notifications.push_back(n);

    const char *type_name;
    switch (level)
    {
    case log_level::info:
        type_name = "Info";
        break;
    case log_level::warning:
        type_name = "Warning";
        break;
    case log_level::error:
        type_name = "Error";
        break;
    case log_level::debug:
        type_name = "Debug";
        break;
    case log_level::success:
        type_name = "Success";
        break;
    default:
        type_name = "Unknown";
        break;
    }

    printf("Notification added: %s - %.*s\n",
           type_name,
           (int)msg.length(),
           msg.data());
}

void notification_manager::update(real delta_time)
{
    // Remove expired
    for (int i = (int)m_notifications.size() - 1; i >= 0; i--)
    {
        m_notifications[i].time_to_live -= delta_time;
        if (m_notifications[i].time_to_live <= 0)
        {
            m_notifications.remove_at(i);
            break;
        }
    }
}

void notification_manager::render()
{
    if (m_notifications.empty())
        return;

    ImGuiIO &io          = ImGui::GetIO();
    float window_padding = 10.0f;
    ImVec2 work_pos      = ImGui::GetMainViewport()->WorkPos;
    ImVec2 work_size     = ImGui::GetMainViewport()->WorkSize;
    ImVec2 render_pos;
    render_pos.x = work_pos.x + work_size.x - window_padding;
    render_pos.y = work_pos.y + work_size.y - window_padding;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));

    ImGui::SetNextWindowPos(render_pos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.9f);

    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(300, 1000));

    if (ImGui::Begin("##Notifications",
                     nullptr,
                     ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove))
    {
        for (const auto &n : m_notifications)
        {
            ImVec4 color;
            switch (n.level)
            {
            case log_level::error:
                color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                break;
            case log_level::warning:
                color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
                break;
            case log_level::success:
                color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
                break;
            case log_level::debug:
                color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                break;
            case log_level::info:
                color = ImVec4(0.3f, 1.0f, 1.0f, 1.0f);
                break;
            default:
                color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                break;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextWrapped("%s", n.message.c_str());
            ImGui::PopStyleColor();
            ImGui::Separator();
        }
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
} // namespace zabato::editor
