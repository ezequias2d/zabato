#pragma once

#include <zabato/object.hpp>

namespace zabato
{
struct game_message;

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

    controller();
    virtual ~controller();

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

protected:
    object *m_object;

private:
    controller *m_next;
    controller *m_prev;
};

} // namespace zabato
