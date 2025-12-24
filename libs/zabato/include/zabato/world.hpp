#pragma once

#include <zabato/camera.hpp>
#include <zabato/controller.hpp>
#include <zabato/model.hpp>
#include <zabato/renderer.hpp> // forward decl?
#include <zabato/spatial.hpp>

namespace zabato
{

class world : public object
{
public:
    static const rtti TYPE;
    virtual const rtti &type() const override { return TYPE; }

    world();
    virtual ~world();

    virtual world *get_world() const override
    {
        return const_cast<world *>(this);
    }

    /**
     * @brief Set the root of the scene graph.
     * @param root Pointer to the root spatial node.
     */
    void set_scene_root(spatial *root);

    /**
     * @brief Get the scene root.
     * @return Pointer to root.
     */
    spatial *get_scene_root() const { return m_root; }

    /**
     * @brief Register a model to the world.
     * Use this if the model is already in the scene graph but not tracked by
     * world.
     * @param mod The model to register.
     */
    void register_model(model *mod);

    /**
     * @brief Unregister a model from the world.
     * @param mod The model to unregister.
     */
    void unregister_model(model *mod);

    void add_controller(controller *ctrl);
    void remove_controller(controller *ctrl);

    /**
     * @brief Recursively register all controllers attached to the spatial and
     * its children.
     * @param s The root spatial to start from.
     */
    void register_controllers_recursive(spatial *s);

    /**
     * @brief Recursively unregister all controllers attached to the spatial and
     * its children.
     * @param s The root spatial to start from.
     */
    void unregister_controllers_recursive(spatial *s);

    /**
     * @brief Update the world (scene graph transforms, animations, etc).
     * @param dt Delta time in seconds.
     */
    void update(real dt);

    void render(renderer &rnd, camera *cam);

private:
    void update_node(spatial *node, real dt);

    pointer<spatial> m_root;
    vector<pointer<model>> m_models;

    controller *m_controller_head;
};

} // namespace zabato
