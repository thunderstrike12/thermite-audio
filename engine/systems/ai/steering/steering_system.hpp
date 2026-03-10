#pragma once

#include <glm/glm.hpp>

#include "engine/core/system.hpp"
#include "engine/core/ecs.hpp"

#include "systems/physics/components/voxel_body.hpp"
#include "components/steering_agent.hpp"
#include "components/steering_mode.hpp"

namespace tmt {

/**
 * Class SteeringSystem
 * Handles all steering behaviors for agents.
 *
 * Responsibilities:
 *  - Processes each SteeringAgent every fixed update.
 *  - Applies SEEK, ARRIVE, FLEE, WANDER behaviors.
 *  - Performs obstacle avoidance using raycasts.
 */
class SteeringSystem : public ISystem {
   public:
    // Inherited via ISystem
    std::string get_name() override { return "Steering System"; }
    void on_start() override;
    void on_update(const FrameData& time) override {}
    void on_fixed_update(const FrameData& time) override;
    void on_end() override {}

    /**
     * Calculate SEEK force toward a target.
     */
    glm::vec3 seek(const SteeringAgent& agent, const glm::vec3& pos, const glm::vec3& target, const glm::vec3& velocity);

    /**
     * Calculate ARRIVE force toward a target, slowing near radius.
     */
    glm::vec3 arrive(const SteeringAgent& agent, const glm::vec3& pos, const glm::vec3& target, float radius, const glm::vec3& velocity);

    /**
     * Calculate WANDER steering force based on WanderData.
     */
    glm::vec3 wander(const SteeringAgent& agent, const glm::vec3& position, const VoxelBody& body, WanderData wander, float dt);

    /**
     * Calculate collision avoidance force using forward and side rays.
     * Ignores entities on the enemy layer.
     */
    glm::vec3 collision_avoidance(Entity entity, const SteeringAgent& agent, const glm::vec3& position, const VoxelBody& body);

    /**
     * Calculate total steering force based on current request and obstacle avoidance.
     */
    glm::vec3 calculate_force(const SteeringAgent& agent, const SteeringRequest& request, const Transform& transform, const VoxelBody& body, float dt);

    /**
     * Check if an ARRIVE request is complete and stop the agent.
     */
    void check_completion(Entity entity, SteeringRequest& request, const Transform& transform, VoxelBody& body);
};

}  // namespace tmt
