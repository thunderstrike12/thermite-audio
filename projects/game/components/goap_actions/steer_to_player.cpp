#include "steer_to_player.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/ai/steering/components/steering_mode.hpp"
#include "engine/systems/ai/steering/components/steering_agent.hpp"
#include "engine/systems/ai/steering/steering_system.hpp"
#include "../gameplay_functionality_components/player.hpp"

namespace game {

void SteerToPlayer::on_start(tmt::Entity agent) {
    if (!tmt::engine.ecs.valid(player_entity)) {
        player_entity = tmt::engine.ecs.view<Player>().front().entity;
    }

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

    tmt::SteeringRequest request {};
    request.mode = SteeringMode::SEEK;

    auto* player_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(player_entity);
    if (player_transform) {
        request.target_position = player_transform->get_world_position();
    }

    registry.emplace_or_replace<tmt::SteeringRequest>(agent, request);
}

void SteerToPlayer::on_fixed_tick(tmt::Entity agent, float /*dt*/) {
    if (!tmt::engine.ecs.valid(player_entity)) return;

    auto& registry = tmt::engine.ecs.get_registry();
    if (!registry.any_of<tmt::SteeringRequest>(agent)) return;

    auto& request = registry.get<tmt::SteeringRequest>(agent);

    auto* player_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(player_entity);
    if (!player_transform) return;

    request.target_position = player_transform->get_world_position();
}

bool SteerToPlayer::is_done(tmt::Entity agent) const {
    if (!tmt::engine.ecs.valid(player_entity)) return true;

    auto& registry = tmt::engine.ecs.get_registry();
    if (!registry.any_of<tmt::SteeringRequest>(agent)) return true;
    auto& request = registry.get<tmt::SteeringRequest>(agent);

    auto* player_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(player_entity);
    auto* agent_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(agent);
    if (!player_transform || !agent_transform) return true;

    float distance = glm::length(player_transform->get_world_position() - agent_transform->get_world_position());

    auto* steering = tmt::engine.ecs.systems.try_get<tmt::SteeringSystem>();
    if (!steering) return true;

    const auto& params = steering->overrides().params;

    return distance <= params.max_explosion_range;
}

void SteerToPlayer::on_finished(tmt::Entity agent) {
    auto& registry = tmt::engine.ecs.get_registry();
    if (registry.any_of<tmt::SteeringRequest>(agent)) {
        registry.remove<tmt::SteeringRequest>(agent);
    }
}

void SteerToPlayer::on_interrupt(tmt::Entity agent) {
    on_finished(agent);
}

}  // namespace game
