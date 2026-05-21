#pragma once

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
    static void reflect(reflection &r);

    model();
    virtual ~model();

    virtual void save_xml(xml_serializer &serializer,
                          tinyxml2::XMLElement &element) const override;
    virtual void load_xml(xml_serializer &serializer,
                          tinyxml2::XMLElement &element) override;
    virtual void link(xml_serializer &serializer,
                      tinyxml2::XMLElement &element) override;

    virtual void save(serializer &serializer) const override;
    virtual void load(serializer &serializer, serializer_link *link) override;
    virtual void link(serializer &serializer, serializer_link *link) override;

    virtual void on_transform_changed() override;

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
     * @brief Get the path to the mesh resource.
     * @return The path to the mesh resource.
     */
    string_view get_mesh_path() const;

    /**
     * @brief Set the material resource for this model.
     * @param path The path to the material resource.
     */
    void set_material(const char *path);

    /**
     * @brief Set the material resource for this model.
     * @param mat The material to set.
     */
    void set_material(shared_ptr<class material> mat);

    /**
     * @brief Get the material resource.
     * @return Shared pointer to the material, or null if not loaded.
     */
    shared_ptr<class material> get_material() const;

    /**
     * @brief Get the path to the material resource.
     * @return The path to the material resource.
     */
    string_view get_material_path() const;

    void set_resource_manager(resource_manager *mgr);

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
    const vector<pointer<spatial>> &get_bones() const { return m_bones; }

    /**
     * @brief Get the evaluated bone matrices in model-local space.
     * @return Reference to the vector of matrices.
     */
    const vector<mat4<real>> &get_bone_matrices();

    /**
     * @brief Binds the scene graph nodes to the mesh's skeleton.
     * Searches for nodes in this model's hierarchy that match the mesh's bone
     * names.
     */
    void bind_skeleton(pointer<spatial> root);

    pointer<spatial> get_skeleton_root() const { return m_skeleton_root; }

private:
    resource_ref m_mesh;
    resource_ref m_material;
    shared_ptr<class material> m_material_override;
    bounding_volume *m_model_bound;
    bounding_volume *m_world_bound;
    bool m_bound_dirty;
    vector<pointer<spatial>> m_bones;
    pointer<spatial> m_skeleton_root;

    vector<mat4<real>> m_bone_matrices;
    vector<event<>::scoped_connection> m_bone_connections;
    bool m_bone_matrices_dirty   = true;
    bool m_skeleton_pending_bind = false;

    void on_bone_dirty()
    {
        m_bone_matrices_dirty = true;
        m_bound_dirty         = true;
    }

    void update_model_bound();
    void resolve_pending_skeleton_bind();
};

} // namespace zabato
