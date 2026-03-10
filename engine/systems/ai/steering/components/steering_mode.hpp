#pragma once

#include <glm/glm.hpp>

/**
 * Enum SteeringMode
 * Defines the current steering behavior of an agent.
 *
 * Modes:
 *  - NONE   : No movement requested.
 *  - SEEK   : Move directly toward a target.
 *  - ARRIVE : Move toward a target, slowing down when close.
 *  - FLEE   : Move directly away from a target.
 *  - WANDER : Move randomly in a wandering pattern.
 *  - PERSUE : Predictive chasing of a moving target (not implemented here yet).
 */
enum class SteeringMode { NONE, SEEK, ARRIVE, FLEE, WANDER, PERSUE };

/**
 * Struct WanderData
 * Stores data for the wandering behavior.
 *
 * Members:
 *  - wanderTarget   : Local target offset for wandering.
 *  - wanderRadius   : Radius of the jitter circle in local space.
 *  - wanderDistance : Distance in front of the agent to project the wander target.
 *  - wanderJitter   : Maximum random displacement per second.
 *
 * Used by SteeringSystem to calculate the wander force.
 */
struct WanderData {
    glm::vec3 wander_target = glm::vec3(0.0f);
    float wander_radius = 2.0f;
    float wander_distance = 3.0f;
    float wander_jitter = 0.5f;
};

/**
 * Struct SteeringRequest
 * Represents a single steering command for an agent.
 *
 * Members:
 *  - mode           : Which SteeringMode to perform.
 *  - target_position: Position for SEEK, ARRIVE, or FLEE behaviors.
 *  - arrive_radius  : Stopping radius for ARRIVE.
 *  - completed      : Whether the request has been fulfilled.
 *  - wanderData     : Data for wandering as shown above.
 */
struct SteeringRequest {
    SteeringMode mode = SteeringMode::NONE;

    glm::vec3 target_position = {};
    float arrive_radius = 6.f;

    bool completed = false;

    WanderData wander_data;
};
