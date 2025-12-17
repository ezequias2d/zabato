#include <zabato/ice.hpp>
#include <zabato/object.hpp>
#include <zabato/serializer.hpp>
#include <zabato/stream.hpp>

namespace zabato
{

const rtti serializer_link::TYPE("zabato.serializer_link", &object::TYPE);

/**
 * @brief Construct a new serializer object.
 * Initializes readers and writers to nullptr.
 */
serializer::serializer(resource_manager &manager)
    : m_manager(&manager), m_reader(nullptr), m_writer(nullptr)
{
}

/**
 * @brief Destroy the serializer object.
 * Cleans up allocated serializer_links.
 */
serializer::~serializer()
{
    // Clean up links
    for (serializer_link *link : m_links)
        delete link;
    m_links.clear();
}

bool serializer::insert_in_map(const object *obj, void *value)
{
    if (m_unique_map.try_get_value(obj, value))
        return false; // Already exists

    m_unique_map.add(obj, value);
    return true;
}

void serializer::insert_in_ordered(const object *obj)
{
    m_ordered_objects.push_back(obj);
}

bool serializer::insert_in_map(void *unique_id, object *obj_or_link)
{
    if (m_unique_map.contains_key(unique_id))
        return false;

    m_unique_map.add(unique_id, obj_or_link);
    return true;
}

object *serializer::get_from_map(void *unique_id)
{
    void *result = nullptr;
    if (m_unique_map.try_get_value(unique_id, result))
    {
        if (result)
        {
            object *obj = (object *)result;
            if (obj->is_derived(serializer_link::TYPE))
            {
                return ((serializer_link *)obj)->get_object();
            }
            return obj;
        }
    }
    return nullptr;
}

size_t serializer::read(object *&obj)
{
    uintptr_t ptr_val = 0;
    if (!m_reader)
        return 0;

    size_t bytes_read = m_reader->read(ptr_val);
    obj               = (object *)ptr_val;
    return bytes_read;
}

size_t serializer::write(const object *obj) { return write((void *)obj); }

size_t serializer::read(string &str)
{
    ice_int32_t length = 0;
    size_t bytes_read  = 0;
    if (!m_reader)
        return 0;

    bytes_read += m_reader->read(length);

    if (length > 0)
    {
        str.resize(length);

        // Direct read into string buffer
        auto buf = buffer((uint8_t *)str.data(), length);
        if (m_reader->read(buf) != (size_t)length)
            return 0; // Failed

        bytes_read += length;
    }
    return bytes_read;
}

size_t serializer::write(const string &str) { return write(string_view(str)); }

size_t serializer::write(const string_view &str)
{
    if (!m_writer)
        return 0;

    ice_int32_t length = (ice_int32_t)str.length();
    auto res           = m_writer->write(&length, sizeof(length));
    if (res.has_error())
        return 0;
    size_t bytes_written = *res;

    if (length > 0)
    {
        auto res2 = m_writer->write(str.data(), length);
        if (res2.has_error())
            return bytes_written;
        bytes_written += *res2;
    }
    return bytes_written;
}

size_t serializer::read(resource_ref &ref)
{
    string path;
    size_t bytes = read(path);
    if (bytes > 0)
    {
        ref.set_path(path.c_str());
        ref.set_manager(m_manager);
    }
    return bytes;
}

size_t serializer::write(const resource_ref &ref)
{
    string path = ref.path();
    return write(path);
}

bool serializer::save(stream &stream, const object *root)
{
    ice_writer writer(stream);
    m_writer = &writer;

    m_unique_map.clear();
    m_ordered_objects.clear();

    if (root)
        root->register_object(*this);

    string top_level = "Top Level";
    write(top_level);

    ice_int32_t count = (ice_int32_t)m_ordered_objects.size();
    write(count);

    for (const object *obj : m_ordered_objects)
    {
        obj->save(*this);
    }
    m_writer = nullptr;
    return true;
}

bool serializer::load(stream &stream)
{
    ice_reader reader(stream);
    m_reader = &reader;

    m_unique_map.clear();
    m_links.clear();

    string header;
    read(header);
    if (header != "Top Level")
    {
        m_reader = nullptr;
        return false;
    }

    ice_int32_t count = 0;
    read(count);

    for (int i = 0; i < count; ++i)
    {
        object *obj = object::factory(*this);
        // factory calls Load internally on object
        // And object::Load populates maps/links via stream calls
        (void)obj; // Suppress unused var warning if factory result ignored
    }

    // Link Phase
    for (auto it = m_unique_map.begin(); it != m_unique_map.end(); ++it)
    {
        // Value in map is link*
        serializer_link *link = (serializer_link *)(it->value);
        if (link && link->get_object())
        {
            link->get_object()->link(*this, link);
        }
    }

    m_reader = nullptr;
    return true;
}

} // namespace zabato
