#pragma once

#include <zabato/hash_map.hpp>
#include <zabato/ice.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/stream.hpp>
#include <zabato/string.hpp>

namespace zabato
{
class resource
{
public:
    virtual ~resource() = default;
};

class resource_manager
{
public:
    using resource_ptr = shared_ptr<resource>;

    template <typename T> result<shared_ptr<T>> load(const string &path)
    {
        resource_ptr resource;
        if (m_resources.try_get_value(path, resource))
            return move(resource);

        auto obj = make_shared<T>();

        FILE *file = fopen(path.c_str(), "rb");
        assert(file);
        if (!file)
            return nullptr;

        file_stream stream(file);
        ice_reader reader(stream);
        auto res = deserialize(reader, obj.get());
        if (res.has_error())
        {
            fclose(file);
            return res;
        }

        fclose(file);
        m_resources.set(path, static_pointer_cast<resource>(res));
        return obj;
    }

    // Unloads a resource by path
    void unload(const string &path) { m_resources.erase(path); }

    // Unloads all resources
    void unload_all() { m_resources.clear(); }

private:
    hash_map<string, resource_ptr> m_resources;
};

class resource_ref
{
public:
    resource_ref() = default;
    resource_ref(string_view path, resource_manager *mgr = nullptr)
        : m_path(path), m_manager(mgr)
    {
    }

    string_view path() const { return m_path; }
    void set_path(string_view path) { m_path = path; }
    const char *c_path() const { return m_path.c_str(); }
    void set_manager(resource_manager *mgr) { m_manager = mgr; }
    resource_manager *manager() const { return m_manager; }

    template <typename T> shared_ptr<T> &&get() const
    {
        if (!m_manager || m_path.empty())
            return nullptr;
        result<shared_ptr<T>> resource = m_manager->load<T>(m_path);
        assert(!resource.has_error());
        if (resource.has_error())
            return nullptr;
        return move(resource.value);
    }

private:
    string m_path;
    resource_manager *m_manager = nullptr;
};

} // namespace zabato