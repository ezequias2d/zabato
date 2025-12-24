#include <stdio.h>
#include <tinyxml2.h>
#include <zabato/controller.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/object.hpp>
#include <zabato/serializer.hpp>
#include <zabato/stream.hpp>
#include <zabato/string_tree.hpp>
#include <zabato/symbol.hpp>
#include <zabato/world.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti object::TYPE("zabato.object", nullptr);
hash_map<uuid, object *> object::s_in_use;
hash_map<string, object::factory_function> *object::s_factory         = nullptr;
hash_map<string, object::factory_function_xml> *object::s_factory_xml = nullptr;

object::object() : m_name(nullptr), m_uiID(uuid::generate()), m_uiRefCount(0) {}

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

    if (!s_factory_xml)
        s_factory_xml =
            new hash_map<string, factory_function_xml>(FACTORY_MAP_SIZE);
}

void object::terminate_factory()
{
    if (s_factory)
    {
        delete s_factory;
        s_factory = nullptr;
    }

    if (s_factory_xml)
    {
        delete s_factory_xml;
        s_factory_xml = nullptr;
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

object *object::factory(xml_serializer &serializer, tinyxml2::XMLElement &el)
{
    if (!s_factory)
        return nullptr;

    string name                = el.Name();
    factory_function_xml pFunc = nullptr;
    if (s_factory_xml->try_get_value(name, pFunc))
        return (*pFunc)(serializer, el);
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

void object::load_xml(xml_serializer &serializer, tinyxml2::XMLElement &el)
{
    const char *id = el.Attribute("id");
    assert(id);
    uuid uuid;
    uuid.parse(id);

    const char *name = el.Attribute("name");
    if (name)
        set_name(name);
    else
        set_name("");

    auto controllers = el.FirstChildElement("controllers");
    for (; controllers != nullptr;
         controllers = controllers->NextSiblingElement("controllers"))
    {
        tinyxml2::XMLElement *controller = controllers->FirstChildElement();
        for (; controller != nullptr;
             controller = controller->NextSiblingElement())
        {
            object *cObj = object::factory(serializer, *controller);
            if (!cObj)
                continue;

            pointer<zabato::controller> ctrl =
                c_dynamic_cast<zabato::controller>(cObj);
            assert(ctrl);
            add_controller(ctrl);
        }
    }
}

void object::save_xml(xml_serializer &serializer,
                      tinyxml2::XMLElement &el) const
{
    el.SetAttribute("id", uuid().to_string().c_str());
    el.SetAttribute("name", name());

    if (!m_controllers.empty())
    {
        tinyxml2::XMLElement *controllers =
            el.InsertNewChildElement("controllers");
        for (auto &ctrl : m_controllers)
        {
            rtti type = ctrl->type();
            tinyxml2::XMLElement *controller =
                controllers->InsertNewChildElement(type.name());
            ctrl->save_xml(serializer, *controller);
        }
    }
}

void object::link(xml_serializer &serializer, tinyxml2::XMLElement &el)
{

    auto controllers = el.FirstChildElement("controllers");
    size_t index     = 0;
    for (; controllers != nullptr;
         controllers = controllers->NextSiblingElement("controllers"))
    {
        tinyxml2::XMLElement *controller = controllers->FirstChildElement();
        for (; controller != nullptr;
             controller = controller->NextSiblingElement())
        {
            pointer<zabato::controller> ctrl = m_controllers[index++];
            if (!ctrl)
                continue;

            ctrl->link(serializer, *controller);
        }
    }
}

int object::get_memory_used() const { return sizeof(*this); }

int object::get_disk_used() const { return 0; }

object *object::clone(resource_manager &manager) const
{
    vector<uint8_t> buffer;
    memory_stream stream(buffer);

    {
        serializer serializer(manager);
        serializer.save(stream, this);
    }

    stream.rewind();

    {
        serializer serializer(manager);
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

void object::add_controller(pointer<controller> ctrl)
{
    if (!ctrl)
        return;

    // Check if already added
    for (auto &c : m_controllers)
    {
        if (c == ctrl)
            return;
    }

    m_controllers.push_back(ctrl);
    ctrl->set_object(this);

    // If we are in a world, register the controller
    world *w = get_world();
    if (w)
    {
        w->add_controller(ctrl);
    }
}

void object::remove_controller(pointer<controller> ctrl)
{
    if (!ctrl)
        return;

    bool found = m_controllers.remove(ctrl);
    if (found)
    {
        ctrl->set_object(nullptr);
        world *w = get_world();
        if (w)
        {
            w->remove_controller(ctrl);
        }
    }
}

pointer<controller> object::get_controller(const rtti &type) const
{
    for (const auto &c : m_controllers)
        if (c->is_derived(type))
            return c;
    return nullptr;
}

void object::get_controllers(const rtti &type,
                             vector<pointer<controller>> &out_controllers) const
{
    for (const auto &c : m_controllers)
        if (c->is_derived(type))
            out_controllers.push_back(c);
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
