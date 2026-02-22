#include <zabato/controller.hpp>
#include <zabato/light.hpp>
#include <zabato/model.hpp>
#include <zabato/node.hpp>
#include <zabato/physics/physics_controller.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/world.hpp>

namespace zabato
{

const rtti world::TYPE("zabato.world", &spatial::TYPE, world::reflect);

world::world()
    : m_root(nullptr), m_active_camera(nullptr), m_controller_head(nullptr),
      m_physics(nullptr)
{
}

world::~world() { clean(); }

void world::set_active_camera(camera *cam) { m_active_camera = cam; }

camera *world::get_active_camera() const { return m_active_camera; }

void world::set_scene_root(spatial *root)
{
    if (m_root)
    {
        unregister_controllers_recursive(m_root);
    }
    m_root = root;
    if (m_root)
    {
        m_root->set_parent(this);
        register_controllers_recursive(m_root);
    }
}

void world::clean()
{
    set_active_camera(nullptr);
    m_models.clear();
    m_lights.clear();
    set_scene_root(nullptr);

    while (m_controller_head)
    {
        remove_controller(m_controller_head);
    }
}

void world::register_model(model *mod)
{
    if (find(m_models.begin(), m_models.end(), mod) == m_models.end())
    {
        m_models.push_back(mod);
    }
}

void world::unregister_model(model *mod) { m_models.remove(mod); }

void world::register_light(light *l)
{
    if (find(m_lights.begin(), m_lights.end(), l) == m_lights.end())
    {
        m_lights.push_back(l);
    }
}

void world::unregister_light(light *l) { m_lights.remove(l); }

void world::add_controller(controller *ctrl)
{
    if (!ctrl)
        return;

    ctrl->initialize(m_context);

    if (m_controller_head)
        m_controller_head->m_prev = ctrl;
    ctrl->m_next      = m_controller_head;
    ctrl->m_prev      = nullptr;
    m_controller_head = ctrl;

    if (m_physics && ctrl->is_derived(physics::physics_controller::TYPE))
        m_physics->add_controller(
            static_cast<physics::physics_controller *>(ctrl));
}

void world::remove_controller(controller *ctrl)
{
    if (!ctrl)
        return;

    if (m_physics && ctrl->is_derived(physics::physics_controller::TYPE))
    {
        m_physics->remove_controller(
            static_cast<physics::physics_controller *>(ctrl));
    }

    if (ctrl->m_prev)
        ctrl->m_prev->m_next = ctrl->m_next;
    if (ctrl->m_next)
        ctrl->m_next->m_prev = ctrl->m_prev;

    if (ctrl == m_controller_head)
    {
        m_controller_head = ctrl->m_next;
    }

    ctrl->m_next = nullptr;
    ctrl->m_prev = nullptr;
}

void world::register_controllers_recursive(spatial *s)
{
    if (!s)
        return;

    // Register s's controllers
    const auto &ctrls = s->get_controllers();
    for (auto &c : ctrls)
        add_controller(c);

    if (s->is_derived(model::TYPE))
        register_model(static_cast<model *>(s));
    else if (s->is_derived(light::TYPE))
        register_light(static_cast<light *>(s));

    // Recurse children if node
    if (s->is_derived(node::TYPE))
    {
        node *n = static_cast<node *>(s);
        for (int i = 0; i < n->quantity(); ++i)
            register_controllers_recursive(n->child_at(i));
    }
}

void world::unregister_controllers_recursive(spatial *s)
{
    if (!s)
        return;

    // Unregister s's controllers
    const auto &ctrls = s->get_controllers();
    for (auto &c : ctrls)
        remove_controller(c);

    if (s->is_derived(model::TYPE))
        unregister_model(static_cast<model *>(s));
    else if (s->is_derived(light::TYPE))
        unregister_light(static_cast<light *>(s));

    // Recurse children if node
    if (s->is_derived(node::TYPE))
    {
        node *n = static_cast<node *>(s);
        for (int i = 0; i < n->quantity(); ++i)
            unregister_controllers_recursive(n->child_at(i));
    }
}

void world::update(real dt)
{
    // Update Physics
    if (m_physics)
    {
        m_physics->update(dt);
    }

    // Update Controllers
    controller *curr = m_controller_head;
    while (curr)
    {
        controller *next = curr->m_next;
        curr->update(dt);
        curr = next;
    }

    process_messages();
}

void world::send_message(const game_message &msg) { m_message_queue.push(msg); }

void world::process_messages()
{
    game_message msg;
    while (m_message_queue.pop(msg))
    {
        // Find receiver
        object *receiver = nullptr;
        object::s_in_use.try_get_value(msg.receiver_id, receiver);

        if (receiver)
        {
            const auto &ctrls = receiver->get_controllers();
            for (auto &c : ctrls)
                c->on_message(msg);
        }
    }
}

void world::render(renderer &rnd, camera &cam)
{
    for (auto &l : m_lights)
    {
        rnd.submit(l);
    }

    const frustum &f = cam.get_frustum();

    for (auto &mod : m_models)
    {
        // View Frustum Culling
        bool visible        = true;
        bounding_volume *bv = mod->get_world_bound();
        if (bv)
        {
            for (int i = 0; i < 6; ++i)
            {
                if (bv->which_side(f.planes[i]) < 0)
                {
                    visible = false;
                    break;
                }
            }
        }

        if (visible && mod->get_mesh())
            rnd.submit(mod);
    }
}

static camera *find_camera_recursive(spatial *s)
{
    if (!s)
        return nullptr;

    if (s->is_derived(camera::TYPE))
        return static_cast<camera *>(s);

    if (s->is_derived(node::TYPE))
    {
        node *n = static_cast<node *>(s);
        for (int i = 0; i < n->quantity(); ++i)
        {
            camera *c = find_camera_recursive(n->child_at(i));
            if (c)
                return c;
        }
    }
    return nullptr;
}

camera *world::find_camera()
{
    if (m_active_camera)
        return m_active_camera;
    return find_camera_recursive(m_root);
}

static void world_active_camera_getter(script_system *,
                                       script_instance *,
                                       script_args *args)
{
    if (args->count() < 1)
        return;
    auto obj = args->get_value(0).as_object();
    world *w = c_dynamic_cast<world>(obj.get());
    if (w)
        args->push_return(w->get_active_camera());
}

static void world_active_camera_setter(script_system *,
                                       script_instance *,
                                       script_args *args)
{
    if (args->count() < 2)
        return;
    auto obj  = args->get_value(0).as_object();
    world *w  = c_dynamic_cast<world>(obj.get());
    camera *c = c_dynamic_cast<camera>(args->get_value(1).as_object().get());
    if (w)
        w->set_active_camera(c);
}

void world::reflect(reflection &r)
{
    r.add_property("active_camera",
                   world_active_camera_getter,
                   world_active_camera_setter);
}

} // namespace zabato
