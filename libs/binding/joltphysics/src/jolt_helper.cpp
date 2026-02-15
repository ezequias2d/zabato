#include <zabato/jolt_helper.hpp>
#include <zabato/mesh.hpp>
#include <zabato/physics/types.hpp>

namespace zabato::physics::jolt
{

JPH::Ref<JPH::Shape> create_jolt_shape(console *console,
                                       const shape_config *config)
{
    if (!config)
        return nullptr;

    const auto &type = config->type();

    if (type == physics::box_shape_config::TYPE)
    {
        const auto *box =
            static_cast<const physics::box_shape_config *>(config);
        JPH::BoxShapeSettings settings(to_jolt(box->half_extent),
                                       static_cast<float>(box->convex_radius));
        settings.mDensity = static_cast<float>(box->density);
        return settings.Create().Get();
    }
    else if (type == physics::sphere_shape_config::TYPE)
    {
        const auto *sphere =
            static_cast<const physics::sphere_shape_config *>(config);
        JPH::SphereShapeSettings settings(static_cast<float>(sphere->radius));
        settings.mDensity = static_cast<float>(sphere->density);
        return settings.Create().Get();
    }
    else if (type == physics::capsule_shape_config::TYPE)
    {
        const auto *capsule =
            static_cast<const physics::capsule_shape_config *>(config);
        JPH::CapsuleShapeSettings settings(
            static_cast<float>(capsule->half_height),
            static_cast<float>(capsule->radius));
        settings.mDensity = static_cast<float>(capsule->density);
        return settings.Create().Get();
    }
    else if (type == physics::cylinder_shape_config::TYPE)
    {
        const auto *cylinder =
            static_cast<const physics::cylinder_shape_config *>(config);
        JPH::CylinderShapeSettings settings(
            static_cast<float>(cylinder->half_height),
            static_cast<float>(cylinder->radius),
            static_cast<float>(cylinder->convex_radius));
        settings.mDensity = static_cast<float>(cylinder->density);
        return settings.Create().Get();
    }
    else if (type == physics::convex_hull_shape_config::TYPE)
    {
        const auto *hull =
            static_cast<const physics::convex_hull_shape_config *>(config);

        auto m = hull->mesh.get<mesh>();
        if (!m)
        {
            char buf[256];
            snprintf(buf,
                     sizeof(buf),
                     "fail to load mesh '%s' of convex_hull_shape_config.",
                     hull->mesh.c_path());
            if (console)
                console->log_error(buf);
            else
                report(report_type::error, buf);
            return nullptr;
        }

        JPH::Array<JPH::Vec3> points;
        const uint16_t vertex_count = m->get_vertex_count();
        points.reserve(vertex_count);
        for (uint16_t i = 0; i < vertex_count; ++i)
        {
            vec3<real> pos = {};
            m->get_position(i, pos);
            points.push_back(to_jolt(pos));
        }

        JPH::ConvexHullShapeSettings settings(
            points, static_cast<float>(hull->max_convex_radius));
        settings.mDensity = static_cast<float>(hull->density);
        settings.mMaxErrorConvexRadius =
            static_cast<float>(hull->max_error_convex_radius);
        settings.mHullTolerance = static_cast<float>(hull->hull_tolerance);
        return settings.Create().Get();
    }
    else if (type == physics::tapered_capsule_shape_config::TYPE)
    {
        const auto *tapered =
            static_cast<const physics::tapered_capsule_shape_config *>(config);
        JPH::TaperedCapsuleShapeSettings settings(
            static_cast<float>(tapered->half_height),
            static_cast<float>(tapered->top_radius),
            static_cast<float>(tapered->bottom_radius));
        settings.mDensity = static_cast<float>(tapered->density);
        return settings.Create().Get();
    }
    else if (type == physics::tapered_cylinder_shape_config::TYPE)
    {
        const auto *tapered =
            static_cast<const physics::tapered_cylinder_shape_config *>(config);
        JPH::TaperedCylinderShapeSettings settings(
            static_cast<float>(tapered->half_height),
            static_cast<float>(tapered->top_radius),
            static_cast<float>(tapered->bottom_radius),
            static_cast<float>(tapered->convex_radius));
        settings.mDensity = static_cast<float>(tapered->density);
        return settings.Create().Get();
    }
    else if (type == physics::triangle_shape_config::TYPE)
    {
        const auto *triangle =
            static_cast<const physics::triangle_shape_config *>(config);
        JPH::TriangleShapeSettings settings(to_jolt(triangle->points[0]),
                                            to_jolt(triangle->points[1]),
                                            to_jolt(triangle->points[2]),
                                            0.0f);
        settings.mDensity = static_cast<float>(triangle->density);
        return settings.Create().Get();
    }

    // TODO: Implement other shapes
    return nullptr;
}

} // namespace zabato::physics::jolt
