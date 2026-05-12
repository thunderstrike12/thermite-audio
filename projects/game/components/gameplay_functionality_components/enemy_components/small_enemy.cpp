#include "small_enemy.hpp"

#include "engine/systems/ai/steering/components/steering_agent.hpp"
#include "engine\core\components\voxel_renderer.hpp"

namespace game {

void SmallEnemy::start() {
    auto& ecs = tmt::engine.ecs;

    // Update each agent with a SteeringAgent component
    ecs.view<SteeringAgent>().each([&](tmt::Entity agent, SteeringAgent& steering) {
        steering.min_explosion_range = logic_paramaters.min_explosion_range;
        steering.activation_range = logic_paramaters.activation_range;

        steering.arrive_radius = movement_paramaters.arrive_radius;
        steering.max_force = movement_paramaters.max_force;
        steering.max_speed = movement_paramaters.max_speed;
        steering.wander_radius_limit = movement_paramaters.wander_radius_limit;
    });

    // Cache initial voxel count of the core
    auto resource = ecs.get_component<tmt::VoxelRenderer>(core).resource;

    core_voxels = resource->blas->voxel_count - resource->blas->voxels_wasted;
}

void SmallEnemy::update(const tmt::FrameData& time) {
    if (core_destroyed) return;

    auto& ecs = tmt::engine.ecs;

    // Get current voxel count
    auto resource = ecs.get_component<tmt::VoxelRenderer>(core).resource;

    uint32_t current_core_voxels = resource->blas->voxel_count - resource->blas->voxels_wasted;

    // If voxel count changed, core was damaged/destroyed
    if (current_core_voxels != core_voxels) {
        core_destroyed = true;
        return;
    }
}

void SmallEnemy::die() {}

}  // namespace game
