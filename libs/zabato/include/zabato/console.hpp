#pragma once

#include <stdio.h>
#include <time.h>
#include <zabato/delegate.hpp>
#include <zabato/spin_lock.hpp>
#include <zabato/string.hpp>
#include <zabato/vector.hpp>

namespace zabato
{

enum class log_level
{
    info,
    warning,
    error,
    debug,
    success,
};

struct log_entry
{
    log_level level;
    string message;
    time_t timestamp;
};

typedef delegate<void(log_level level, const string_view &msg)> log_delegate;

class console
{
public:
    console() {}
    ~console() = default;

    void log(log_level level, const string_view &msg);
    void add_callback(log_delegate cb) { m_callbacks.push_back(cb); }
    void remove_callback(log_delegate cb) { m_callbacks.remove(cb); }

    void log_info(const string_view &msg) { log(log_level::info, msg); }
    void log_warning(const string_view &msg) { log(log_level::warning, msg); }
    void log_error(const string_view &msg) { log(log_level::error, msg); }
    void log_debug(const string_view &msg) { log(log_level::debug, msg); }
    void log_success(const string_view &msg) { log(log_level::success, msg); }

    void clear();

    const vector<log_entry> &entries() const { return m_history; }

    spin_lock &lock() { return m_lock; }

private:
    vector<log_delegate> m_callbacks;
    vector<log_entry> m_history;

    spin_lock m_lock;
    const size_t MAX_LOGS = 1024;
};
} // namespace zabato