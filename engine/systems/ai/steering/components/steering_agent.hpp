#pragma once

#include <glm/glm.hpp>

/**
 * Struct SteeringAgent
 * Represents a steering-capable agent.
 *
 * Used by SteeringSystem to calculate motion, forces, and obstacle avoidance.
 */
struct SteeringAgent {
    bool active = true;  // Whether the agent is currently participating in steering

    // For wandering aroundthe origin
    glm::vec3 wander_origin;
    bool has_origin = false;

    float min_explosion_range = 5.f;

    float arrive_radius = 6.f;         // Radius it will start to slow down at
    float activation_range = 50.f;     // Range it will start to follow you
    float max_speed = 5.f;             // Max movement speed
    float max_force = 20.f;            // Max turn speed
    float wander_radius_limit = 25.f;  // Max distance it can wander

    // Used for gravity gun
    bool force_override = false;
    glm::vec3 override_force = glm::vec3(0);
};
