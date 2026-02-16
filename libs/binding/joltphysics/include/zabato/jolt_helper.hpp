#pragma once

// Jolt includes
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>

#include <zabato/console.hpp>
#include <zabato/math.hpp>
#include <zabato/physics/types.hpp>
#include <zabato/real.hpp>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/TaperedCapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/TaperedCylinderShape.h>
#include <Jolt/Physics/Collision/Shape/TriangleShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>

namespace zabato::physics::jolt
{
inline JPH::Vec3 to_jolt(const vec3<real> &v)
{
    return JPH::Vec3(static_cast<float>(v.x),
                     static_cast<float>(v.y),
                     static_cast<float>(v.z));
}
inline vec3<real> from_jolt(const JPH::Vec3 &v)
{
    return vec3<real>(v.GetX(), v.GetY(), v.GetZ());
}
inline JPH::Quat to_jolt(const quat<real> &q)
{
    return JPH::Quat(static_cast<float>(q.x),
                     static_cast<float>(q.y),
                     static_cast<float>(q.z),
                     static_cast<float>(q.w));
}
inline quat<real> from_jolt(const JPH::Quat &q)
{
    quat<real> r;
    r.x = q.GetX();
    r.y = q.GetY();
    r.z = q.GetZ();
    r.w = q.GetW();
    return r;
}

JPH::Ref<JPH::Shape> create_jolt_shape(console *console,
                                       const physics::shape_config *config);

} // namespace zabato::physics::jolt