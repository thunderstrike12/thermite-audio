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
    float arrive_radius = 6.f;
    float activation_range = 50.f;
    float min_explosion_range = 1.f;
    float max_explosion_range = 5.f;
    float max_speed = 5.f;
    float max_force = 20.f;
};

}  // namespace tmt

TMT_OBJECT(tmt::SteeringParams, (arrive_radius, activation_range, min_explosion_range, max_explosion_range, max_speed, max_force));
