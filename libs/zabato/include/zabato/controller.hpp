#pragma once

#include <zabato/console.hpp>
#include <zabato/object.hpp>
#include <zabato/resource.hpp>

namespace zabato
{
struct game_message;
class gpu;
class console;
class resource_manager;

/**
 * @class controller
 * @brief Base class for any logic that controls an object or runs periodically.
 * Maintains a global linked list of all active controllers for easy iteration.
 */
class controller : public object
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    controller();
    virtual ~controller();

    struct context
    {
        resource_manager *resources  = nullptr;
        console *logger              = nullptr;
        class window *window         = nullptr;
        class script_system *scripts = nullptr;
    };

    /**
     * @brief Called when the controller is first loaded.
     */
    virtual void initialize(const context &ctx)
    {
        m_console          = ctx.logger;
        m_resource_manager = ctx.resources;
    }

    /**
     * @brief Called when the controller is first started.
     */
    virtual void start() = 0;

    /**
     * @brief Update logic called every frame.
     * @param dt Delta time.
     */
    virtual void update(real dt) = 0;

    /**
     * @brief Sets a generic property on this controller.
     * @param name The identification/name of the property.
     * @param val The value to set.
     */
    virtual void set_property(const char *name, real val) {}
    virtual void set_property(const char *name, int64_t val) {}
    virtual void set_property(const char *name, bool val) {}
    virtual void set_property(const char *name, const char *val) {}

    virtual void on_message(const game_message &msg) {}

    virtual void on_draw_gizmos(class gpu &g, bool selected) {}

    /**
     * @brief Set the object this controller "possesses".
     * @param obj The target object.
     */
    void set_object(object *obj);
    object *get_object() const { return m_object; }

    // Intrusive list pointers for world
    controller *next() const { return m_next; }
    controller *prev() const { return m_prev; }

    // Friend world to allow it to manipulate links
    friend class world;

    resource_manager *get_resource_manager() const
    {
        return m_resource_manager;
    }
    console *get_console() const { return m_console; }

protected:
    object *m_object;
    resource_manager *m_resource_manager = nullptr;
    console *m_console                   = nullptr;

private:
    controller *m_next;
    controller *m_prev;
};

} // namespace zabato
