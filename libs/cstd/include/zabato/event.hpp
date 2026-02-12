#pragma once

#include <zabato/delegate.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/vector.hpp>

namespace zabato
{

template <typename... Args> class event
{
public:
    using callback_type = delegate<void(Args...)>;
    using id_type       = size_t;

    struct connection_state
    {
        event *evt     = nullptr;
        id_type id     = 0;
        bool connected = false;

        connection_state(event *e, id_type i) : evt(e), id(i), connected(true)
        {
        }
    };

    class scoped_connection
    {
    public:
        scoped_connection() = default;
        scoped_connection(shared_ptr<connection_state> state) : m_state(state)
        {
        }

        ~scoped_connection() { disconnect(); }

        scoped_connection(const scoped_connection &)            = delete;
        scoped_connection &operator=(const scoped_connection &) = delete;

        scoped_connection(scoped_connection &&other) noexcept
            : m_state(zabato::move(other.m_state))
        {
        }

        scoped_connection &operator=(scoped_connection &&other) noexcept
        {
            if (this != &other)
            {
                disconnect();
                m_state = zabato::move(other.m_state);
            }
            return *this;
        }

        void disconnect()
        {
            if (m_state && m_state->connected && m_state->evt)
            {
                m_state->evt->disconnect(m_state->id);
                m_state->connected = false;
                m_state            = nullptr;
            }
        }

        bool connected() const { return m_state && m_state->connected; }

    private:
        shared_ptr<connection_state> m_state;
    };

    scoped_connection connect(callback_type callback)
    {
        id_type id = ++m_next_id;
        m_callbacks.push_back({id, callback});
        return scoped_connection(make_shared<connection_state>(this, id));
    }

    void disconnect(id_type id)
    {
        for (size_t i = 0; i < m_callbacks.size(); ++i)
        {
            if (m_callbacks[i].id == id)
            {
                m_callbacks.erase(m_callbacks.begin() + i);
                return;
            }
        }
    }

    void invoke(Args... args) const
    {
        for (const auto &item : m_callbacks)
        {
            if (item.callback)
                item.callback(args...);
        }
    }

    void operator()(Args... args) const { invoke(args...); }

    void clear() { m_callbacks.clear(); }

private:
    struct callback_entry
    {
        id_type id;
        callback_type callback;
    };

    vector<callback_entry> m_callbacks;
    id_type m_next_id = 0;
};

} // namespace zabato
