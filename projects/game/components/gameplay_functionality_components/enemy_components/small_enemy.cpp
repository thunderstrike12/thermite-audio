#include "small_enemy.hpp"

#include "engine/systems/ai/steering/components/steering_agent.hpp"

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
}

void SmallEnemy::die() {}

}  // namespace game
