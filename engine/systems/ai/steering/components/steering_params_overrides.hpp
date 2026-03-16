#pragma once

#include <unordered_map>
#include <string>

#include "engine/core/reflection.hpp"
#include "steering_params.hpp"

namespace tmt {

/**
 * Class SteeringOverrides
 * Container that stores editor overrides for SteeringRequest.
 *
 * Provides runtime access to overridden arrive_radius, activation_range, etc.
 */
class SteeringOverrides {
   public:
    // Overriden paramaters for steering agents
    SteeringParams params;

    void load();
    void save() const;
};

}  // namespace tmt

TMT_OBJECT(tmt::SteeringOverrides, (params));
