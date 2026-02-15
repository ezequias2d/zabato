
#include <zabato/base_object.hpp>
#include <zabato/error.hpp>
#include <zabato/jolt_physics_world.hpp>
#include <zabato/physics/physics_controller.hpp>

#include <zabato/jolt/jolt_character_controller.hpp>
#include <zabato/jolt/jolt_rigid_body_controller.hpp>
#include <zabato/jolt/jolt_vehicle_controller.hpp>

namespace zabato::physics::jolt
{

class jolt_physics_world::BPLayerInterfaceImpl final
    : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        mObjectToBroadPhase[Layers::NON_MOVING] =
            JPH::BroadPhaseLayer(Layers::NON_MOVING);
        mObjectToBroadPhase[Layers::MOVING] =
            JPH::BroadPhaseLayer(Layers::MOVING);
    }

    virtual JPH::uint GetNumBroadPhaseLayers() const override
    {
        return Layers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer
    GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char *
    GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::ObjectLayer)inLayer.GetValue())
        {
        case Layers::NON_MOVING:
            return "NON_MOVING";
        case Layers::MOVING:
            return "MOVING";
        default:
            JPH_ASSERT(false);
            return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class jolt_physics_world::ObjectVsBroadPhaseLayerFilterImpl
    : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1,
                               JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case Layers::NON_MOVING:
            return inLayer2 == JPH::BroadPhaseLayer(Layers::MOVING);
        case Layers::MOVING:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

class jolt_physics_world::ObjectLayerPairFilterImpl
    : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1,
                               JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case Layers::NON_MOVING:
            // Non moving only collides with moving
            return inObject2 == Layers::MOVING;
        case Layers::MOVING:
            // Moving collides with everything
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

static void TraceImpl(const char *inFMT, ...)
{
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    report(report_type::trace, buffer);
}

#ifdef JPH_ENABLE_ASSERTS
static bool AssertFailedImpl(const char *inExpression,
                             const char *inMessage,
                             const char *inFile,
                             JPH::uint inLine)
{
    report(report_type::error,
           "%s:%d: (%s) %s",
           inFile,
           inLine,
           inExpression,
           inMessage ? inMessage : "");
    return true;
};
#endif

const rtti jolt_physics_world::TYPE("zabato.jolt_physics_world",
                                    &physics::physics_world::TYPE,
                                    jolt_physics_world::reflect);

void jolt_physics_world::reflect(reflection &r)
{
    physics::physics_world::reflect(r);
}

// Temporary allocator size
static const size_t cTempAllocatorSize = 10 * 1024 * 1024; // 10 MiB

jolt_physics_world::jolt_physics_world()
{
    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
    JPH::AssertFailed = AssertFailedImpl;
#endif

    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    m_temp_allocator = new JPH::TempAllocatorImpl(cTempAllocatorSize);
    m_job_system =
        new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs,
                                     JPH::cMaxPhysicsBarriers,
                                     std::thread::hardware_concurrency() - 1);

    m_bp_layer_interface        = new BPLayerInterfaceImpl();
    m_object_vs_bp_layer_filter = new ObjectVsBroadPhaseLayerFilterImpl();
    m_object_layer_pair_filter  = new ObjectLayerPairFilterImpl();

    const JPH::uint cMaxBodies             = 1024;
    const JPH::uint cNumBodyMutexes        = 0;
    const JPH::uint cMaxBodyPairs          = 1024;
    const JPH::uint cMaxContactConstraints = 1024;

    m_physics_system = new JPH::PhysicsSystem();
    m_physics_system->Init(cMaxBodies,
                           cNumBodyMutexes,
                           cMaxBodyPairs,
                           cMaxContactConstraints,
                           *m_bp_layer_interface,
                           *m_object_vs_bp_layer_filter,
                           *m_object_layer_pair_filter);

    m_physics_system->SetBodyActivationListener(this);
    m_physics_system->SetContactListener(this);
}

jolt_physics_world::~jolt_physics_world()
{
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    delete m_physics_system;
    delete m_object_layer_pair_filter;
    delete m_object_vs_bp_layer_filter;
    delete m_bp_layer_interface;
    delete m_job_system;
    delete m_temp_allocator;
}

void jolt_physics_world::update(real dt)
{
    const int cCollisionSteps = 1;
    m_physics_system->Update(static_cast<float>(dt),
                             cCollisionSteps,
                             m_temp_allocator,
                             m_job_system);

    for (auto *c : m_characters)
        c->pre_physics_update((float)dt);
}

void jolt_physics_world::set_gravity(const vec3<real> &g)
{
    if (m_physics_system)
        m_physics_system->SetGravity(to_jolt(g));
}

vec3<real> jolt_physics_world::get_gravity() const
{
    if (m_physics_system)
        return from_jolt(m_physics_system->GetGravity());
    return vec3<real>(0);
}

void jolt_physics_world::add_controller(physics::physics_controller *controller)
{
    if (!controller)
        return;

    if (auto *rb = c_dynamic_cast<jolt_rigid_body_controller>(controller))
    {
        rb->initialize_with_world(this);
    }
    else if (auto *c = c_dynamic_cast<jolt_character_controller>(controller))
    {
        c->initialize_with_world(this);
        register_character(c);
    }
    else if (auto *v = c_dynamic_cast<jolt_vehicle_controller>(controller))
    {
        v->initialize_with_world(this);
        register_vehicle(v);
    }
}

void jolt_physics_world::remove_controller(
    physics::physics_controller *controller)
{
    if (!controller)
        return;

    if (auto *rb = c_dynamic_cast<jolt_rigid_body_controller>(controller))
    {
        rb->destroy_body();
        unregister_rigid_body(rb);
    }
    else if (auto *c = c_dynamic_cast<jolt_character_controller>(controller))
    {
        // c->destroy(); // TODO: Implement destroy on character
        unregister_character(c);
    }
    else if (auto *v = c_dynamic_cast<jolt_vehicle_controller>(controller))
    {
        // v->destroy(); // TODO: Implement destroy on vehicle
        unregister_vehicle(v);
    }
}

bool jolt_physics_world::raycast(const ray3<real> &ray,
                                 physics::physics_raycast_result &out)
{
    JPH::RRayCast j_ray(to_jolt(ray.origin), to_jolt(ray.direction));
    JPH::RayCastResult hit;

    if (m_physics_system->GetNarrowPhaseQuery().CastRay(j_ray, hit))
    {
        out.distance  = (real)hit.mFraction;
        out.hit_point = ray.origin + ray.direction * out.distance;

        JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(),
                               hit.mBodyID);
        if (lock.Succeeded())
        {
            const JPH::Body &body = lock.GetBody();
            out.hit_normal        = from_jolt(body.GetWorldSpaceSurfaceNormal(
                hit.mSubShapeID2, j_ray.GetPointOnRay(hit.mFraction)));

            out.body =
                static_cast<physics_controller *>((void *)body.GetUserData());
            return true;
        }
    }
    return false;
}

pointer<physics::rigid_body_controller> jolt_physics_world::create_rigid_body(
    const physics::rigid_body_creation_config &settings)
{
    auto ctrl = new jolt_rigid_body_controller();
    ctrl->set_config(settings);
    ctrl->initialize_with_world(this);
    return ctrl;
}

pointer<physics::character_controller> jolt_physics_world::create_character(
    const physics::character_creation_config &settings)
{
    auto ctrl = new jolt_character_controller();
    ctrl->set_config(settings);
    ctrl->initialize_with_world(this);
    return ctrl;
}

pointer<physics::vehicle_controller> jolt_physics_world::create_vehicle(
    const physics::vehicle_creation_config &settings)
{
    auto ctrl = new jolt_vehicle_controller();
    ctrl->set_config(settings);
    ctrl->initialize_with_world(this);
    return ctrl;
}

void jolt_physics_world::OnBodyActivated(const JPH::BodyID &inBodyID,
                                         JPH::uint64 inBodyUserData)
{
}

void jolt_physics_world::OnBodyDeactivated(const JPH::BodyID &inBodyID,
                                           JPH::uint64 inBodyUserData)
{
}

JPH::ValidateResult jolt_physics_world::OnContactValidate(
    const JPH::Body &inBody1,
    const JPH::Body &inBody2,
    JPH::RVec3Arg inBaseOffset,
    const JPH::CollideShapeResult &inCollisionResult)
{
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

void jolt_physics_world::OnContactAdded(const JPH::Body &inBody1,
                                        const JPH::Body &inBody2,
                                        const JPH::ContactManifold &inManifold,
                                        JPH::ContactSettings &ioSettings)
{
}

void jolt_physics_world::OnContactPersisted(
    const JPH::Body &inBody1,
    const JPH::Body &inBody2,
    const JPH::ContactManifold &inManifold,
    JPH::ContactSettings &ioSettings)
{
}

void jolt_physics_world::OnContactRemoved(
    const JPH::SubShapeIDPair &inSubShapePair)
{
}

JPH::BodyInterface &jolt_physics_world::get_body_interface() const
{
    return m_physics_system->GetBodyInterface();
}

const JPH::BodyLockInterface &
jolt_physics_world::get_body_lock_interface() const
{
    return m_physics_system->GetBodyLockInterface();
}

JPH::BroadPhaseLayerInterface &
jolt_physics_world::get_bp_layer_interface() const
{
    return *m_bp_layer_interface;
}

JPH::ObjectVsBroadPhaseLayerFilter &
jolt_physics_world::get_object_vs_bp_layer_filter() const
{
    return *m_object_vs_bp_layer_filter;
}

JPH::ObjectLayerPairFilter &
jolt_physics_world::get_object_layer_pair_filter() const
{
    return *m_object_layer_pair_filter;
}

JPH::TempAllocator &jolt_physics_world::get_temp_allocator() const
{
    return *m_temp_allocator;
}

JPH::JobSystem &jolt_physics_world::get_job_system() const
{
    return *m_job_system;
}

void jolt_physics_world::register_character(jolt_character_controller *c)
{
    m_characters.push_back(c);
}

void jolt_physics_world::unregister_character(jolt_character_controller *c)
{
    auto it = std::find(m_characters.begin(), m_characters.end(), c);
    if (it != m_characters.end())
        m_characters.erase(it);
}

void jolt_physics_world::register_rigid_body(jolt_rigid_body_controller *c)
{
    m_rigid_bodies.push_back(c);
}

void jolt_physics_world::unregister_rigid_body(jolt_rigid_body_controller *c)
{
    auto it = std::find(m_rigid_bodies.begin(), m_rigid_bodies.end(), c);
    if (it != m_rigid_bodies.end())
        m_rigid_bodies.erase(it);
}

void jolt_physics_world::register_vehicle(jolt_vehicle_controller *c)
{
    m_vehicles.push_back(c);
}

void jolt_physics_world::unregister_vehicle(jolt_vehicle_controller *c)
{
    auto it = std::find(m_vehicles.begin(), m_vehicles.end(), c);
    if (it != m_vehicles.end())
        m_vehicles.erase(it);
}

} // namespace zabato::physics::jolt