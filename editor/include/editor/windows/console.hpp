#pragma once

#include <zabato/console.hpp>
#include <zabato/game_message.hpp>

namespace zabato::editor
{
class editor_app;

class console_window
{
public:
    console_window(console &console) : m_console(console) {}

    void render(editor_app &app);
    void on_message(const game_message &msg) {}

private:
    console &m_console;

    bool m_auto_scroll      = true;
    bool m_scroll_to_bottom = false;
};

} // namespace zabato::editor
