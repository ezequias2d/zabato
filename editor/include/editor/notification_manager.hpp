#pragma once

#include <zabato/console.hpp>
#include <zabato/real.hpp>
#include <zabato/string.hpp>
#include <zabato/vector.hpp>

namespace zabato::editor
{

struct notification
{
    log_level level;
    string message;
    real time_to_live; // in seconds
    real initial_ttl;
};

class notification_manager
{
public:
    notification_manager(console &console);

    void update(real delta_time);
    void render();

private:
    void on_log(log_level level, const string_view &msg);
    console &m_console;
    vector<notification> m_notifications;
};

} // namespace zabato::editor
