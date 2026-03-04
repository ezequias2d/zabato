#include <stdio.h>
#include <tinyxml2.h>
#include <zabato/controller.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/ice.hpp>
#include <zabato/object.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/serializer.hpp>
#include <zabato/stream.hpp>
#include <zabato/string_tree.hpp>
#include <zabato/symbol.hpp>
#include <zabato/world.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti object::TYPE("zabato.object", &base_object::TYPE, object::reflect);
hash_map<uuid, object *> object::s_in_use;
hash_map<string, object::factory_info> *object::s_factory = nullptr;

object::object() : m_name(""), m_uiID(uuid::generate())
{
    s_in_use.add(m_uiID, this);
}

object::~object() { s_in_use.erase(m_uiID); }

bool object::register_factory()
{
    if (!s_factory)
        initialize_factory();
    return true;
}

void object::initialize_factory()
{
    if (!s_factory)
        s_factory = new hash_map<string, factory_info>(FACTORY_MAP_SIZE);
}

void object::terminate_factory()
{
    if (s_factory)
    {
        delete s_factory;
        s_factory = nullptr;
    }
}

bool object::register_factory_type(const string &name,
                                   const rtti *type,
                                   factory_delegate f)
{
    if (!s_factory)
        initialize_factory();

    factory_info info;
    info.type    = type;
    info.factory = f;
    s_factory->add_or_set(name, info);
    return true;
}

const rtti *object::get_factory_type(const string &name)
{
    if (!s_factory)
        return nullptr;

    factory_info info;
    if (s_factory->try_get_value(name, info))
        return info.type;
    return nullptr;
}

object *object::create_default(const string &type_name, resource_manager &mgr)
{
    if (!s_factory)
        return nullptr;

    factory_info info;
    if (s_factory->try_get_value(type_name, info))
    {
        if (!info.factory)
            return nullptr;

        return info.factory();
    }
    return nullptr;
}

object *object::factory(string_view type_name)
{
    if (!s_factory)
        return nullptr;

    factory_info info;

    if (s_factory->try_get_value(type_name, info) && info.factory)
        return info.factory();

    return nullptr;
}

bool object::register_object(serializer &serializer) const
{
    object *pkThis = (object *)this;
    if (serializer.insert_in_map(pkThis, nullptr))
    {
        serializer.insert_in_ordered(pkThis);
        for (auto &controller : m_controllers)
            if (controller && !controller->register_object(serializer))
                return false;
    }

    return true;
}

void object::save(serializer &stream) const
{
    stream.write(string(type().name()));
    stream.write((object *)this);

    string n = name();
    stream.write(n);

    // controllers
    ice_int32_t quantity = m_controllers.size();
    stream.write(quantity);
    for (auto &controller : m_controllers)
        stream.write((const object *)controller);
}

void object::load(serializer &stream, serializer_link *link)
{
    object *pkLinkID = nullptr;
    stream.read(pkLinkID);

    stream.insert_in_map((void *)pkLinkID, link);

    string n;
    stream.read(n);
    set_name(n.c_str());

    // controllers
    ice_int32_t quantity = 0;
    stream.read(quantity);

    for (int i = 0; i < quantity; i++)
    {
        object *pkController = nullptr;
        stream.read(pkController);
        link->add_child_id(pkController);
    }
}

void object::link(serializer &serializer, serializer_link *link)
{
    // Base class has no children or references to link.
    //
    // Derived classes should implement this method to resolve pointers to other
    // objects. During load(), objects should have registered their
    // dependencies/children using link->add_child_id(). Inside link(), they
    // should retrieve these IDs using link->get_next_child_id() in the same
    // order and resolve them to actual object pointers using
    // stream.get_from_map().

    // controllers
    ice_int32_t quantity = 0;
    serializer.read(quantity);
    assert(quantity >= 0);
    m_controllers.resize(quantity);

    for (int i = 0; i < quantity; i++)
    {
        object *pkObj = link->get_next_child_id();
        if (pkObj)
        {
            auto c = c_dynamic_cast<controller>(serializer.get_from_map(pkObj));
            assert(c);
            m_controllers[i] = c;
        }
    }
}

void object::print_in_use(const char *file, const char *acMessage)
{
    printf("DEBUG: print_in_use (%s): %s\n", file, acMessage);
    for (auto &entry : s_in_use)
    {
        char buf[37];
        entry.key.to_chars(buf);
        printf("  - ID: %s, Obj: %p, Name: %s\n",
               buf,
               entry.value,
               entry.value->name());
    }
}

void object::set_id(const uuid &id)
{
    if (m_uiID != id)
    {
        s_in_use.erase(m_uiID);
        m_uiID = id;
        s_in_use.add(m_uiID, this);
    }
}

void object::load_xml(xml_serializer &serializer, tinyxml2::XMLElement &el)
{
    const char *id = el.Attribute("id");
    assert(id);
    uuid uuid_val;
    if (uuid::try_parse(id, uuid_val))
    {
        uuid_val = serializer.remap(uuid_val);
        set_id(uuid_val);
        serializer.add_object(uuid_val, this);
    }
    else
    {
        assert(false && "Invalid UUID");
    }

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
            object *cObj = object::factory(controller->Name());
            if (!cObj)
                continue;

            cObj->load_xml(serializer, *controller);

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
    el.SetAttribute("id", id().to_string().c_str());
    el.SetAttribute("name", name());

    if (!m_controllers.empty())
    {
        tinyxml2::XMLElement *controllers =
            el.InsertNewChildElement("controllers");
        for (auto &ctrl : m_controllers)
        {
            if (ctrl)
                serializer.write_object(*controllers, ctrl);
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
            if (index >= m_controllers.size())
            {
                printf("ERROR: object::link controller count mismatch! XML has "
                       "more controllers than loaded.\n");
                break;
            }

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
        serializer serializer(&manager);
        serializer.save(stream, this);
    }

    stream.rewind();

    {
        serializer serializer(&manager);
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

void object::set_name(const char *name) { m_name = name; }

void object::set_name(string_view name) { m_name = name; }

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
        w->add_controller(ctrl);
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
            w->remove_controller(ctrl);
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

void object::set_name(symbol *name) { m_name = name; }

const char *object::name() const { return m_name.c_str(); }

object *object::get_object_by_name(const char *name)
{
    if (!name || name[0] == '\0')
        return nullptr;

    symbol_ref s = name;
    object *obj  = get_object_by_name(s);
    return obj;
}

object *object::get_object_by_name(const symbol_ref &name)
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

    symbol_ref s = name;
    get_all_objects_by_name(s, objects);
}

void object::get_all_objects_by_name(const symbol_ref &name,
                                     vector<object *> &objects)
{
    if (m_name == name)
        objects.push_back(this);
}

static void
object_name_getter(script_system *sys, script_instance *ctx, script_args *args)
{
    if (args->count() < 1)
        return;
    value v          = args->get_value(0);
    base_object *obj = v.as_object();
    if (obj->is_derived(object::TYPE))
        args->push_return(static_cast<object *>(obj)->name());
    else
        args->type_error("Null object pointer");
}

static void
object_name_setter(script_system *sys, script_instance *ctx, script_args *args)
{
    if (args->count() < 2)
        return;
    value v          = args->get_value(0);
    base_object *obj = v.as_object();
    if (obj->is_derived(object::TYPE))
        static_cast<object *>(obj)->set_name(
            args->get_value(1).as_string().data());
    else
        args->type_error("Null object pointer");
}

static void object_get_object_by_name(script_system *sys,
                                      script_instance *ctx,
                                      script_args *args)
{
    if (args->count() < 2)
        return;
    value v          = args->get_value(0);
    base_object *obj = v.as_object();
    if (obj->is_derived(object::TYPE))
    {
        object *found = static_cast<object *>(obj)->get_object_by_name(
            args->get_value(1).as_string().data());
        args->push_return((void *)found);
    }
    else
        args->type_error("Null object pointer");
}

static void
object_id_getter(script_system *sys, script_instance *ctx, script_args *args)
{
    if (args->count() < 1)
        return;
    value v          = args->get_value(0);
    base_object *obj = v.as_object();
    if (obj->is_derived(object::TYPE))
    {
        string id = static_cast<object *>(obj)->id().to_string();
        args->push_return(id);
    }
    else
        args->type_error("Null object pointer");
}

void object::reflect(reflection &r)
{
    r.add_property("name", object_name_getter, object_name_setter);
    r.add_property("id", object_id_getter);
    r.add_method("get_object_by_name", object_get_object_by_name);
}

} // namespace zabato
