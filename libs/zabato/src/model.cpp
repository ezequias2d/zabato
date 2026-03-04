#include <zabato/gpu.hpp>
#include <zabato/material.hpp>
#include <zabato/mesh.hpp>
#include <zabato/model.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/serializer.hpp>
#include <zabato/world.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti model::TYPE("zabato.model", &spatial::TYPE, model::reflect);

static void
model_mesh_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    auto v   = args->get_value(0);
    auto o   = v.as_object();
    model *m = c_dynamic_cast<model>(o.get());
    if (m)
    {
        args->push_return(m->get_mesh() ? m->get_mesh_path() : "");
    }
}

static void
model_mesh_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    auto v1  = args->get_value(0);
    auto o   = v1.as_object();
    model *m = c_dynamic_cast<model>(o.get());
    auto v2  = args->get_value(1);
    if (m)
    {
        string_view path = v2.as_string();
        m->set_mesh(string(path).c_str());
    }
}

static void
model_material_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    auto v   = args->get_value(0);
    auto o   = v.as_object();
    model *m = c_dynamic_cast<model>(o.get());
    if (m)
    {
        args->push_return(m->get_material() ? m->get_material_path() : "");
    }
}

static void
model_material_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    auto v1  = args->get_value(0);
    auto o   = v1.as_object();
    model *m = c_dynamic_cast<model>(o.get());
    auto v2  = args->get_value(1);
    if (m)
    {
        string_view path = v2.as_string();
        m->set_material(string(path).c_str());
    }
}

void model::reflect(reflection &r)
{
    spatial::reflect(r);

    value mesh_attrs = value::make_map();
    mesh_attrs.set_field("asset_type", "mesh");
    r.add_property("mesh", model_mesh_getter, model_mesh_setter, mesh_attrs);

    value mat_attrs = value::make_map();
    mat_attrs.set_field("asset_type", "material");
    r.add_property(
        "material", model_material_getter, model_material_setter, mat_attrs);
}

model::model()
    : m_model_bound(nullptr), m_world_bound(nullptr), m_bound_dirty(true),
      m_bones()
{
}

model::~model()
{
    world *w = get_world();
    if (w)
        w->unregister_model(this);

    delete m_model_bound;
    delete m_world_bound;
}

void model::set_mesh(const char *path) { m_mesh.set_path(path); }

void model::set_resource_manager(resource_manager *mgr)
{
    m_mesh.set_manager(mgr);
    m_material.set_manager(mgr);
}

shared_ptr<mesh> model::get_mesh() const { return m_mesh.get<mesh>(); }

string_view model::get_mesh_path() const { return m_mesh.path(); }

void model::set_material(const char *path)
{
    m_material_override = nullptr;
    m_material.set_path(path);
}

void model::set_material(shared_ptr<material> mat)
{
    m_material_override = mat;
}

shared_ptr<material> model::get_material() const
{
    if (m_material_override)
        return m_material_override;
    return m_material.get<material>();
}

string_view model::get_material_path() const { return m_material.path(); }

void model::bind_skeleton()
{
    m_bones.clear();
    shared_ptr<mesh> m = get_mesh();
    if (!m)
        return;

    const auto &bones = m->get_bones();
    m_bones.resize(bones.size(), nullptr);

    for (size_t i = 0; i < bones.size(); ++i)
    {
        const auto &info = bones[i];
        object *obj      = get_object_by_name(info.name.c_str());
        spatial *bone    = c_dynamic_cast<spatial>(obj);
        if (bone)
            m_bones[i] = bone;
    }
}

void model::on_transform_changed()
{
    spatial::on_transform_changed();
    m_bound_dirty = true;
}

void model::update_model_bound()
{
    shared_ptr<mesh> m = get_mesh();

    delete m_model_bound;
    m_model_bound = nullptr;

    if (m)
    {
        vector<vec3<real>> points;
        points.resize(m->get_vertex_count());
        for (uint16_t i = 0; i < m->get_vertex_count(); ++i)
        {
            m->get_position(i, points[i]);
        }
        m_model_bound = bounding_volume::create(vec3<real>(0), 0);
        if (m_model_bound)
        {
            m_model_bound->compute_from_data(
                span<const vec3<real>>(points.data(), points.size()));
        }
    }
}

bounding_volume *model::get_world_bound()
{
    // Lazy update of model bound if needed (e.g. if mesh was just loaded)
    if (!m_model_bound && !m_mesh.path().empty())
    {
        update_model_bound();
    }

    if (!m_model_bound)
        return nullptr;

    if (!m_world_bound)
    {
        m_world_bound = bounding_volume::create(vec3<real>(0), 0);
        m_bound_dirty = true;
    }

    if (m_bound_dirty)
    {
        m_model_bound->transform_by(get_world_transform(), m_world_bound);
        m_bound_dirty = false;
    }

    return m_world_bound;
}

void model::save_xml(xml_serializer &serializer,
                     tinyxml2::XMLElement &element) const
{
    spatial::save_xml(serializer, element);
    xml_serializer::write_resource_ref(element, m_mesh);
    xml_serializer::write_resource_ref(element, m_material, "material");
}

void model::save(serializer &stream) const
{
    spatial::save(stream);
    stream.write(m_mesh);
    stream.write(m_material);
}

void model::load_xml(xml_serializer &serializer, tinyxml2::XMLElement &element)
{
    spatial::load_xml(serializer, element);

    xml_serializer::read_resource_ref(element, m_mesh);
    m_mesh.set_manager(serializer.get_manager());

    xml_serializer::read_resource_ref(element, m_material, "material");
    m_material.set_manager(serializer.get_manager());

    update_model_bound();
    bind_skeleton();
}

void model::load(serializer &stream, serializer_link *link)
{
    spatial::load(stream, link);
    stream.read(m_mesh);
    m_mesh.set_manager(stream.get_manager());

    stream.read(m_material);
    m_material.set_manager(stream.get_manager());

    update_model_bound();
    bind_skeleton();
}

} // namespace zabato
