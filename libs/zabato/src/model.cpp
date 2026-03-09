#include <zabato/gpu.hpp>
#include <zabato/material.hpp>
#include <zabato/mesh.hpp>
#include <zabato/model.hpp>
#include <zabato/object.hpp>
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

static void
model_skeleton_getter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 1)
        return;
    auto v   = args->get_value(0);
    auto o   = v.as_object();
    model *m = c_dynamic_cast<model>(o.get());
    if (m)
        args->push_return((object *)m->get_skeleton_root());
}

static void
model_skeleton_setter(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;
    auto v1  = args->get_value(0);
    auto o   = v1.as_object();
    model *m = c_dynamic_cast<model>(o.get());
    if (!m)
    {
        args->error("Invalid model");
        return;
    }

    auto v2 = args->get_value(1);
    if (v2.is_nil())
    {
        m->bind_skeleton(nullptr);
        return;
    }

    auto obj = v2.as_object();
    if (!obj)
    {
        args->error("Invalid skeleton root is not an object");
        return;
    }

    auto root = c_dynamic_cast<spatial>(obj.get());
    if (!root)
    {
        args->error("Invalid skeleton root is not a spatial");
        return;
    }

    m->bind_skeleton(root);
}

static void model_skeleton_filter(script_system *sys,
                                  script_instance *inst,
                                  script_args *args)
{
    if (args->count() < 1)
    {
        args->push_return(false);
        return;
    }

    auto v = args->get_value(0);
    auto o = v.as_object();
    auto s = c_dynamic_cast<spatial>(o.get());
    args->push_return(s != nullptr);
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

    value skeleton_attrs = value::make_map();
    skeleton_attrs.set_field("object_filter", model_skeleton_filter);
    r.add_property("skeleton",
                   model_skeleton_getter,
                   model_skeleton_setter,
                   skeleton_attrs);
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

void model::bind_skeleton(pointer<spatial> root)
{
    m_skeleton_root = root;

    m_bones.clear();
    m_bone_connections.clear();
    m_bone_matrices_dirty = true;
    m_bound_dirty         = true;

    shared_ptr<mesh> m = get_mesh();
    if (!m)
        return;

    const auto &bones = m->get_bones();
    m_bones.resize(bones.size(), nullptr);

    for (size_t i = 0; i < bones.size(); ++i)
    {
        const auto &info = bones[i];
        object *obj      = root->get_object_by_name(info.name.c_str());
        spatial *bone    = c_dynamic_cast<spatial>(obj);
        if (bone)
        {
            m_bones[i] = bone;
            m_bone_connections.push_back(bone->on_dirty.connect(
                delegate<void()>::from_method<model, &model::on_bone_dirty>(
                    this)));

            spatial *current = bone;
            while (current)
            {
                current->add_tag("bone");
                if (current == m_skeleton_root.get())
                    break;
                current = current->parent();
            }
        }
        else
            printf("Bone %s not found\n", info.name.c_str());
    }
}

const vector<mat4<real>> &model::get_bone_matrices()
{
    if (!m_bone_matrices_dirty)
        return m_bone_matrices;

    m_bone_matrices_dirty = false;
    shared_ptr<mesh> m    = get_mesh();
    if (!m || m_bones.empty())
    {
        m_bone_matrices.clear();
        return m_bone_matrices;
    }

    const auto &bone_infos = m->get_bones();
    if (bone_infos.empty())
    {
        m_bone_matrices.clear();
        return m_bone_matrices;
    }

    m_bone_matrices.resize(bone_infos.size());

    transformation inv_model;
    get_world_transform().inverse(inv_model);
    mat4<real> inv_model_mat = mat4_translation(inv_model.translate()) *
                               mat4_from_quat(inv_model.rotate()) *
                               mat4_scaling(inv_model.scale());

    size_t bone_count = min(m_bones.size(), bone_infos.size());

    for (size_t i = 0; i < bone_count; ++i)
    {
        spatial *bone = m_bones[i].get();
        if (bone)
        {
            transformation bone_world = bone->get_world_transform();
            mat4<real> m_bone_world = mat4_translation(bone_world.translate()) *
                                      mat4_from_quat(bone_world.rotate()) *
                                      mat4_scaling(bone_world.scale());

            m_bone_matrices[i] = inv_model_mat * m_bone_world *
                                 (mat4<real>)bone_infos[i].offset_transform;
        }
        else
        {
            m_bone_matrices[i] = mat4<real>::identity();
        }
    }

    return m_bone_matrices;
}

void model::on_transform_changed()
{
    spatial::on_transform_changed();
    m_bound_dirty         = true;
    m_bone_matrices_dirty = true;
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
        const auto &bone_matrices = get_bone_matrices();
        for (uint16_t i = 0; i < m->get_vertex_count(); ++i)
        {
            m->get_skinned_position(i, bone_matrices, points[i]);
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
    bool needs_update = false;
    if (!m_model_bound && !m_mesh.path().empty())
        needs_update = true;
    if (!m_bones.empty() && m_bone_matrices_dirty)
        needs_update = true;

    if (needs_update)
    {
        update_model_bound();
        m_bound_dirty = true;
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

    if (m_skeleton_root)
    {
        tinyxml2::XMLElement *skeleton_element =
            element.InsertNewChildElement("skeleton");
        serializer.write_object(*skeleton_element, m_skeleton_root.get());
    }
}

void model::save(serializer &stream) const
{
    spatial::save(stream);
    stream.write(m_mesh);
    stream.write(m_material);
    stream.write((object *)m_skeleton_root.get());
}

void model::load_xml(xml_serializer &serializer, tinyxml2::XMLElement &element)
{
    spatial::load_xml(serializer, element);

    xml_serializer::read_resource_ref(element, m_mesh);
    m_mesh.set_manager(serializer.get_manager());

    xml_serializer::read_resource_ref(element, m_material, "material");
    m_material.set_manager(serializer.get_manager());

    update_model_bound();

    auto skeleton_element = element.FirstChildElement("skeleton");
    if (skeleton_element)
    {
        pointer<object> skeleton = serializer.read_object(*skeleton_element);
        m_skeleton_root          = c_dynamic_cast<spatial>(skeleton.get());
        assert(m_skeleton_root);
    }
}

void model::link(xml_serializer &serializer, tinyxml2::XMLElement &element)
{
    spatial::link(serializer, element);
    if (m_skeleton_root)
    {
        m_skeleton_root->link(serializer, element);
        bind_skeleton(m_skeleton_root);
    }
}

void model::load(serializer &stream, serializer_link *link)
{
    spatial::load(stream, link);
    stream.read(m_mesh);
    m_mesh.set_manager(stream.get_manager());

    stream.read(m_material);
    m_material.set_manager(stream.get_manager());

    update_model_bound();

    object *skeleton_obj = nullptr;
    stream.read(skeleton_obj);
    link->add_child_id(skeleton_obj);
}

void model::link(serializer &serializer, serializer_link *link)
{
    spatial::link(serializer, link);

    object *old_skeleton = link->get_next_child_id();
    if (old_skeleton)
    {
        auto skeleton   = serializer.get_from_map(old_skeleton);
        m_skeleton_root = c_dynamic_cast<spatial>(skeleton);
        bind_skeleton(m_skeleton_root);
    }
}

} // namespace zabato
