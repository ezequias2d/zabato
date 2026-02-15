#pragma once

#include <zabato/camera.hpp>
#include <zabato/controller.hpp>
#include <zabato/game_message.hpp>
#include <zabato/light.hpp>
#include <zabato/model.hpp>
#include <zabato/physics/physics_world.hpp>
#include <zabato/renderer.hpp>
#include <zabato/spatial.hpp>

namespace zabato
{

class world : public spatial
{
public:
    static const rtti TYPE;
    virtual const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    world();
    virtual ~world();

    void set_context(const controller::context &ctx) { m_context = ctx; }

    const controller::context &get_context() const { return m_context; }

    physics::physics_world *get_physics() const { return m_physics; }
    void set_physics(physics::physics_world *phy) { m_physics = phy; }

    /**
     * @brief Set the active camera for the world.
     * @param cam The camera to set as active.
     */
    void set_active_camera(camera *cam);

    /**
     * @brief Get the currently active camera.
     * @return Pointer to the active camera.
     */
    camera *get_active_camera() const;

    virtual world *get_world() const override final
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
    const vector<model *> &get_models() const { return m_models; }

    /**
     * @brief Unregister a model from the world.
     * @param mod The model to unregister.
     */
    void unregister_model(model *mod);

    void register_light(light *l);
    void unregister_light(light *l);
    const vector<light *> &get_lights() const { return m_lights; }

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
     * @brief Clean the world (remove all models, lights, controllers, and scene
     * root).
     */
    void clean();

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

    /**
     * @brief Finds the first camera in the scene graph.
     * @return Pointer to the camera or nullptr if not found.
     */
    camera *find_camera();

private:
    void update_node(spatial *node, real dt);
    void process_messages();

    pointer<spatial> m_root;
    pointer<camera> m_active_camera;
    vector<model *> m_models;
    vector<light *> m_lights;

    controller *m_controller_head;

    // Message Queue
    game_message_queue m_message_queue;

    pointer<physics::physics_world> m_physics;
    controller::context m_context;
};

} // namespace zabato
