
#include <zabato/jolt/jolt_rigid_body_controller.hpp>
#include <zabato/jolt_helper.hpp>
#include <zabato/physics/physics_debug_draw.hpp>
#include <zabato/spatial.hpp>
#include <zabato/world.hpp>

namespace zabato::physics::jolt
{

const rtti
    jolt_rigid_body_controller::TYPE("zabato.jolt.jolt_rigid_body_controller",
                                     &physics::rigid_body_controller::TYPE,
                                     jolt_rigid_body_controller::reflect);

void jolt_rigid_body_controller::reflect(reflection &r)
{
    physics::rigid_body_controller::reflect(r);
}

// Default constructor for serialization
jolt_rigid_body_controller::jolt_rigid_body_controller() : m_world(nullptr) {}

jolt_rigid_body_controller::jolt_rigid_body_controller(
    JPH::BodyID id,
    jolt_physics_world *w,
    const physics::rigid_body_creation_config &cfg)
    : m_body_id(id), m_world(w), m_config(cfg)
{
}

jolt_rigid_body_controller::~jolt_rigid_body_controller() { destroy_body(); }

void jolt_rigid_body_controller::destroy_body()
{
    if (m_world)
    {
        m_world->unregister_rigid_body(this);
    }

    if (!m_body_id.IsInvalid() && m_world)
    {
        JPH::BodyInterface &bi = m_world->get_body_interface();
        bi.RemoveBody(m_body_id);
        bi.DestroyBody(m_body_id);

        char buff[256];
        snprintf(buff,
                 sizeof(buff),
                 "Body destroyed with ID: %d",
                 m_body_id.GetIndex());
        m_console->log_info(buff);
    }
    m_body_id = JPH::BodyID();
}

void jolt_rigid_body_controller::create_body()
{
    if (!m_world)
        return;

    if (m_config.shape)
    {
        JPH::Ref<JPH::Shape> shape =
            create_jolt_shape(m_console, m_config.shape);
        if (shape)
        {
            JPH::EMotionType motion_type = JPH::EMotionType::Dynamic;
            if (m_config.motion_type == physics::physics_motion_type::static_)
                motion_type = JPH::EMotionType::Static;
            else if (m_config.motion_type ==
                     physics::physics_motion_type::kinematic)
                motion_type = JPH::EMotionType::Kinematic;

            JPH::ObjectLayer object_layer = (JPH::ObjectLayer)m_config.layer;
            if (m_config.layer == 0)
                object_layer = Layers::MOVING; // Default if 0

            vec3<real> pos = m_config.position;
            quat<real> rot = m_config.rotation;

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

            JPH::BodyCreationSettings body_settings(
                shape, to_jolt(pos), to_jolt(rot), motion_type, object_layer);

            // Apply extended settings
            body_settings.mAllowedDOFs =
                (JPH::EAllowedDOFs)m_config.allowed_dofs;
            if (m_config.allowed_dofs == physics::physics_allowed_dofs::all)
                body_settings.mAllowedDOFs = JPH::EAllowedDOFs::All;
            else if (m_config.allowed_dofs ==
                     physics::physics_allowed_dofs::plane_2d)
                body_settings.mAllowedDOFs = JPH::EAllowedDOFs::Plane2D;
            else if (m_config.allowed_dofs ==
                     physics::physics_allowed_dofs::rotation_only)
                body_settings.mAllowedDOFs = JPH::EAllowedDOFs::RotationX |
                                             JPH::EAllowedDOFs::RotationY |
                                             JPH::EAllowedDOFs::RotationZ;
            else if (m_config.allowed_dofs ==
                     physics::physics_allowed_dofs::translation_only)
                body_settings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX |
                                             JPH::EAllowedDOFs::TranslationY |
                                             JPH::EAllowedDOFs::TranslationZ;

            body_settings.mAllowDynamicOrKinematic =
                m_config.allow_dynamic_or_kinematic ||
                (motion_type != JPH::EMotionType::Static);
            body_settings.mIsSensor = m_config.is_sensor;
            body_settings.mCollideKinematicVsNonDynamic =
                m_config.collide_kinematic_vs_non_dynamic;
            body_settings.mUseManifoldReduction =
                m_config.use_manifold_reduction;
            body_settings.mApplyGyroscopicForce =
                m_config.apply_gyroscopic_force;
            body_settings.mAllowSleeping = m_config.allow_sleeping;

            body_settings.mFriction       = (float)m_config.friction;
            body_settings.mRestitution    = (float)m_config.restitution;
            body_settings.mLinearDamping  = (float)m_config.linear_damping;
            body_settings.mAngularDamping = (float)m_config.angular_damping;
            body_settings.mMaxLinearVelocity =
                (float)m_config.max_linear_velocity;
            body_settings.mMaxAngularVelocity =
                (float)m_config.max_angular_velocity;
            body_settings.mGravityFactor = (float)m_config.gravity_factor;

            body_settings.mNumVelocityStepsOverride =
                m_config.num_velocity_steps_override;
            body_settings.mNumPositionStepsOverride =
                m_config.num_position_steps_override;

            if (m_config.override_mass_properties ==
                physics::physics_override_mass_properties::calculate_inertia)
            {
                body_settings.mOverrideMassProperties =
                    JPH::EOverrideMassProperties::CalculateInertia;
                body_settings.mMassPropertiesOverride.mMass =
                    (float)m_config.mass_override;
            }
            else if (m_config.override_mass_properties ==
                     physics::physics_override_mass_properties::
                         mass_and_inertia_provided)
            {
                body_settings.mOverrideMassProperties =
                    JPH::EOverrideMassProperties::MassAndInertiaProvided;
                body_settings.mMassPropertiesOverride.mMass =
                    (float)m_config.mass_override;
                body_settings.mOverrideMassProperties =
                    JPH::EOverrideMassProperties::CalculateInertia;
            }
            else
            {
                body_settings.mOverrideMassProperties =
                    JPH::EOverrideMassProperties::CalculateMassAndInertia;
            }

            body_settings.mInertiaMultiplier =
                (float)m_config.inertia_multiplier;

            body_settings.mUserData = (uint64_t)this;

            JPH::Body *body =
                m_world->get_body_interface().CreateBody(body_settings);
            if (body)
            {
                m_body_id       = body->GetID();
                auto activation = (motion_type == JPH::EMotionType::Static)
                                      ? JPH::EActivation::DontActivate
                                      : JPH::EActivation::Activate;
                m_world->get_body_interface().AddBody(m_body_id, activation);
                m_world->register_rigid_body(this);

                char buff[256];
                snprintf(buff,
                         256,
                         "Body created with ID: %d, Type: %d, Mass: %.2f",
                         m_body_id.GetIndex(),
                         (int)motion_type,
                         (float)body_settings.mMassPropertiesOverride.mMass);
                m_console->log_info(buff);
            }
            else
            {
                m_console->log_error("Failed to create Jolt body!");
            }
        }
    }
}

void jolt_rigid_body_controller::initialize_with_world(jolt_physics_world *w)
{
    m_world = w;
    create_body();
    m_world->register_rigid_body(this);
}

void jolt_rigid_body_controller::start()
{
    physics::rigid_body_controller::start();

    // If not initialized (no valid body ID), try to initialize now
    if (m_body_id.IsInvalid())
    {
        // Find world
        spatial *s = c_dynamic_cast<spatial>(m_object);
        world *w   = nullptr;
        while (s)
        {
            if (s->type().is_derived(world::TYPE))
            {
                w = static_cast<world *>(s);
                break;
            }
            s = c_dynamic_cast<spatial>(s->parent());
        }

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

void jolt_rigid_body_controller::set_position(const vec3<real> &pos)
{
    if (m_body_id.IsInvalid() || !m_world)
        return;
    m_world->get_body_interface().SetPosition(
        m_body_id, to_jolt(pos), JPH::EActivation::DontActivate);
}

vec3<real> jolt_rigid_body_controller::get_position() const
{
    if (m_body_id.IsInvalid() || !m_world)
        return m_object ? c_dynamic_cast<spatial>(m_object)
                              ->get_world_transform()
                              .translate()
                        : vec3<real>(0);

    return from_jolt(m_world->get_body_interface().GetPosition(m_body_id));
}

void jolt_rigid_body_controller::set_rotation(const quat<real> &rot)
{
    if (m_body_id.IsInvalid() || !m_world)
        return;
    m_world->get_body_interface().SetRotation(
        m_body_id, to_jolt(rot), JPH::EActivation::DontActivate);
}

quat<real> jolt_rigid_body_controller::get_rotation() const
{
    if (m_body_id.IsInvalid() || !m_world)
        return m_object ? c_dynamic_cast<spatial>(m_object)
                              ->get_world_transform()
                              .rotate()
                        : quat<real>();

    return from_jolt(m_world->get_body_interface().GetRotation(m_body_id));
}

void jolt_rigid_body_controller::on_draw_gizmos(gpu &g, bool selected)
{
    transformation t;
    t.set_translate(get_position());
    t.set_rotate(get_rotation());

    zabato::color c = selected ? zabato::color::green() : zabato::color::gray();

    if (m_config.shape)
    {
        draw_shape_config(g, m_config.shape, t, c);
    }
}

void jolt_rigid_body_controller::set_config(
    const physics::rigid_body_creation_config &config)
{
    m_config = config;
    if (m_world)
    {
        m_console->log_info(
            "set_motion_type: Recreating body for new motion type.");
        destroy_body();
        create_body();
    }
}

physics::rigid_body_creation_config
jolt_rigid_body_controller::get_config() const
{
    return m_config;
}

void jolt_rigid_body_controller::set_linear_velocity(const vec3<real> &velocity)
{
    if (m_body_id.IsInvalid() || !m_world)
        return;
    m_world->get_body_interface().SetLinearVelocity(m_body_id,
                                                    to_jolt(velocity));
}

vec3<real> jolt_rigid_body_controller::get_linear_velocity() const
{
    if (m_body_id.IsInvalid() || !m_world)
        return vec3<real>(0);
    return from_jolt(
        m_world->get_body_interface().GetLinearVelocity(m_body_id));
}

void jolt_rigid_body_controller::set_angular_velocity(
    const vec3<real> &velocity)
{
    if (m_body_id.IsInvalid() || !m_world)
        return;
    m_world->get_body_interface().SetAngularVelocity(m_body_id,
                                                     to_jolt(velocity));
}

vec3<real> jolt_rigid_body_controller::get_angular_velocity() const
{
    if (m_body_id.IsInvalid() || !m_world)
        return vec3<real>(0);
    return from_jolt(
        m_world->get_body_interface().GetAngularVelocity(m_body_id));
}

void jolt_rigid_body_controller::add_force(const vec3<real> &force)
{
    if (m_body_id.IsInvalid() || !m_world)
        return;
    m_world->get_body_interface().AddForce(m_body_id, to_jolt(force));
}

void jolt_rigid_body_controller::add_force(const vec3<real> &force,
                                           const vec3<real> &point)
{
    if (m_body_id.IsInvalid() || !m_world)
        return;
    m_world->get_body_interface().AddForce(
        m_body_id, to_jolt(force), to_jolt(point));
}

void jolt_rigid_body_controller::add_torque(const vec3<real> &torque)
{
    if (m_body_id.IsInvalid() || !m_world)
        return;
    m_world->get_body_interface().AddTorque(m_body_id, to_jolt(torque));
}

void jolt_rigid_body_controller::add_impulse(const vec3<real> &impulse)
{
    if (m_body_id.IsInvalid() || !m_world)
        return;
    m_world->get_body_interface().AddImpulse(m_body_id, to_jolt(impulse));
}

void jolt_rigid_body_controller::add_impulse(const vec3<real> &impulse,
                                             const vec3<real> &point)
{
    if (m_body_id.IsInvalid() || !m_world)
        return;
    m_world->get_body_interface().AddImpulse(
        m_body_id, to_jolt(impulse), to_jolt(point));
}

void jolt_rigid_body_controller::add_angular_impulse(const vec3<real> &impulse)
{
    if (m_body_id.IsInvalid() || !m_world)
        return;
    m_world->get_body_interface().AddAngularImpulse(m_body_id,
                                                    to_jolt(impulse));
}

void jolt_rigid_body_controller::activate()
{
    if (m_body_id.IsInvalid() || !m_world)
        return;
    m_world->get_body_interface().ActivateBody(m_body_id);
}

void jolt_rigid_body_controller::deactivate()
{
    if (m_body_id.IsInvalid() || !m_world)
        return;
    m_world->get_body_interface().DeactivateBody(m_body_id);
}

bool jolt_rigid_body_controller::is_active() const
{
    if (m_body_id.IsInvalid() || !m_world)
        return false;
    return m_world->get_body_interface().IsActive(m_body_id);
}

} // namespace zabato::physics::jolt
