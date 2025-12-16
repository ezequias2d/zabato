#pragma once

#include <zabato/hash_map.hpp>
#include <zabato/hash_set.hpp>
#include <zabato/ice.hpp>
#include <zabato/object.hpp>
#include <zabato/stream.hpp>

namespace zabato
{

/**
 * @brief Helper class for resolving object connections during serialization
 * loading.
 *
 * When objects are loaded, their children or dependencies might not be fully
 * loaded yet. This class acts as a link context, collecting IDs of child
 * objects so they can be resolved to actual pointers later in the `link()`
 * phase.
 */
class serializer_link : public object
{
public:
    static const rtti TYPE;

    virtual const rtti &type() const { return TYPE; }

    /**
     * @brief Construct a new serializer link.
     * @param obj The parent object that owns this link context.
     */
    serializer_link(object *obj) : m_obj(obj) {}
    ~serializer_link() {}

    /**
     * @brief Register a child object ID to be linked later.
     * @param id The pointer/ID of the child object (usually an address from the
     * stream).
     */
    void add_child_id(object *id) { m_children_ids.push_back(id); }

    /**
     * @brief Retrieve the next child ID during linking.
     * @return The ID of the next child, or nullptr if no more children.
     */
    object *get_next_child_id()
    {
        if (m_current_child < m_children_ids.size())
            return m_children_ids[m_current_child++];
        return nullptr;
    }

    /**
     * @brief Get the object associated with this link.
     * @return Pointer to the object.
     */
    object *get_object() { return m_obj; }

private:
    object *m_obj;
    vector<object *> m_children_ids;
    size_t m_current_child = 0;
};

/**
 * @brief Handles object serialization and deserialization.
 *
 * specialized methods for reading/writing basic types and objects.
 * It manages the mapping of object pointers to unique IDs to handle
 * shared references and cycles during serialization.
 */
class serializer
{
public:
    serializer();
    ~serializer();

    /**
     * @brief Register an object in the unique map during save.
     * @param obj The object pointer.
     * @param value Associated value (can be nullptr).
     * @return true if inserted (new object), false if already exists.
     */
    bool insert_in_map(const object *obj, void *value);

    /**
     * @brief Register an object to be saved in collected order.
     * @param obj The object pointer.
     */
    void insert_in_ordered(const object *obj);

    /**
     * @brief Register a loaded object with its original ID.
     * @param unique_id The ID read from the stream.
     * @param obj_or_link The loaded object or its link context.
     * @return true on success.
     */
    bool insert_in_map(void *unique_id, object *obj_or_link);

    /**
     * @brief Retrieve a loaded object by its collected ID.
     * @param unique_id The ID searched.
     * @return Pointer to the object, or nullptr.
     */
    object *get_from_map(void *unique_id);

    /**
     * @brief Read data of type T from the stream.
     * @tparam T Type of data.
     * @param data Reference to store read data.
     * @return Number of bytes read.
     */
    template <typename T> size_t read(T &data)
    {
        return m_reader ? m_reader->read(data) : 0;
    }

    /**
     * @brief Write data of type T to the stream.
     * @tparam T Type of data.
     * @param data Data to write.
     * @return Number of bytes written.
     */
    template <typename T> size_t write(const T &data)
    {
        if (m_writer)
        {
            auto res = m_writer->write(&data, sizeof(T));
            if (!res.has_error())
                return *res;
        }
        return 0;
    }

    /**
     * @brief Read an object pointer.
     * Handles resolving existing objects or marking for load.
     * @param obj Reference to update.
     * @return bytes read.
     */
    size_t read(object *&obj);

    /**
     * @brief Write an object pointer.
     * Takes care of pointer-ID mapping.
     * @param obj The object to write.
     * @return bytes written.
     */
    size_t write(const object *obj);

    /**
     * @brief Read a string.
     * @param str Reference to string.
     * @return bytes read.
     */
    size_t read(string &str);

    /**
     * @brief Write a string.
     * @param str The string to write.
     * @return bytes written.
     */
    size_t write(const string &str);

    /**
     * @brief Save an object hierarchy starting from root.
     * @param stream The output stream.
     * @param root The root object.
     * @return true on success.
     */
    bool save(stream &stream, const object *root);

    /**
     * @brief Load objects from a stream.
     * @param stream The input stream.
     * @return true on success.
     */
    bool load(stream &stream);

private:
    hash_map<const void *, void *> m_unique_map;
    vector<const object *> m_ordered_objects;
    vector<serializer_link *> m_links;

    ice_reader *m_reader = nullptr;
    ice_writer *m_writer = nullptr;
};

} // namespace zabato