#pragma once

#include <zabato/camera.hpp>
#include <zabato/controller.hpp>
#include <zabato/game_message.hpp>
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
     * @brief Get all registered models.
     * @return Const reference to the vector of models.
     */
    const vector<pointer<model>> &get_models() const { return m_models; }

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

    void render(renderer &rnd, camera &cam);

    /**
     * @brief Send a game message to be processed in the next update.
     * @param msg The message to send.
     */
    void send_message(const game_message &msg);

private:
    void update_node(spatial *node, real dt);
    void process_messages();

    pointer<spatial> m_root;
    vector<pointer<model>> m_models;

    controller *m_controller_head;

    // Message Queue
    game_message_queue m_message_queue;
};

} // namespace zabato
