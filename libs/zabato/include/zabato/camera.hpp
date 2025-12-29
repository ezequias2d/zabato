#pragma once

#include <zabato/math.hpp>
#include <zabato/spatial.hpp>

namespace zabato
{

struct frustum
{
    plane3<real> planes[6];

    /**
     * @brief Extract frustum planes from view-projection matrix.
     * @param vp The view-projection matrix.
     */
    void extract_from_matrix(const mat4<real> &vp);
};

class camera : public spatial
{
public:
    static const rtti TYPE;
    virtual const rtti &type() const override { return TYPE; }

    camera();
    virtual ~camera();

    /**
     * @brief Set perspective projection parameters.
     * @param fovY Field of view in Y direction (radians).
     * @param aspect Aspect ratio (width / height).
     * @param near Near plane distance.
     * @param far Far plane distance.
     */
    void set_perspective(real fovY, real aspect, real near, real far);

    /**
     * @brief Set view parameters using look-at.
     * @param eye Camera position.
     * @param center Target position.
     * @param up Up vector.
     */
    void look_at(const vec3<real> &eye,
                 const vec3<real> &center,
                 const vec3<real> &up);

    const mat4<real> &get_projection() const { return m_projection; }
    const mat4<real> &get_view() const { return m_view; }

    /**
     * @brief Get the view-projection matrix.
     * @return projection * view
     */
    mat4<real> get_view_projection() const;

    /**
     * @brief Get the frustum associated with this camera.
     * @return Frustum.
     */
    const frustum &get_frustum() const;

    // Override spatial set_world/set_local to verify view matrix updates?
    // Usually camera is a node in scene. World transform of node = inverse view
    // matrix? standard: View Matrix = Inverse(Camera World Matrix). so if we
    // move camera node, view matrix updates.

    /**
     * @brief Update view matrix from spatial world transform.
     */
    void update_view_from_transform();

private:
    mat4<real> m_projection;
    mat4<real> m_view;
    mutable frustum m_frustum;
    mutable bool m_frustum_dirty;
};

} // namespace zabato
