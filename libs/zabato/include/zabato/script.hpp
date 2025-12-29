#pragma once

#include <zabato/controller.hpp>
#include <zabato/error.hpp>
#include <zabato/fs.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/resource.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/span.hpp>
#include <zabato/string.hpp>
#include <zabato/value.hpp>

namespace zabato
{
struct game_message;

using script_type  = value_type;
using script_value = value;

class script_args
{
public:
    virtual ~script_args() = default;

    virtual int count() const                 = 0;
    virtual script_value get_value(int index) = 0;

    // Return Handling
    virtual void push_return(const script_value &v) = 0;

    // Error Handling (Terminal - Must return after calling)
    virtual void error(const char *msg)      = 0;
    virtual void type_error(const char *msg) = 0;
};

struct script_class_def
{
    const char *class_name;
    value constructor;
    value destructor;

    struct method_def
    {
        const char *name;
        value cb;
    };

    struct property_def
    {
        const char *name;
        value getter;
        value setter; // can be nullptr (readonly)
    };

    hash_map<string, method_def> methods;
    hash_map<string, property_def> properties;
};

struct script : public resource
{
public:
    static constexpr chunk_id CHUNK_ID = chunk_id("SCRT");

    script() = default;
    script(string_view source) : resource() { m_source = source; }

    string_view source() const { return m_source; }
    const char *c_source() { return m_source.c_str(); }

    template <typename T>
    friend result<void> serialize(ice_writer &writer, const T &m);

    template <typename T>
    friend result<void> deserialize(ice_reader &reader, T &m);

private:
    string m_source;
};

template <> inline result<void> serialize(ice_writer &writer, const script &obj)
{
    writer.write_chunk_header(script::CHUNK_ID, obj.m_source.size());
    buffer buf((uint8_t *)obj.m_source.data(), obj.m_source.size());
    return writer.write(buf);
}

template <> inline result<void> deserialize(ice_reader &reader, script &obj)
{
    bool compressed  = false;
    const auto chunk = reader.find_chunk_or_berg(script::CHUNK_ID, compressed);
    if (!chunk)
        return report_error(error_code::chunk_broken,
                            script::CHUNK_ID.to_string().c_str(),
                            (uint32_t)script::CHUNK_ID);

    obj.m_source.resize(chunk->size);
    buffer buf((uint8_t *)obj.m_source.data(), obj.m_source.size());
    const auto readed = reader.read(buf);
    assert(readed == chunk->size);
    return error_code::ok;
}

/**
 * @class IScriptInstance
 * @brief Represents a single, isolated script file/object (e.g. "player.lua").
 */
class script_instance : public controller
{
public:
    static const rtti TYPE;

    const rtti &type() const override { return TYPE; }

    script_instance(const shared_ptr<script> &script)
        : controller(), m_script(script)
    {
    }
    virtual ~script_instance() = default;

    /**
     * @brief Returns the script this instance is associated with.
     */
    const shared_ptr<zabato::script> &script() const { return m_script; }

    /**
     * @brief Called every frame to run the script's update logic.
     * @param dt Delta time in seconds.
     */
    virtual void update(real dt) override = 0;

    /**
     * @brief Called when the object receives a message from the MessageSystem.
     * @param msg The game message containing ID and data.
     */
    virtual void on_message(const game_message &msg) override = 0;

    virtual void on_draw_gizmos(gpu &g, bool selected) override {}

    /**
     * @brief Sets a variable directly inside this script's scope.
     * @param name Name of the variable.
     * @param val Value to set.
     */
    virtual void set_property(const char *name, real val) override = 0;
    virtual void set_property(const char *name, int64_t val) override {}
    virtual void set_property(const char *name, bool val) override {}
    virtual void set_property(const char *name, const char *val) override {}

protected:
    shared_ptr<zabato::script> m_script;
};

/**
 * @class IScriptSystem
 * @brief The Main Engine / Virtual Machine Interface.
 * Handles the lifecycle of the VM, global registration, and resource
 * management.
 */
class script_system
{
public:
    script_system(fs::file_system &fs) : m_fs(fs) {}
    virtual ~script_system() = default;

    /** @brief Initialize the VM. */
    virtual bool initialize() = 0;

    /** @brief Shutdown and cleanup VM. */
    virtual void shutdown() = 0;

    /** @brief Periodic update (Garbage Collection step). */
    virtual void tick() = 0;

    /**
     * @brief Loads a script file and returns an isolated instance.
     * * This allows multiple objects to use the same script logic but have
     * different data (self).
     * @param filepath Path to the script file (e.g., "/scripts/goblin.lua")
     * @param owner_id The ID of the C++ object owning this script (injected
     * into script).
     * @return Pointer to new instance, or nullptr on failure.
     */
    virtual script_instance *load_script(const char *filepath,
                                         uuid owner_id) = 0;

    /** @brief Register a global C++ function callable from anywhere. */
    virtual void register_global_function(const string_view &name,
                                          value cb) = 0;

    /** @brief Register a C++ Class/Userdata type. */
    virtual void register_class(const script_class_def &def) = 0;

    /** @brief Set a global variable available to all scripts. */
    virtual void set_global_var(const char *name, const script_value &val) = 0;

protected:
    fs::file_system &m_fs;
};
} // namespace zabato