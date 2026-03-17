#pragma once

#include <glm/glm.hpp>

#include "steering_params.hpp"

/**
 * Struct SteeringAgent
 * Represents a steering-capable agent.
 *
 * Used by SteeringSystem to calculate motion, forces, and obstacle avoidance.
 */
struct SteeringAgent {
    const tmt::SteeringParams* params = nullptr;
    bool active = true;  // Whether the agent is currently participating in steering

    // For wandering aroundthe origin
    glm::vec3 wander_origin;
    bool has_origin = false;
};
