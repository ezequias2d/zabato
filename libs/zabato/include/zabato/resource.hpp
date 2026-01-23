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

    virtual ~resource() = default;
};

class resource_manager
{
public:
    using resource_ptr = shared_ptr<resource>;

    void set_file_system(fs::file_system *fs) { m_fs = fs; }
    fs::file_system *get_file_system() const { return m_fs; }

    result<shared_ptr<resource>> import_resource(const string &path)
    {
        resource_ptr resource;
        if (m_resources.try_get_value(path, resource))
            return resource;

        if (!m_fs)
        {
            return report_error(error_code::value,
                                "File system not set in resource_manager");
        }

        // Check for sidecar XML configuration
        tinyxml2::XMLDocument doc;
        const tinyxml2::XMLElement *settings   = nullptr;
        shared_ptr<importer> specific_importer = nullptr;

        string xml_path = path;
        size_t last_dot = xml_path.rfind('.');
        if (last_dot != string::npos)
        {
            xml_path = xml_path + ".xml";

            if (m_fs->exists(xml_path))
            {
                auto file_xml = m_fs->open(xml_path, fs::open_mode::read);
                if (file_xml)
                {
                    // Read entire XML into string
                    file_xml->seek(0, fs::origin::end);
                    size_t len = file_xml->tell();
                    file_xml->seek(0, fs::origin::begin);

                    vector<char> buf(len + 1);
                    buffer b(reinterpret_cast<uint8_t *>(buf.data()), len);
                    file_xml->read(b);
                    buf[len] = 0;
                    file_xml->close();
                    delete file_xml;

                    if (doc.Parse(buf.data()) == tinyxml2::XML_SUCCESS)
                    {
                        auto root = doc.FirstChildElement("import");
                        if (root)
                        {
                            auto importer_elem =
                                root->FirstChildElement("importer");
                            if (importer_elem && importer_elem->GetText())
                            {
                                specific_importer =
                                    importer_registry::find_importer_by_name(
                                        importer_elem->GetText());
                            }
                            settings = root->FirstChildElement("settings");
                        }
                    }
                }
            }
        }

        auto importer = specific_importer;
        if (!importer)
        {
            if (last_dot != string::npos)
            {
                string ext = path.substr(last_dot);
                importer   = importer_registry::find_importer(ext);
            }
        }

        if (importer)
        {
            auto res = importer->import(*m_fs, m_gpu, path, settings);
            if (!res.has_error())
            {
                m_resources.set(path, res.value);
                return res.value;
            }
            return res.error;
        }

        return report_error(error_code::unable_to_match,
                            "No importer found for file");
    }

    template <typename T> bool is_resource_type(const string &path)
    {
        // Find importer
        string xml_path                        = path;
        size_t last_dot                        = xml_path.rfind('.');
        shared_ptr<importer> specific_importer = nullptr;

        // Simplified check: usually we just look up extension or specific
        // importer from XML Replicating logic partially for importer lookup

        // ... (Skipping full XML sidecar check for performance in drag loop, or
        // duplicating it?) The user dragging a file likely relies on extension
        // unless registered.

        if (last_dot != string::npos)
        {
            string ext    = path.substr(last_dot);
            auto importer = importer_registry::find_importer(ext);
            if (importer)
            {
                return importer->is_resource_type(T::TYPE);
            }
        }
        return false;
    }

    template <typename T> result<shared_ptr<T>> load(const string &path)
    {
        // 1. Try to import using registered importers
        auto import_res = import_resource(path);
        if (!import_res.has_error())
        {
            return static_pointer_cast<T>(import_res.value);
        }
        else if (import_res.error != error_code::unable_to_match)
        {
            // If importer matched but failed, propagate error
            return import_res.error;
        }

        // 2. If no importer found, try native load (only if T is not resource
        // and not abstract)
        if constexpr (!std::is_same_v<T, resource> && !std::is_abstract_v<T>)
        {
            resource_ptr resource;
            if (m_resources.try_get_value(path, resource))
            {
                return static_pointer_cast<T>(resource);
            }

            if (!m_fs)
            {
                return report_error(error_code::value,
                                    "File system not set in resource_manager");
            }

            auto obj = make_shared<T>();

            fs::file *file = m_fs->open(path, fs::open_mode::read);
            if (!file)
                return report_error(error_code::file_not_found, path.c_str());

            ice_reader reader(*file);
            auto res = deserialize(reader, *obj.get());

            file->close();
            delete file;

            if (res.has_error())
            {
                return res.error;
            }

            m_resources.set(path, obj);
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
        assert(!resource.has_error());
        if (resource.has_error())
            return nullptr;
        return resource.value;
    }

private:
    string m_path;
    resource_manager *m_manager = nullptr;
};

} // namespace zabato