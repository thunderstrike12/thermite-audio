#include "small_enemy.hpp"

#include "engine/systems/ai/steering/components/steering_agent.hpp"
#include "engine\core\components\voxel_renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/core/polyline.hpp"
#include "engine/systems/physics/physics_system.hpp"

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

void SmallEnemy::die(tmt::Entity agent) {
    auto& ecs = tmt::engine.ecs;
    auto& registry = ecs.get_registry();

    auto& transform = registry.get<tmt::Transform>(agent);

    std::set<tmt::Entity> children = transform.get_all_children();

    for (tmt::Entity child : children) {
        if (!ecs.valid(child)) continue;

        if (ecs.has_component<tmt::RigModel>(child)) {
            ecs.remove_component<tmt::RigModel>(child);
        }

        if (ecs.has_component<tmt::VoxelBody>(child) == false) continue;

        if (auto* renderer = ecs.try_get_component<tmt::VoxelRenderer>(child)) {
            tmt::VoxelBody* body = ecs.try_get_component<tmt::VoxelBody>(child);

            if (body->type == tmt::VoxelBody::STATIC) {
                // Preserve current world transform
                auto& child_transform = registry.get<tmt::Transform>(child);
                auto pos = child_transform.get_world_position();
                auto rot = child_transform.get_world_rotation();

                // Force sync physics transform
                body->type = tmt::VoxelBody::DYNAMIC;
                body->position = pos;
                body->rotation = rot;

                tmt::Physics::initialize_voxel_body(*body, *renderer->resource.resource.get());
            }
        }
    }
}

}  // namespace game
