#pragma once

#include <zabato/fs.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/ice.hpp>
#include <zabato/importer.hpp>
#include <zabato/rtti.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/stream.hpp>
#include <zabato/string.hpp>

namespace zabato
{
class resource
{
public:
    static const rtti TYPE;
    virtual const rtti &type() const { return TYPE; }

    /**
     * @brief Check if this object is exactly of the specified type.
     * @param t The type to check against.
     * @return true if the types match exactly, false otherwise.
     */
    bool is_exactly(const rtti &t) const { return type().is_exactly(t); }

    /**
     * @brief Check if this object is derived from the specified type.
     * @param t The base type to check against.
     * @return true if this object is derived from t, false otherwise.
     */
    bool is_derived(const rtti &t) const { return type().is_derived(t); }

    /**
     * @brief Check if this object is exactly the same type as another object.
     * @param obj The object to compare with.
     * @return true if both objects have exactly the same type.
     */
    bool is_exactly_typeof(const base_object *obj) const
    {
        return obj && is_exactly(obj->type());
    }

    /**
     * @brief Check if this object is of a type derived from the other object's
     * type.
     * @param obj The potential base object.
     * @return true if this object is derived from obj's type.
     */
    bool is_derived_typeof(const base_object *obj) const
    {
        return obj && is_derived(obj->type());
    }

    virtual ~resource() = default;
};

class resource_manager
{
public:
    using resource_ptr = shared_ptr<resource>;

    void set_file_system(fs::file_system *fs) { m_fs = fs; }
    fs::file_system *get_file_system() const { return m_fs; }

    result<shared_ptr<resource>> import_typed_resource(const string &path,
                                                       const rtti &type);

    result<shared_ptr<resource>> import_resource(const string &path);

    template <typename T> bool is_resource_type(const string &path)
    {
        string xml_path                        = path;
        size_t last_dot                        = xml_path.rfind('.');
        shared_ptr<importer> specific_importer = nullptr;

        if (last_dot != string::npos)
        {
            string ext    = path.substr(last_dot);
            auto importer = importer_registry::find_importer(ext);
            if (importer)
                return importer->is_resource_type(T::TYPE);
        }
        return false;
    }

    template <typename T> result<shared_ptr<T>> load(const string &path)
    {
        auto import_res = import_typed_resource(path, T::TYPE);
        if (!import_res.has_error())
            return static_pointer_cast<T>(import_res.value);

        if (import_res.error != error_code::unable_to_match)
            return import_res.error;

        if constexpr (!std::is_same_v<T, resource> && !std::is_abstract_v<T>)
        {
            resource_ptr resource;
            if (m_resources.try_get_value(path, resource))
                return static_pointer_cast<T>(resource);

            if (!m_fs)
                return report_error(error_code::value,
                                    "File system not set in resource_manager");

            auto obj = make_shared<T>();

            fs::file *file = m_fs->open(path, fs::open_mode::read);
            if (!file)
                return report_error(error_code::file_not_found, path.c_str());

            ice_reader reader(*file);
            auto res = deserialize(reader, *obj.get());

            file->close();
            delete file;

            if (res.has_error())
                return res.error;

            m_resources.add_or_set(path, obj);
            return obj;
        }
        else
        {
            return report_error(error_code::unable_to_match,
                                "No importer found and cannot natively load "
                                "generic resource");
        }
    }

    template <typename T>
    void add_resource(const string &path, shared_ptr<T> res)
    {
        m_resources.add(path, static_pointer_cast<resource>(res));
    }

    // Unloads a resource by path
    void unload(const string &path) { m_resources.erase(path); }

    // Unloads all resources
    void unload_all() { m_resources.clear(); }

    void set_gpu(class gpu *gpu) { m_gpu = gpu; }
    class gpu *get_gpu() const { return m_gpu; }

    resource_manager()  = default;
    ~resource_manager() = default;

    // Prevent accidental copying
    resource_manager(const resource_manager &)            = delete;
    resource_manager &operator=(const resource_manager &) = delete;

private:
    hash_map<string, resource_ptr> m_resources;
    fs::file_system *m_fs = nullptr;
    class gpu *m_gpu      = nullptr;
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

    template <typename T> shared_ptr<T> get() const
    {
        if (!m_manager || m_path.empty())
            return nullptr;

        result<shared_ptr<T>> resource = m_manager->load<T>(m_path);
        if (resource.has_error())
            return nullptr;
        return resource.value;
    }

    bool operator==(const resource_ref &other) const
    {
        return m_path == other.m_path;
    }

    bool operator!=(const resource_ref &other) const
    {
        return !(*this == other);
    }

private:
    string m_path;
    resource_manager *m_manager = nullptr;
};

} // namespace zabato