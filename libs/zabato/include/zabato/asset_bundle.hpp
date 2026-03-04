#pragma once

#include <zabato/resource.hpp>
#include <zabato/vector.hpp>

namespace zabato
{

/**
 * @class asset_bundle
 * @brief A container resource holding multiple sub-resources loaded from a
 * single file.
 */
class asset_bundle : public resource
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }

    struct sub_resource
    {
        string name;
        shared_ptr<resource> res;
    };

    /**
     * @brief Adds a sub-resource to the bundle.
     * @param name Name of the resource within the bundle (e.g. "mesh_0",
     * "anim_Run").
     * @param res The resource to store.
     */
    void add_resource(const string &name, shared_ptr<resource> res);

    /**
     * @brief Retrieves a sub-resource by its name.
     * @param name The name of the resource.
     * @return A pointer to the resource, or nullptr if not found.
     */
    shared_ptr<resource> get_resource(const string &name) const;

    /**
     * @brief Retrieves all resources in this bundle.
     */
    const vector<sub_resource> &get_resources() const { return m_resources; }

    /**
     * @brief Looks for the first resource matching the specified type.
     */
    shared_ptr<resource> get_first_resource_of_type(const rtti &type) const;

private:
    vector<sub_resource> m_resources;
};

} // namespace zabato
