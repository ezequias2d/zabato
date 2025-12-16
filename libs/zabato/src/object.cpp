#include <stdio.h>
#include <zabato/hash_map.hpp>
#include <zabato/object.hpp>
#include <zabato/serializer.hpp>
#include <zabato/stream.hpp>
#include <zabato/string_tree.hpp>
#include <zabato/symbol.hpp>

namespace zabato
{

const rtti object::TYPE("zabato.object", nullptr);
unsigned int object::ms_uiNextID = 0;
hash_map<unsigned int, object *> object::s_in_use;
hash_map<string, object::factory_function> *object::s_factory = nullptr;

object::object() : m_name(nullptr), m_uiID(next_id()), m_uiRefCount(0) {}

object::~object()
{
    if (m_name)
        release_symbol(m_name);
}

bool object::register_factory()
{
    if (!s_factory)
        initialize_factory();
    return true;
}

void object::initialize_factory()
{
    if (!s_factory)
        s_factory = new hash_map<string, factory_function>(FACTORY_MAP_SIZE);
}

void object::terminate_factory()
{
    if (s_factory)
    {
        delete s_factory;
        s_factory = nullptr;
    }
}

object *object::factory(serializer &stream)
{
    if (!s_factory)
        return nullptr;

    // Read the RTTI type string (e.g., "zabato.object") from the stream.
    // This identifies the class of the object to be created.
    string name;
    stream.read(name);

    factory_function pFunc = nullptr;

    // Dispatch to the specific class factory function.
    // The registered factory function is responsible for:
    // 1. Instantiating the specific class (e.g., via new).
    // 2. invoking the Load() method to populate the object from the stream.
    // Note that the RTTI name has already been consumed by this dispatcher.
    if (s_factory->try_get_value(name, pFunc))
        return (*pFunc)(stream);

    // If the class is not registered in the factory map, return nullptr.
    // This indicates that the class type serialized in the stream is unknown to
    // the runtime.
    return nullptr;
}

bool object::register_object(serializer &stream) const
{
    object *pkThis = (object *)this;
    if (stream.insert_in_map(pkThis, nullptr))
    {
        stream.insert_in_ordered(pkThis);
        return true;
    }
    return false;
}

void object::save(serializer &stream) const
{
    stream.write(string(type().name()));
    stream.write((object *)this);

    string n = name();
    stream.write(n);

    // link data
    int quantity = 0;
    stream.write(quantity);
}

void object::load(serializer &stream, serializer_link *link)
{
    object *pkLinkID = nullptr;
    stream.read(pkLinkID);

    stream.insert_in_map((void *)pkLinkID, link);

    string n;
    stream.read(n);
    set_name(n.c_str());

    // link data
    int quantity = 0;
    stream.read(quantity);

    for (int i = 0; i < quantity; i++)
    {
        object *pkChild = nullptr;
        stream.read(pkChild);
        link->add_child_id(pkChild);
    }
}

void object::link(serializer &stream, serializer_link *link)
{
    // Base class has no children or references to link.
    //
    // Derived classes should implement this method to resolve pointers to other
    // objects. During load(), objects should have registered their
    // dependencies/children using link->add_child_id(). Inside link(), they
    // should retrieve these IDs using link->get_next_child_id() in the same
    // order and resolve them to actual object pointers using
    // stream.get_from_map().
}

int object::get_memory_used() const { return sizeof(*this); }

int object::get_disk_used() const { return 0; }

object *object::clone() const
{
    vector<uint8_t> buffer;
    memory_stream stream(buffer);

    {
        serializer serializer;
        serializer.save(stream, this);
    }

    stream.rewind();

    {
        serializer serializer;
        serializer.load(stream);
        return serializer.get_from_map((void *)this);
    }
}

void object::save_strings(string_tree *tree)
{
    char acBuffer[256];
    sprintf(acBuffer, "%s: %s", type().name(), name());
    tree->set_text(acBuffer);
}

void object::set_name(const char *name)
{
    if (m_name)
        release_symbol(m_name);
    m_name = get_symbol(name);
}

void object::set_name(symbol *name)
{
    if (m_name)
        release_symbol(m_name);

    m_name = name;
    if (m_name)
        ref_symbol(m_name);
}

const char *object::name() const
{
    if (m_name)
        return get_symbol_name(m_name);
    return "";
}

object *object::get_object_by_name(const char *name)
{
    if (!name || name[0] == '\0')
        return nullptr;

    symbol *s = get_symbol(name);
    if (!s)
        return nullptr;

    object *obj = get_object_by_name(s);
    release_symbol(s);
    return obj;
}

object *object::get_object_by_name(const symbol *name)
{
    if (m_name == name)
        return this;
    return nullptr;
}

void object::get_all_objects_by_name(const char *name,
                                     vector<object *> &objects)
{
    if (!name || name[0] == '\0')
        return;

    symbol *s = get_symbol(name);
    if (!s)
        return;

    get_all_objects_by_name(s, objects);
    release_symbol(s);
}

void object::get_all_objects_by_name(const symbol *name,
                                     vector<object *> &objects)
{
    if (m_name == name)
        objects.push_back(this);
}

} // namespace zabato
