#include <zabato/console.hpp>

namespace zabato
{
void console::log(log_level level, const string_view &msg)
{
    time_t now = time(nullptr);

    m_lock.lock();

    m_history.push_back({level, string(msg), now});
    if (m_history.size() > MAX_LOGS)
        m_history.erase(m_history.begin());

    m_lock.unlock();

    for (auto &callback : m_callbacks)
        callback(level, msg);
}

void console::clear()
{
    m_lock.lock();
    m_history.clear();
    m_lock.unlock();
}
} // namespace zabato