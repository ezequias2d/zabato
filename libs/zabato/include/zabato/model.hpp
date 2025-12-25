#pragma once

#include <zabato/animator.hpp>
#include <zabato/bounding_volume.hpp>
#include <zabato/resource.hpp>
#include <zabato/spatial.hpp>

namespace zabato
{

class mesh;

class model : public spatial
{
public:
    static const rtti TYPE;
    virtual const rtti &type() const override { return TYPE; }

    model();
    virtual ~model();

    virtual void save_xml(xml_serializer &serializer,
                          tinyxml2::XMLElement &element) const override;
    virtual void load_xml(xml_serializer &serializer,
                          tinyxml2::XMLElement &element) override;

    /**
     * @brief Set the mesh resource for this model.
     * @param path The path to the mesh resource.
     */
    void set_mesh(const char *path);

    /**
     * @brief Get the mesh resource.
     * @return Shared pointer to the mesh, or null if not loaded.
     */
    shared_ptr<mesh> get_mesh() const;

    /**
     * @brief Set the animator for this model.
     * @param anim The animator to set.
     */
    void set_animator(animator *anim);

    void set_resource_manager(resource_manager *mgr);

    /**
     * @brief Get the animator.
     * @return Pointer to the animator.
     */
    animator *get_animator() const;

    /**
     * @brief Get the world space bounding volume.
     * Recomputes the bound based on current world transform.
     * @return Pointer to bounding volume (sphere). owned by model.
     */
    bounding_volume *get_world_bound();

    /**
     * @brief Get the list of spatial nodes acting as bones for this model.
     * @return Reference to the vector of bone nodes.
     */
    const vector<spatial *> &get_bones() const { return m_bones; }

    /**
     * @brief Binds the scene graph nodes to the mesh's skeleton.
     * Searches for nodes in this model's hierarchy that match the mesh's bone
     * names.
     */
    void bind_skeleton();

    // Serialization
    virtual void save(serializer &stream) const override;
    virtual void load(serializer &stream, serializer_link *link) override;

private:
    resource_ref m_mesh;
    bounding_volume *m_model_bound;
    bounding_volume *m_world_bound;
    vector<spatial *> m_bones;

    void update_model_bound();
};

} // namespace zabato
