#include <zabato/base_object.hpp>
#include <zabato/camera.hpp>
#include <zabato/math.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti camera::TYPE("zabato.camera", &spatial::TYPE, camera::reflect);

static void
camera_fov_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    camera *c      = c_dynamic_cast<camera>(o);
    if (c)
        args->push_return((double)c->get_fov());
}

static void
camera_fov_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    camera *c      = c_dynamic_cast<camera>(o);
    value v2       = args->get_value(1);
    if (c)
        c->set_perspective(
            v2.as_number(), c->get_aspect(), c->get_near(), c->get_far());
}

static void
camera_aspect_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    camera *c      = c_dynamic_cast<camera>(o);
    if (c)
        args->push_return((double)c->get_aspect());
}
static void
camera_aspect_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    camera *c      = c_dynamic_cast<camera>(o);
    value v2       = args->get_value(1);
    if (c)
        c->set_perspective(
            c->get_fov(), v2.as_number(), c->get_near(), c->get_far());
}

static void
camera_near_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    camera *c      = c_dynamic_cast<camera>(o);
    if (c)
        args->push_return((double)c->get_near());
}
static void
camera_near_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    camera *c      = c_dynamic_cast<camera>(o);
    value v2       = args->get_value(1);
    if (c)
        c->set_perspective(
            c->get_fov(), c->get_aspect(), v2.as_number(), c->get_far());
}

static void
camera_far_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    value v        = args->get_value(0);
    base_object *o = v.as_object();
    camera *c      = c_dynamic_cast<camera>(o);
    if (c)
        args->push_return((double)c->get_far());
}
static void
camera_far_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    value v1       = args->get_value(0);
    base_object *o = v1.as_object();
    camera *c      = c_dynamic_cast<camera>(o);
    value v2       = args->get_value(1);
    if (c)
        c->set_perspective(c->get_fov(),
                           c->get_aspect(),
                           c->get_near(),
                           args->get_value(1).as_number());
}

void camera::reflect(reflection &r)
{
    spatial::reflect(r);
    r.properties.add("fov", {camera_fov_getter, camera_fov_setter});
    r.properties.add("aspect", {camera_aspect_getter, camera_aspect_setter});
    r.properties.add("near", {camera_near_getter, camera_near_setter});
    r.properties.add("far", {camera_far_getter, camera_far_setter});
}

void frustum::extract_from_matrix(const mat4<real> &vp)
{
    // Left
    planes[0].normal.x = vp[0][3] + vp[0][0];
    planes[0].normal.y = vp[1][3] + vp[1][0];
    planes[0].normal.z = vp[2][3] + vp[2][0];
    planes[0].d        = vp[3][3] + vp[3][0];

    // Right
    planes[1].normal.x = vp[0][3] - vp[0][0];
    planes[1].normal.y = vp[1][3] - vp[1][0];
    planes[1].normal.z = vp[2][3] - vp[2][0];
    planes[1].d        = vp[3][3] - vp[3][0];

    // Bottom
    planes[2].normal.x = vp[0][3] + vp[0][1];
    planes[2].normal.y = vp[1][3] + vp[1][1];
    planes[2].normal.z = vp[2][3] + vp[2][1];
    planes[2].d        = vp[3][3] + vp[3][1];

    // Top
    planes[3].normal.x = vp[0][3] - vp[0][1];
    planes[3].normal.y = vp[1][3] - vp[1][1];
    planes[3].normal.z = vp[2][3] - vp[2][1];
    planes[3].d        = vp[3][3] - vp[3][1];

    // Near
    planes[4].normal.x = vp[0][3] + vp[0][2];
    planes[4].normal.y = vp[1][3] + vp[1][2];
    planes[4].normal.z = vp[2][3] + vp[2][2];
    planes[4].d        = vp[3][3] + vp[3][2];

    // Far
    planes[5].normal.x = vp[0][3] - vp[0][2];
    planes[5].normal.y = vp[1][3] - vp[1][2];
    planes[5].normal.z = vp[2][3] - vp[2][2];
    planes[5].d        = vp[3][3] - vp[3][2];

    // Normalize planes
    for (int i = 0; i < 6; ++i)
    {
        real len = length(planes[i].normal);
        planes[i].normal /= len;
        planes[i].d /= len;
    }
}

camera::camera()
    : m_frustum_dirty(true), m_fov(45.0f), m_aspect(1.777f), m_near(0.1f),
      m_far(100.0f)
{
    m_projection = mat4<real>::identity();
    m_view       = mat4<real>::identity();
}

camera::~camera() {}

void camera::set_perspective(real fovY, real aspect, real near, real far)
{
    m_fov           = fovY;
    m_aspect        = aspect;
    m_near          = near;
    m_far           = far;
    m_projection    = mat4_perspective_fov(fovY, aspect, near, far);
    m_frustum_dirty = true;
}

void extract_transform(const mat4<real> &m, transformation &t)
{
    vec3<real> scale, trans;
    quat<real> rot;
    mat4_decompose(m, trans, scale, rot);

    t.set_translate(trans);
    t.set_scale(scale);
    t.set_rotate(rot);
}

void camera::look_at(const vec3<real> &eye,
                     const vec3<real> &center,
                     const vec3<real> &up)
{
    m_view = mat4_look_at(eye, center, up);

    mat4<real> world_mat;
    if (inverse(m_view, world_mat))
    {
        transformation t;
        extract_transform(world_mat, t);
        set_local(t);
    }

    m_frustum_dirty = true;
}

mat4<real> camera::get_view_projection() const { return m_projection * m_view; }

const frustum &camera::get_frustum() const
{
    if (m_frustum_dirty)
    {
        m_frustum.extract_from_matrix(get_view_projection());
        m_frustum_dirty = false;
    }
    return m_frustum;
}

void camera::update_view_from_transform()
{
    transformation t     = get_world_transform();
    mat4<real> world_mat = mat4_translation(t.translate()) *
                           mat4_from_quat(t.rotate()) * mat4_scaling(t.scale());

    // Invert to get view matrix
    mat4<real> view_mat;
    if (inverse(world_mat, view_mat))
    {
        m_view          = view_mat;
        m_frustum_dirty = true;
    }
}

void camera::save_xml(xml_serializer &serializer,
                      tinyxml2::XMLElement &element) const
{
    spatial::save_xml(serializer, element);
    element.SetAttribute("fov", (double)m_fov);
    element.SetAttribute("aspect", (double)m_aspect);
    element.SetAttribute("near", (double)m_near);
    element.SetAttribute("far", (double)m_far);
}

void camera::load_xml(xml_serializer &serializer, tinyxml2::XMLElement &element)
{
    spatial::load_xml(serializer, element);

    float val;
    if (element.QueryFloatAttribute("fov", &val) == tinyxml2::XML_SUCCESS)
        m_fov = val;
    if (element.QueryFloatAttribute("aspect", &val) == tinyxml2::XML_SUCCESS)
        m_aspect = val;
    if (element.QueryFloatAttribute("near", &val) == tinyxml2::XML_SUCCESS)
        m_near = val;
    if (element.QueryFloatAttribute("far", &val) == tinyxml2::XML_SUCCESS)
        m_far = val;

    set_perspective(m_fov, m_aspect, m_near, m_far);
    update_view_from_transform();
}

void camera::on_transform_changed()
{
    spatial::on_transform_changed();
    update_view_from_transform();
}

} // namespace zabato
