#pragma once

#include <tinyxml2.h>

#include <zabato/base_object.hpp>
#include <zabato/delegate.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/pointer.hpp>
#include <zabato/rtti.hpp>
#include <zabato/uuid.hpp>
#include <zabato/value.hpp>
#include <zabato/vector.hpp>

namespace zabato
{
class xml_serializer;
class serializer;
class serializer_link;
class string_tree;
class world;
class controller;
struct symbol;
class resource_manager;

template <class T> class pointer;

class object : public base_object
{
public:
    object();
    virtual ~object();

#pragma region Type
    static const rtti TYPE;

    /**
     * @brief Get the Run-Time Type Information (RTTI) for this object.
     * @return The RTTI structure describing this object's type.
     */
    virtual const rtti &type() const override { return TYPE; }

    static void reflect(reflection &r);
#pragma endregion Type

#pragma region Name
    /**
     * @brief Set the name of the object.
     * @param name The new name.
     */
    void set_name(const char *name);

    /**
     * @brief Set the name of the object.
     * @param name The new name.
     */
    void set_name(string_view name);

    /**
     * @brief Set the name of the object using a symbol.
     * @param name The symbol to set.
     */
    void set_name(symbol *name);

    /**
     * @brief Get the name of the object.
     * @return The object's name.
     */
    const char *name() const;

    /**
     * @brief search for an object with a specific name within this object's
     * hierarchy.
     * @param name The name to search for.
     * @return Pointer to the object if found, nullptr otherwise.
     */
    virtual object *get_object_by_name(const char *name);

    /**
     * @brief search for an object with a specific symbol name.
     * @param name The symbol to search for.
     * @return Pointer to the object if found, nullptr otherwise.
     */
    virtual object *get_object_by_name(const symbol_ref &name);

    /**
     * @brief Collect all objects with a specific name.
     * @param name The name to search for.
     * @param objects Vector to populate with found objects.
     */
    virtual void get_all_objects_by_name(const char *name,
                                         vector<object *> &objects);

    /**
     * @brief Collect all objects with a specific symbol name.
     * @param name The symbol to search for.
     * @param objects Vector to populate with found objects.
     */
    virtual void get_all_objects_by_name(const symbol_ref &name,
                                         vector<object *> &objects);
#pragma endregion Name

#pragma region ID
    /**
     * @brief Get the unique ID of this object.
     * @return The object ID.
     */
    uuid id() const { return m_uiID; }

    /**
     * @brief Search for an object by its unique ID.
     * @param uiID The ID to search for.
     * @return Pointer to the object if found, nullptr otherwise.
     */
    virtual object *get_object_by_id(uuid uiID)
    {
        if (m_uiID == uiID)
            return this;
        return nullptr;
    }

    /**
     * @brief Set the unique ID of this object (updating registry).
     * @param id The new ID.
     */
    void set_id(const uuid &id);

    /**
     * @brief Create a default instance of an object by type name.
     * @param type_name The registered factory name.
     * @param mgr Resource manager required for serializer context.
     * @return Pointer to the new object, or nullptr.
     */
    static object *create_default(const string &type_name,
                                  resource_manager &mgr);

#pragma endregion ID

#pragma region Streaming
    using factory_delegate = delegate<object *()>;

    enum
    {
        FACTORY_MAP_SIZE = 256
    };

    struct factory_info
    {
        factory_delegate factory;
        const rtti *type = nullptr;
    };

    static hash_map<string, factory_info> *s_factory;

    /**
     * @brief Register the factory and type for a class.
     * @tparam T The class to register.
     * @return true on success.
     */
    template <typename T> static bool register_type()
    {
        factory_delegate f;

        f = []() -> object * { return new T(); };

        return register_factory_type(T::TYPE.name(), &T::TYPE, f);
    }

    static bool register_factory_type(const string &name,
                                      const rtti *type,
                                      factory_delegate f);
    static const rtti *get_factory_type(const string &name);

    /**
     * @brief Register the factory for this class.
     * @return true on success.
     */
    static bool register_factory();

    /**
     * @brief Initialize the global factory map.
     */
    static void initialize_factory();

    /**
     * @brief Terminate and clean up the global factory map.
     */
    static void terminate_factory();

    /**
     * @brief Factory method to create an object from a serialized stream.
     *
     * This method reads the RTTI type name from the stream, looks up the
     * corresponding factory function in the global registry, and invokes it to
     * instantiate and load the object.
     *
     * @param stream The serializer stream containing the object data.
     * @return A pointer to the created object, or nullptr if the factory is not
     * initialized or the type is unknown.
     */
    static object *factory(string_view name);

    /**
     * @brief Load object data from a stream.
     * @param stream The stream to read from.
     * @param link The link context for resolving object references.
     */
    virtual void load(serializer &stream, serializer_link *link);

    /**
     * @brief Resolve links to other objects after loading.
     * @param stream The stream context.
     * @param link The link context containing object IDs.
     */
    virtual void link(serializer &stream, serializer_link *link);

    /**
     * @brief Resolve links to other objects after loading.
     * @param serializer The serializer context.
     * @param el The XML element containing object IDs.
     */
    virtual void link(xml_serializer &serializer, tinyxml2::XMLElement &el);

    /**
     * @brief Register this object for streaming.
     * @param stream The stream to register with.
     * @return true if registered successfully.
     */
    virtual bool register_object(serializer &stream) const;

    /**
     * @brief Save this object to a stream.
     * @param stream The stream to write to.
     */
    virtual void save(serializer &stream) const;

    /**
     * @brief Save this object to an XML element.
     * @param element The XML element to write to.
     */
    virtual void save_xml(xml_serializer &serializer,
                          tinyxml2::XMLElement &element) const;

    /**
     * @brief Load this object from an XML element.
     * @param element The XML element to read from.
     */
    virtual void load_xml(xml_serializer &serializer,
                          tinyxml2::XMLElement &element);

    /**
     * @brief Get the memory usage of this object.
     * @return Memory used in bytes.
     */
    virtual int get_memory_used() const;

    /**
     * @brief Get the disk space usage of this object.
     * @return Disk space used in bytes.
     */
    virtual int get_disk_used() const;

    /**
     * @brief Save the object's representation to a string tree.
     * @param tree The string tree to append data to.
     */
    virtual void save_strings(string_tree *tree);
#pragma endregion Streaming

#pragma region Reference Count
    static hash_map<uuid, object *> s_in_use;
    static void print_in_use(const char *file, const char *acMessage);
#pragma endregion Reference Count

#pragma region Cloning
    object *clone(resource_manager &manager) const;
#pragma endregion Cloning

#pragma region World
    virtual world *get_world() const { return nullptr; }
#pragma endregion World

#pragma region Controllers
    /**
     * @brief Add a controller to this object.
     * @param ctrl The controller to add.
     */
    void add_controller(pointer<controller> ctrl);

    /**
     * @brief Remove a controller from this object.
     * @param ctrl The controller to remove.
     */
    void remove_controller(pointer<controller> ctrl);

    /**
     * @brief Get the first controller of a specific type.
     * @param type The RTTI type to search for.
     * @return Pointer to the controller, or nullptr.
     */
    pointer<controller> get_controller(const rtti &type) const;

    /**
     * @brief Get the first controller of a specific type (template version).
     * @tparam T The type of controller.
     * @return Pointer to the controller, or nullptr.
     */
    template <typename T> pointer<T> get_controller() const
    {
        return get_controller(T::TYPE);
    }

    /**
     * @brief Get all controllers of a specific type.
     * @param type The RTTI type to search for.
     * @param out_controllers Vector to populate.
     */
    void get_controllers(const rtti &type,
                         vector<pointer<controller>> &out_controllers) const;

    /**
     * @brief Get all controllers attached to this object.
     * @return The list of controllers.
     */
    const vector<pointer<controller>> &get_controllers() const
    {
        return m_controllers;
    }
#pragma endregion Controllers

private:
    symbol_ref m_name;
    uuid m_uiID;

    vector<pointer<controller>> m_controllers;
};

/**
 * @brief Safely casts an object to a derived type using the custom RTTI system.
 *
 * Checks if the object is an instance of the target type T (or a subclass)
 * using is_derived(T::TYPE). If the check is successful, returns the cast
 * pointer.
 *
 * @tparam T The target type to cast to. Must be a subclass of `object` with
 * valid RTTI type.
 * @param obj The object to cast. Can be nullptr.
 * @return A pointer to the object cast to T*, or nullptr if the cast fails or
 * the input is null.
 */
template <class T, class U> T *c_dynamic_cast(U *obj)
{
    return obj && obj->is_derived(T::TYPE) ? (T *)obj : nullptr;
}

/**
 * @brief Safely casts a const object to a derived const type using the custom
 * RTTI system.
 *
 * See c_dynamic_cast(object*) for details.
 *
 * @tparam T The target type to cast to.
 * @param obj The const object to cast.
 * @return A const pointer to the object cast to T*, or nullptr if the cast
 * fails or the input is null.
 */
template <class T, class U> const T *c_dynamic_cast(const U *obj)
{
    return obj && obj->is_derived(T::TYPE) ? (const T *)obj : nullptr;
}

} // namespace zabato