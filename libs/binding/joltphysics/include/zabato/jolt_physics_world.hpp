#pragma once

#include <zabato/jolt_helper.hpp>
#include <zabato/physics/physics_world.hpp>

namespace zabato::physics::jolt
{

class jolt_physics_world : public physics::physics_world,
                           public JPH::BodyActivationListener,
                           public JPH::ContactListener
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    jolt_physics_world();
    virtual ~jolt_physics_world();

    void update(real dt) override;
    void set_gravity(const vec3<real> &g) override;
    vec3<real> get_gravity() const override;

    void add_controller(physics::physics_controller *controller) override;
    void remove_controller(physics::physics_controller *controller) override;

    bool raycast(const ray3<real> &ray,
                 physics::physics_raycast_result &out) override;

    pointer<physics::character_controller> create_character(
        const physics::character_creation_config &settings) override;

    // pointer<physics::vehicle_controller>
    // create_vehicle(const physics::vehicle_creation_config &settings)
    // override;

    pointer<physics::rigid_body_controller> create_rigid_body(
        const physics::rigid_body_creation_config &settings) override;

    // Jolt Callbacks
    void OnBodyActivated(const JPH::BodyID &inBodyID,
                         JPH::uint64 inBodyUserData) override;
    void OnBodyDeactivated(const JPH::BodyID &inBodyID,
                           JPH::uint64 inBodyUserData) override;

    JPH::ValidateResult OnContactValidate(
        const JPH::Body &inBody1,
        const JPH::Body &inBody2,
        JPH::RVec3Arg inBaseOffset,
        const JPH::CollideShapeResult &inCollisionResult) override;

    void OnContactAdded(const JPH::Body &inBody1,
                        const JPH::Body &inBody2,
                        const JPH::ContactManifold &inManifold,
                        JPH::ContactSettings &ioSettings) override;

    void OnContactPersisted(const JPH::Body &inBody1,
                            const JPH::Body &inBody2,
                            const JPH::ContactManifold &inManifold,
                            JPH::ContactSettings &ioSettings) override;

    void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override;

    JPH::PhysicsSystem *get_system() const { return m_physics_system; }
    JPH::BodyInterface &get_body_interface() const;
    const JPH::BodyLockInterface &get_body_lock_interface() const;
    JPH::BroadPhaseLayerInterface &get_bp_layer_interface() const;
    JPH::ObjectVsBroadPhaseLayerFilter &get_object_vs_bp_layer_filter() const;
    JPH::ObjectLayerPairFilter &get_object_layer_pair_filter() const;
    JPH::TempAllocator &get_temp_allocator() const;
    JPH::JobSystem &get_job_system() const;

    void register_character(class jolt_character_controller *c);
    void unregister_character(class jolt_character_controller *c);

    void register_rigid_body(class jolt_rigid_body_controller *c);
    void unregister_rigid_body(class jolt_rigid_body_controller *c);

    // void register_vehicle(class jolt_vehicle_controller *c);
    // void unregister_vehicle(class jolt_vehicle_controller *c);

private:
    JPH::TempAllocatorImpl *m_temp_allocator;
    JPH::JobSystemThreadPool *m_job_system;
    JPH::PhysicsSystem *m_physics_system;

    class BPLayerInterfaceImpl;
    class ObjectVsBroadPhaseLayerFilterImpl;
    class ObjectLayerPairFilterImpl;

    BPLayerInterfaceImpl *m_bp_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl *m_object_vs_bp_layer_filter;
    ObjectLayerPairFilterImpl *m_object_layer_pair_filter;

    std::vector<class jolt_character_controller *> m_characters;
    std::vector<class jolt_rigid_body_controller *> m_rigid_bodies;
    // std::vector<class jolt_vehicle_controller *> m_vehicles;
};

namespace Layers
{
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING     = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}; // namespace Layers

} // namespace zabato::physics::jolt