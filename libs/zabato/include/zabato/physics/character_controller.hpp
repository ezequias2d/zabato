#pragma once

#include <zabato/controller.hpp>
#include <zabato/event.hpp>
#include <zabato/physics/physics_controller.hpp>
#include <zabato/physics/types.hpp>
#include <zabato/shape.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/transformation.hpp>
#include <zabato/unique_ptr.hpp>

namespace zabato::physics
{

enum class back_face_mode
{
    /** @brief Ignore collision with back facing surfaces/triangles. */
    ignore_back_faces,
    /** @brief Collide with back facing surfaces/triangles. */
    collide_with_back_faces,
};

struct character_creation_config : public base_object
{
    static const rtti TYPE;
    const rtti &type() const override final { return TYPE; }
    static void reflect(reflection &r);

    vec3<real> up                       = {0, 1, 0};
    plane3<real> supporting_volume      = {{0, 1, 0}, -1.0e10};
    real max_slope_angle                = to_rad((real)50);
    bool enhanced_internal_edge_removal = false;
    pointer<shape_config> shape         = new capsule_shape_config();

    back_face_mode back_face;
    real predictive_contact_distance   = 0.1;
    uint32_t max_collision_iterations  = 5;
    uint32_t max_constraint_iterations = 15;
    real min_time_remaining            = 1.0e-4;
    real collision_tolerance           = 1.0e-3;
    real character_padding             = 0.02;
    uint32_t max_num_hits              = 256;
    real hit_reduction_cos_max_angle   = 0.999;
    real penetration_recovery_speed    = 1.0;

    pointer<shape_config> inner_body_shape = new capsule_shape_config();

    real mass               = 70.0;
    real max_strength       = 100.0;
    vec3<real> shape_offset = {0, 0, 0};
    real gravity_factor     = 1.0;

    bool operator==(const character_creation_config &other) const
    {
        return shape == other.shape && mass == other.mass &&
               max_slope_angle == other.max_slope_angle &&
               gravity_factor == other.gravity_factor && up == other.up &&
               supporting_volume.normal == other.supporting_volume.normal &&
               supporting_volume.d == other.supporting_volume.d &&
               enhanced_internal_edge_removal ==
                   other.enhanced_internal_edge_removal &&
               back_face == other.back_face &&
               predictive_contact_distance ==
                   other.predictive_contact_distance &&
               max_collision_iterations == other.max_collision_iterations &&
               max_constraint_iterations == other.max_constraint_iterations &&
               min_time_remaining == other.min_time_remaining &&
               collision_tolerance == other.collision_tolerance &&
               character_padding == other.character_padding &&
               max_num_hits == other.max_num_hits &&
               hit_reduction_cos_max_angle ==
                   other.hit_reduction_cos_max_angle &&
               penetration_recovery_speed == other.penetration_recovery_speed &&
               inner_body_shape == other.inner_body_shape &&
               max_strength == other.max_strength &&
               shape_offset == other.shape_offset;
    }

    bool operator!=(const character_creation_config &other) const
    {
        return !(*this == other);
    }

    void save(serializer &stream) const;
    void load(serializer &stream, serializer_link *link);
    void save_xml(xml_serializer &stream, tinyxml2::XMLElement &element) const;
    void load_xml(xml_serializer &stream, tinyxml2::XMLElement &element);
};

/**
 * @class character_controller
 * @brief Logic controller that bridges a scene node with a physics_character.
 *
 * Automatically updates the scene node's transform to match the physics
 * simulation. Also provides a simplified interface for moving the character
 * from game logic.
 */
class character_controller : public physics_controller
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    character_controller();
    virtual ~character_controller();

    void initialize(const controller::context &ctx) override;
    void start() override;
    void update(real dt) override;

    void save(serializer &stream) const override;
    void load(serializer &stream, serializer_link *link) override;
    void save_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) const override;
    void load_xml(xml_serializer &stream,
                  tinyxml2::XMLElement &element) override;

    /** @brief Proxy to physics_character::is_on_ground */
    virtual bool is_on_ground() const = 0;

    virtual character_creation_config get_config() const             = 0;
    virtual void set_config(const character_creation_config &config) = 0;
};

} // namespace zabato::physics
