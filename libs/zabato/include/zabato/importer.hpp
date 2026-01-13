#pragma once

#include <zabato/error.hpp>
#include <zabato/fs.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/string.hpp>
#include <zabato/vector.hpp>

#include <tinyxml2.h>

namespace zabato
{

class resource;
class rtti;

/**
 * @class importer
 * @brief Abstract interface for asset importers.
 */
class importer
{
public:
    virtual ~importer() = default;

    /**
     * @brief Returns the unique name of this importer.
     */
    virtual string_view name() const = 0;

    /**
     * @brief Checks if this importer supports the given file extension.
     * @param extension The file extension (including dot, e.g., ".fbx").
     * @return True if supported, false otherwise.
     */
    virtual bool supports(const string &extension) const = 0;

    /**
     * @brief Imports a resource from the specified path.
     * @param fs The file system to read from.
     * @param gpu_ctx The GPU context (optional, may be null).
     * @param path The absolute or relative path to the source file.
     * @param settings Optional XML configuration element for this import.
     * @return A shared_ptr to the imported resource, or an error.
     */
    virtual result<shared_ptr<resource>>
    import(fs::file_system &fs,
           class gpu *gpu_ctx,
           const string &path,
           const tinyxml2::XMLElement *settings = nullptr) = 0;

    /**
     * @brief Checks if this importer produces a resource of the specified type.
     * @param type The RTTI of the resource class.
     * @return True if the importer can import this resource type.
     */
    virtual bool is_resource_type(const zabato::rtti &type) const = 0;
};

/**
 * @class importer_registry
 * @brief Global registry for available importers.
 */
class importer_registry
{
public:
    /**
     * @brief Registers an importer instance.
     * @param imp The importer to register.
     */
    static void register_importer(shared_ptr<importer> imp);

    /**
     * @brief Finds a suitable importer for the given extension.
     * @param extension The file extension to look up.
     * @return The first matching importer, or nullptr if none found.
     */
    static shared_ptr<importer> find_importer(const string &extension);

    /**
     * @brief Finds an importer by its unique name.
     * @param name The unique name of the importer.
     * @return The importer, or nullptr.
     */
    static shared_ptr<importer> find_importer_by_name(const string &name);

    /**
     * @brief Finds all suitable importers for the given extension.
     * @param extension The file extension to look up.
     * @return A vector of all matching importers.
     */
    static vector<shared_ptr<importer>> find_importers(const string &extension);

private:
    static vector<shared_ptr<importer>> s_importers;
};

} // namespace zabato
