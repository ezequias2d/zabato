#include "zabato/controller.hpp"
#include <algorithm>
#include <zabato/node.hpp>
#include <zabato/world.hpp>

namespace zabato
{

const rtti world::TYPE("zabato.world", &object::TYPE);

world::world() : m_root(nullptr), m_controller_head(nullptr) {}

world::~world() {}

void world::set_scene_root(spatial *root)
{
    if (m_root)
    {
        unregister_controllers_recursive(m_root);
    }
    m_root = root;
    if (m_root)
    {
        register_controllers_recursive(m_root);
    }
}

void world::register_model(model *mod)
{
    if (std::find(m_models.begin(), m_models.end(), mod) == m_models.end())
    {
        m_models.push_back(mod);
    }
}

void world::unregister_model(model *mod) { m_models.remove(mod); }

void world::add_controller(controller *ctrl)
{
    if (!ctrl)
        return;

    if (m_controller_head)
        m_controller_head->m_prev = ctrl;
    ctrl->m_next      = m_controller_head;
    ctrl->m_prev      = nullptr;
    m_controller_head = ctrl;
}

void world::remove_controller(controller *ctrl)
{
    if (!ctrl)
        return;

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
    // Update Controllers
    controller *curr = m_controller_head;
    while (curr)
    {
        controller *next = curr->m_next;
        curr->update(dt);
        curr = next;
    }

    // Update models animators
    for (auto &mod : m_models)
    {
        auto animator = mod->get_animator();
        if (animator)
        {
            animator->update(dt);
        }
    }
}

void world::render(renderer &rnd, camera &cam)
{
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

} // namespace zabato
