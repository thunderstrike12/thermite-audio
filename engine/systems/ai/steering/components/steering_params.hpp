#pragma once

#include "engine/core/reflection.hpp"

namespace tmt {

/**
 * Struct SteeringParams
 * Holds override data for steering paramaters, editable in the editor.
 *
 * Allows modifying a steering requests arrive_radius, activation_range, and min_explosion_range at runtime
 * without changing the original SteeringRequest definition.
 */
struct SteeringParams {
    float arrive_radius = 6.f;         // Radius it will start to slow down at
    float activation_range = 50.f;     // Range it will start to follow you
    float min_explosion_range = 1.f;   // Minimum explosion range
    float max_explosion_range = 5.f;   // Maximum explosion range
    float max_speed = 5.f;             // Max movement speed
    float max_force = 20.f;            // Max turn speed
    float wander_radius_limit = 25.f;  // Max distance it can wander
};

}  // namespace tmt

TMT_OBJECT(tmt::SteeringParams, (arrive_radius, activation_range, min_explosion_range, max_explosion_range, max_speed, max_force, wander_radius_limit));
