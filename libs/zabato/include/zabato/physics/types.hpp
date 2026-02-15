#pragma once

#include <zabato/base_object.hpp>
#include <zabato/math.hpp>
#include <zabato/physics/physics_controller.hpp>
#include <zabato/pointer.hpp>
#include <zabato/real.hpp>
#include <zabato/rtti.hpp>
#include <zabato/utils.hpp>

namespace zabato::physics
{

enum class physics_body_type
{
    static_body,
    dynamic_body,
    kinematic_body,
};

enum class physics_motion_quality
{
    discrete,
    continuous,
};

struct physics_raycast_result
{
    pointer<physics_controller> body;
    vec3<real> hit_point;
    vec3<real> hit_normal;
    real distance;
};

struct shape_config : public base_object
{
    static const rtti TYPE;
    virtual const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    virtual bool operator==(const shape_config &other) const
    {
        return type() == other.type();
    }

    virtual bool operator!=(const shape_config &other) const
    {
        return !(*this == other);
    }

    virtual void save(serializer &stream) const;
    virtual void load(serializer &stream, serializer_link *link);
    virtual void save_xml(xml_serializer &stream,
                          tinyxml2::XMLElement &element) const;
    virtual void load_xml(xml_serializer &stream,
                          tinyxml2::XMLElement &element);
};

struct convex_shape_config : public shape_config
{
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    /**
     * @brief Uniform density of the interior of the convex object (kg / m^3)
     */
    real density = 1000.0;

    virtual bool operator==(const shape_config &other) const override
    {
        if (type() != other.type())
            return false;
        const auto *other_convex =
            static_cast<const convex_shape_config *>(&other);
        return density == other_convex->density;
    }

    virtual bool operator!=(const shape_config &other) const override
    {
        return !(*this == other);
    }

    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    void save_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) const override;
    void load_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) override;
};

struct box_shape_config : public convex_shape_config
{
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    /** @brief Half extents of the box (including convex radius) */
    vec3<real> half_extent = vec3<real>(0.5);

    /** @brief Convex radius of the box. */
    real convex_radius = 0.0;

    virtual bool operator==(const shape_config &other) const override
    {
        if (type() != other.type())
            return false;
        const auto *other_box = static_cast<const box_shape_config *>(&other);
        return density == other_box->density &&
               half_extent == other_box->half_extent &&
               convex_radius == other_box->convex_radius;
    }

    virtual bool operator!=(const shape_config &other) const override
    {
        return !(*this == other);
    }

    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    void save_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) const override;
    void load_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) override;
};

struct capsule_shape_config : public convex_shape_config
{
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    /** @brief Radius of the capsule (including convex radius) */
    real radius = 0.5;

    /** @brief Half height of the capsule (including convex radius) */
    real half_height = 0.5;

    virtual bool operator==(const shape_config &other) const override
    {
        if (type() != other.type())
            return false;
        const auto *other_capsule =
            static_cast<const capsule_shape_config *>(&other);
        return density == other_capsule->density &&
               radius == other_capsule->radius &&
               half_height == other_capsule->half_height;
    }

    virtual bool operator!=(const shape_config &other) const override
    {
        return !(*this == other);
    }

    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    void save_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) const override;
    void load_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) override;
};

struct convex_hull_shape_config : public convex_shape_config
{
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    /** @brief Points of the convex hull */
    resource_ref mesh;

    /** @brief Convex radius of the convex hull */
    real max_convex_radius = 0.0;

    /**
     * @brief Maximum distance between the shrunk hull + convex radius and the
     * actual hull.
     */
    real max_error_convex_radius = 0.05;

    /**
     * @brief Points are allowed this far outside of the hull.
     */
    real hull_tolerance = 1.0e-3;

    virtual bool operator==(const shape_config &other) const override
    {
        if (type() != other.type())
            return false;
        const auto *other_hull =
            static_cast<const convex_hull_shape_config *>(&other);
        return density == other_hull->density &&
               max_convex_radius == other_hull->max_convex_radius &&
               max_error_convex_radius == other_hull->max_error_convex_radius &&
               hull_tolerance == other_hull->hull_tolerance &&
               mesh == other_hull->mesh;
    }

    virtual bool operator!=(const shape_config &other) const override
    {
        return !(*this == other);
    }

    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    void save_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) const override;
    void load_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) override;
};

struct cylinder_shape_config : public convex_shape_config
{
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    /** @brief Radius of the cylinder (including convex radius) */
    real radius = 0.5;

    /** @brief Half height of the cylinder (including convex radius) */
    real half_height = 0.5;

    /** @brief Convex radius of the cylinder */
    real convex_radius = 0.0;

    virtual bool operator==(const shape_config &other) const override
    {
        if (type() != other.type())
            return false;
        const auto *other_cylinder =
            static_cast<const cylinder_shape_config *>(&other);
        return density == other_cylinder->density &&
               radius == other_cylinder->radius &&
               half_height == other_cylinder->half_height &&
               convex_radius == other_cylinder->convex_radius;
    }

    virtual bool operator!=(const shape_config &other) const override
    {
        return !(*this == other);
    }

    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    void save_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) const override;
    void load_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) override;
};

struct sphere_shape_config : public convex_shape_config
{
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    /** @brief Radius of the sphere (including convex radius) */
    real radius = 0.5;

    virtual bool operator==(const shape_config &other) const override
    {
        if (type() != other.type())
            return false;
        const auto *other_sphere =
            static_cast<const sphere_shape_config *>(&other);
        return density == other_sphere->density &&
               radius == other_sphere->radius;
    }

    virtual bool operator!=(const shape_config &other) const override
    {
        return !(*this == other);
    }

    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    void save_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) const override;
    void load_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) override;
};

struct tapered_capsule_shape_config : public convex_shape_config
{
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    /** @brief Radius of the tapered capasule (including convex radius) */
    real top_radius = 0.5;

    /** @brief Radius of the tapered capasule (including convex radius) */
    real bottom_radius = 0.5;

    /** @brief Half height of the tapered capasule (including convex radius) */
    real half_height = 0.5;

    virtual bool operator==(const shape_config &other) const override
    {
        if (type() != other.type())
            return false;
        const auto *other_tapered_capsule =
            static_cast<const tapered_capsule_shape_config *>(&other);
        return density == other_tapered_capsule->density &&
               top_radius == other_tapered_capsule->top_radius &&
               bottom_radius == other_tapered_capsule->bottom_radius &&
               half_height == other_tapered_capsule->half_height;
    }

    virtual bool operator!=(const shape_config &other) const override
    {
        return !(*this == other);
    }

    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    void save_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) const override;
    void load_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) override;
};

struct tapered_cylinder_shape_config : public convex_shape_config
{
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    /** @brief Radius of the tapered cylinder (including convex radius) */
    real top_radius = 0.5;

    /** @brief Radius of the tapered cylinder (including convex radius) */
    real bottom_radius = 0.5;

    /** @brief Half height of the tapered cylinder (including convex radius) */
    real half_height = 0.5;

    /** @brief Convex radius of the tapered cylinder */
    real convex_radius = 0.0;

    virtual bool operator==(const shape_config &other) const override
    {
        if (type() != other.type())
            return false;
        const auto *other_tapered_cylinder =
            static_cast<const tapered_cylinder_shape_config *>(&other);
        return density == other_tapered_cylinder->density &&
               top_radius == other_tapered_cylinder->top_radius &&
               bottom_radius == other_tapered_cylinder->bottom_radius &&
               half_height == other_tapered_cylinder->half_height &&
               convex_radius == other_tapered_cylinder->convex_radius;
    }

    virtual bool operator!=(const shape_config &other) const override
    {
        return !(*this == other);
    }

    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    void save_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) const override;
    void load_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) override;
};

struct triangle_shape_config : public convex_shape_config
{
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    /** @brief Points of the triangle */
    vec3<real> points[3];

    virtual bool operator==(const shape_config &other) const override
    {
        if (type() != other.type())
            return false;
        const auto *other_triangle =
            static_cast<const triangle_shape_config *>(&other);
        return density == other_triangle->density &&
               points[0] == other_triangle->points[0] &&
               points[1] == other_triangle->points[1] &&
               points[2] == other_triangle->points[2];
    }

    virtual bool operator!=(const shape_config &other) const override
    {
        return !(*this == other);
    }

    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    void save_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) const override;
    void load_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) override;
};

struct empty_shape_config : public shape_config
{
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    vec3<real> center_of_mass = vec3<real>(0);

    virtual bool operator==(const shape_config &other) const override
    {
        if (type() != other.type())
            return false;
        const auto *other_empty =
            static_cast<const empty_shape_config *>(&other);
        return center_of_mass == other_empty->center_of_mass;
    }

    virtual bool operator!=(const shape_config &other) const override
    {
        return !(*this == other);
    }

    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    void save_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) const override;
    void load_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) override;
};

const tuple<const rtti *, delegate<shape_config *()>> ALL_SHAPE_CONFIG_TYPES[] =
    {
        {&box_shape_config::TYPE, []() { return new box_shape_config(); }},
        {&capsule_shape_config::TYPE,
         []() { return new capsule_shape_config(); }},
        {&convex_hull_shape_config::TYPE,
         []() { return new convex_hull_shape_config(); }},
        {&box_shape_config::TYPE, []() { return new box_shape_config(); }},
        {&capsule_shape_config::TYPE,
         []() { return new capsule_shape_config(); }},
        {&convex_hull_shape_config::TYPE,
         []() { return new convex_hull_shape_config(); }},
        {&cylinder_shape_config::TYPE,
         []() { return new cylinder_shape_config(); }},
        {&sphere_shape_config::TYPE,
         []() { return new sphere_shape_config(); }},
        {&tapered_capsule_shape_config::TYPE,
         []() { return new tapered_capsule_shape_config(); }},
        {&tapered_cylinder_shape_config::TYPE,
         []() { return new tapered_cylinder_shape_config(); }},
        {&triangle_shape_config::TYPE,
         []() { return new triangle_shape_config(); }},
        {&empty_shape_config::TYPE, []() { return new empty_shape_config(); }},
        {nullptr, nullptr},
};

shape_config *create_shape_config(string_view type_name);

enum class physics_motion_type
{
    static_ = 0,
    kinematic,
    dynamic
};

enum class physics_allowed_dofs
{
    all = 0,
    plane_2d,
    rotation_only,
    translation_only
};

enum class physics_override_mass_properties
{
    calculate_mass_and_inertia = 0,
    calculate_inertia,
    mass_and_inertia_provided
};

struct rigid_body_creation_config : base_object
{
    static const rtti TYPE;
    virtual const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    pointer<shape_config> shape = new box_shape_config();
    vec3<real> position         = vec3<real>(0);
    quat<real> rotation         = quat<real>();

    // Simulation
    physics_motion_type motion_type       = physics_motion_type::dynamic;
    physics_allowed_dofs allowed_dofs     = physics_allowed_dofs::all;
    bool allow_dynamic_or_kinematic       = true;
    bool is_sensor                        = false;
    bool collide_kinematic_vs_non_dynamic = false;
    bool use_manifold_reduction           = true;
    bool apply_gyroscopic_force           = false;
    bool allow_sleeping                   = true;

    real friction             = 0.2;
    real restitution          = 0.0;
    real linear_damping       = 0.05;
    real angular_damping      = 0.05;
    real max_linear_velocity  = 500.0;
    real max_angular_velocity = 0.25 * 3.14159 * 60.0;
    real gravity_factor       = 1.0;

    uint32_t num_velocity_steps_override = 0;
    uint32_t num_position_steps_override = 0;

    // Mass Properties
    physics_override_mass_properties override_mass_properties =
        physics_override_mass_properties::calculate_mass_and_inertia;
    real inertia_multiplier = 1.0;
    real mass_override      = 0.0;

    // Legacy/Helper
    physics_motion_quality quality = physics_motion_quality::discrete;
    uint16_t layer                 = 0;

    bool operator==(const rigid_body_creation_config &other) const
    {
        return shape == other.shape && position == other.position &&
               rotation == other.rotation && motion_type == other.motion_type &&
               allowed_dofs == other.allowed_dofs &&
               allow_dynamic_or_kinematic == other.allow_dynamic_or_kinematic &&
               is_sensor == other.is_sensor &&
               collide_kinematic_vs_non_dynamic ==
                   other.collide_kinematic_vs_non_dynamic &&
               use_manifold_reduction == other.use_manifold_reduction &&
               apply_gyroscopic_force == other.apply_gyroscopic_force &&
               allow_sleeping == other.allow_sleeping &&
               friction == other.friction && restitution == other.restitution &&
               linear_damping == other.linear_damping &&
               angular_damping == other.angular_damping &&
               max_linear_velocity == other.max_linear_velocity &&
               max_angular_velocity == other.max_angular_velocity &&
               gravity_factor == other.gravity_factor &&
               num_velocity_steps_override ==
                   other.num_velocity_steps_override &&
               num_position_steps_override ==
                   other.num_position_steps_override &&
               override_mass_properties == other.override_mass_properties &&
               inertia_multiplier == other.inertia_multiplier &&
               mass_override == other.mass_override &&
               quality == other.quality && layer == other.layer;
    }

    bool operator!=(const rigid_body_creation_config &other) const
    {
        return !(*this == other);
    }

    void save(serializer &stream) const;
    void load(serializer &stream, serializer_link *link);
    void save_xml(xml_serializer &stream, tinyxml2::XMLElement &element) const;
    void load_xml(xml_serializer &stream, tinyxml2::XMLElement &element);
};
} // namespace zabato::physics