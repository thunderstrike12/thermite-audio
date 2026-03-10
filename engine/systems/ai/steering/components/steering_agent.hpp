#pragma once

#include <glm/glm.hpp>

/**
 * Struct SteeringAgent
 * Represents a steering-capable agent.
 *
 * Used by SteeringSystem to calculate motion, forces, and obstacle avoidance.
 */
struct SteeringAgent {
    float max_speed = 5.f;   // Maximum linear speed the agent can move at
    float max_force = 20.f;  // Maximum steering force that can be applied per frame
    bool active = true;      // Whether the agent is currently participating in steering
};
