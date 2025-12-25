#include <zabato/mesh.hpp>
#include <zabato/model.hpp>
#include <zabato/serializer.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti model::TYPE("zabato.model", &spatial::TYPE);

model::model() : m_model_bound(nullptr), m_world_bound(nullptr) {}

model::~model()
{
    delete m_model_bound;
    delete m_world_bound;
}

void model::set_mesh(const char *path)
{
    m_mesh.set_path(path);
    bind_skeleton();
}

void model::set_resource_manager(resource_manager *mgr)
{
    m_mesh.set_manager(mgr);
}

shared_ptr<mesh> model::get_mesh() const { return m_mesh.get<mesh>(); }

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

void model::set_animator(animator *anim) { add_controller(anim); }

animator *model::get_animator() const
{
    pointer<controller> ctrl = get_controller(animator::TYPE);
    return c_dynamic_cast<animator>(ctrl);
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
            m_model_bound->compute_from_data(
                span<const vec3<real>>(points.data(), points.size()));
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
    }

    m_model_bound->transform_by(get_world_transform(), m_world_bound);

    return m_world_bound;
}

void model::save(serializer &stream) const
{
    spatial::save(stream);
    stream.write(m_mesh);
    // TODO: support saving animator (needs animator serialization support)
}

void model::load(serializer &stream, serializer_link *link)
{
    spatial::load(stream, link);
    stream.read(m_mesh);
    m_mesh.set_manager(stream.get_manager());

    update_model_bound();
    bind_skeleton();
    // TODO: support loading animator
}

void model::save_xml(xml_serializer &serializer,
                     tinyxml2::XMLElement &element) const
{
    spatial::save_xml(serializer, element);
    xml_serializer::write_resource_ref(element, m_mesh);
}

void model::load_xml(xml_serializer &serializer, tinyxml2::XMLElement &element)
{
    spatial::load_xml(serializer, element);
    xml_serializer::read_resource_ref(element, m_mesh);
    // TODO: m_mesh.set_manager(serializer.get_manager());
    update_model_bound();
    bind_skeleton();
}

} // namespace zabato
