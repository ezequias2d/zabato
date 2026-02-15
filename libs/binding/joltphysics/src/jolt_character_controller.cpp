
#include <zabato/jolt/jolt_character_controller.hpp>
#include <zabato/jolt_helper.hpp>
#include <zabato/physics/physics_debug_draw.hpp>
#include <zabato/physics/types.hpp>
#include <zabato/spatial.hpp>
#include <zabato/world.hpp>

namespace zabato::physics::jolt
{

class CharBroadPhaseLayerFilter : public JPH::BroadPhaseLayerFilter
{
public:
    CharBroadPhaseLayerFilter(
        const JPH::ObjectVsBroadPhaseLayerFilter &inInterface,
        JPH::ObjectLayer inLayer)
        : mInterface(inInterface), mLayer(inLayer)
    {
    }

    virtual bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override
    {
        return mInterface.ShouldCollide(mLayer, inLayer);
    }

private:
    const JPH::ObjectVsBroadPhaseLayerFilter &mInterface;
    JPH::ObjectLayer mLayer;
};

class CharObjectLayerFilter : public JPH::ObjectLayerFilter
{
public:
    CharObjectLayerFilter(const JPH::ObjectLayerPairFilter &inInterface,
                          JPH::ObjectLayer inLayer)
        : mInterface(inInterface), mLayer(inLayer)
    {
    }

    virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override
    {
        return mInterface.ShouldCollide(mLayer, inLayer);
    }

private:
    const JPH::ObjectLayerPairFilter &mInterface;
    JPH::ObjectLayer mLayer;
};

const rtti
    jolt_character_controller::TYPE("zabato.jolt.jolt_character_controller",
                                    &character_controller::TYPE,
                                    jolt_character_controller::reflect);

void jolt_character_controller::reflect(reflection &r)
{
    character_controller::reflect(r);
}

jolt_character_controller::jolt_character_controller()
    : m_character(nullptr), m_world(nullptr)
{
}

jolt_character_controller::jolt_character_controller(
    JPH::Ref<JPH::CharacterVirtual> character,
    jolt_physics_world *w,
    const character_creation_config &cfg)
    : m_character(character), m_world(w), m_config(cfg)
{
}

jolt_character_controller::~jolt_character_controller()
{
    if (m_world)
    {
        m_world->unregister_character(this);
    }
    m_character = nullptr;
}

void jolt_character_controller::initialize_with_world(jolt_physics_world *w)
{
    m_world = w;
    if (m_config.shape && m_world)
    {
        JPH::Ref<JPH::Shape> shape =
            create_jolt_shape(m_console, m_config.shape);
        if (shape)
        {
            JPH::CharacterVirtualSettings c_settings;
            c_settings.mShape = shape;
            c_settings.mMass  = static_cast<float>(m_config.mass);
            c_settings.mMaxSlopeAngle =
                static_cast<float>(m_config.max_slope_angle);
            c_settings.mUp = to_jolt(m_config.up);
            c_settings.mSupportingVolume =
                JPH::Plane(to_jolt(m_config.supporting_volume.normal),
                           static_cast<float>(m_config.supporting_volume.d));
            c_settings.mEnhancedInternalEdgeRemoval =
                m_config.enhanced_internal_edge_removal;
            c_settings.mMaxStrength = static_cast<float>(m_config.max_strength);
            c_settings.mShapeOffset = to_jolt(m_config.shape_offset);
            c_settings.mBackFaceMode = (JPH::EBackFaceMode)m_config.back_face;
            c_settings.mPredictiveContactDistance =
                static_cast<float>(m_config.predictive_contact_distance);
            c_settings.mMaxCollisionIterations =
                m_config.max_collision_iterations;
            c_settings.mMaxConstraintIterations =
                m_config.max_constraint_iterations;
            c_settings.mMinTimeRemaining =
                static_cast<float>(m_config.min_time_remaining);
            c_settings.mCollisionTolerance =
                static_cast<float>(m_config.collision_tolerance);
            c_settings.mCharacterPadding =
                static_cast<float>(m_config.character_padding);
            c_settings.mMaxNumHits = m_config.max_num_hits;
            c_settings.mHitReductionCosMaxAngle =
                static_cast<float>(m_config.hit_reduction_cos_max_angle);
            c_settings.mPenetrationRecoverySpeed =
                static_cast<float>(m_config.penetration_recovery_speed);

            if (m_config.inner_body_shape)
            {
                c_settings.mInnerBodyShape =
                    create_jolt_shape(m_console, m_config.inner_body_shape);
            }

            vec3<real> pos = vec3<real>(0);
            quat<real> rot = quat<real>();

            // Prefer current object transform if available
            if (m_object)
            {
                spatial *s = c_dynamic_cast<spatial>(m_object);
                if (s)
                {
                    transformation t = s->get_world_transform();
                    pos              = t.translate();
                    rot              = t.rotate();
                }
            }

            m_character = new JPH::CharacterVirtual(
                &c_settings, to_jolt(pos), to_jolt(rot), m_world->get_system());

            m_world->register_character(this);
        }
    }
}

void jolt_character_controller::start()
{
    character_controller::start();

    if (m_character == nullptr)
    {
        world *w = m_object->get_world();
        if (w && w->get_physics())
        {
            if (w->get_physics()->type().is_derived(jolt_physics_world::TYPE))
            {
                initialize_with_world(
                    static_cast<jolt_physics_world *>(w->get_physics()));
            }
        }
    }
}

void jolt_character_controller::set_position(const vec3<real> &pos)
{
    if (m_character)
    {
        m_character->SetPosition(to_jolt(pos));
    }
}

vec3<real> jolt_character_controller::get_position() const
{
    if (m_character)
    {
        return from_jolt(m_character->GetPosition());
    }
    return m_object ? c_dynamic_cast<spatial>(m_object)
                          ->get_world_transform()
                          .translate()
                    : vec3<real>(0);
}

void jolt_character_controller::set_rotation(const quat<real> &rot)
{
    if (m_character)
        m_character->SetRotation(to_jolt(rot));
}

quat<real> jolt_character_controller::get_rotation() const
{
    if (m_character)
        return from_jolt(m_character->GetRotation());
    return m_object ? c_dynamic_cast<spatial>(m_object)
                          ->get_world_transform()
                          .rotate()
                    : quat<real>();
}

void jolt_character_controller::set_linear_velocity(const vec3<real> &velocity)
{
    if (m_character)
        m_character->SetLinearVelocity(to_jolt(velocity));
}

vec3<real> jolt_character_controller::get_linear_velocity() const
{
    if (m_character)
        return from_jolt(m_character->GetLinearVelocity());
    return vec3<real>(0);
}

void jolt_character_controller::activate()
{
    // TODO:
}

void jolt_character_controller::deactivate()
{
    // TODO:
}

bool jolt_character_controller::is_active() const
{
    // TODO: ?
    return m_character != nullptr;
}

bool jolt_character_controller::is_on_ground() const
{
    if (m_character)
        return m_character->GetGroundState() ==
               JPH::CharacterVirtual::EGroundState::OnGround;
    return false;
}

void jolt_character_controller::pre_physics_update(float dt)
{
    if (m_character && m_world)
    {
        JPH::Vec3 gravity =
            to_jolt(m_world->get_gravity() * m_config.gravity_factor);

        CharBroadPhaseLayerFilter bp_filter(
            m_world->get_object_vs_bp_layer_filter(), Layers::MOVING);
        CharObjectLayerFilter obj_filter(
            m_world->get_object_layer_pair_filter(), Layers::MOVING);
        JPH::BodyFilter body_filter;
        JPH::ShapeFilter shape_filter;

        m_character->Update(dt,
                            gravity,
                            bp_filter,
                            obj_filter,
                            body_filter,
                            shape_filter,
                            m_world->get_temp_allocator());
    }
}

void jolt_character_controller::on_draw_gizmos(gpu &g, bool selected)
{
    transformation t;
    t.set_translate(get_position());
    t.set_rotate(get_rotation());

    color c = selected ? color::cyan() : color::blue();

    const auto &cfg = get_config();
    if (cfg.shape)
    {
        draw_shape_config(g, cfg.shape, t, c);
    }
}

void jolt_character_controller::set_config(
    const character_creation_config &config)
{
    m_config = config;
}

character_creation_config jolt_character_controller::get_config() const
{
    return m_config;
}

} // namespace zabato::physics::jolt
