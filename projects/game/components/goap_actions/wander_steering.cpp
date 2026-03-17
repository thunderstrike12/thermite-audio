#include "wander_steering.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"

#include "engine/systems/ai/steering/steering_system.hpp"
#include "engine/systems/ai/steering/components/steering_mode.hpp"
#include "engine/systems/ai/steering/components/steering_agent.hpp"
#include "../gameplay_functionality_components/player.hpp"

namespace game {

void WanderSteering::on_start(tmt::Entity agent) {
    auto& registry = tmt::engine.ecs.get_registry();

    if (!registry.any_of<SteeringAgent>(agent)) {
        registry.emplace<SteeringAgent>(agent);
    }

    auto* steering = tmt::engine.ecs.systems.try_get<tmt::SteeringSystem>();

    if (!steering) {
        tmt::Log::warn("Steering system not active.");
        return;
    }

    // Get the SteeringAgent component & copy global steering params into this agent
    auto& steering_agent = registry.get<SteeringAgent>(agent);
    steering_agent.params = &steering->overrides().params;

    // Create a wander steering request
    tmt::SteeringRequest request {};
    request.mode = SteeringMode::WANDER;

    // set initial WanderData
    request.wander_data.wander_radius = 2.0f;                // radius of the wander sphere
    request.wander_data.wander_distance = 3.0f;              // distance in front of the agent
    request.wander_data.wander_jitter = 0.5f;                // how much the target jitters per second
    request.wander_data.wander_target = glm::vec3(0, 0, 0);  // start at sphere center

    registry.emplace_or_replace<tmt::SteeringRequest>(agent, request);

    if (!tmt::engine.ecs.valid(player_entity)) {
        player_entity = tmt::engine.ecs.view<Player>().front().entity;  // Assuming there's only one player entity in the game
    }
}

void WanderSteering::on_fixed_tick(tmt::Entity agent, float /*dt*/) {}

bool WanderSteering::is_done(tmt::Entity /*agent*/) const {
    return false;
}

void WanderSteering::on_finished(tmt::Entity agent) {
    auto& registry = tmt::engine.ecs.get_registry();
    if (registry.any_of<tmt::SteeringRequest>(agent)) {
        registry.remove<tmt::SteeringRequest>(agent);
    }
}

void WanderSteering::on_interrupt(tmt::Entity agent) {
    on_finished(agent);
}

}  // namespace game
