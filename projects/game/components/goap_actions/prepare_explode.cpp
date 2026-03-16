#include "prepare_explode.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/ai/steering/components/steering_agent.hpp"
#include "engine/systems/ai/steering/steering_system.hpp"
#include "../gameplay_functionality_components/player.hpp"

namespace game {

void PrepareExplode::on_start(tmt::Entity agent) {
    if (!tmt::engine.ecs.valid(player_entity)) {
        player_entity = tmt::engine.ecs.view<Player>().front().entity;
    }

    tmt::SteeringRequest request {};
    request.mode = SteeringMode::ARRIVE;

    auto* player_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(player_entity);
    if (player_transform) {
        request.target_position = player_transform->get_world_position();
    }

    auto& registry = tmt::engine.ecs.get_registry();
    registry.emplace_or_replace<tmt::SteeringRequest>(agent, request);
}

void PrepareExplode::on_fixed_tick(tmt::Entity agent, float /*dt*/) {
    if (!tmt::engine.ecs.valid(player_entity)) return;

    auto& registry = tmt::engine.ecs.get_registry();
    if (!registry.any_of<tmt::SteeringRequest>(agent)) return;

    auto& request = registry.get<tmt::SteeringRequest>(agent);

    auto* player_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(player_entity);
    if (!player_transform) return;

    request.target_position = player_transform->get_world_position();
}

bool PrepareExplode::is_done(tmt::Entity agent) const {
    auto& registry = tmt::engine.ecs.get_registry();

    if (!registry.any_of<tmt::SteeringRequest>(agent)) return false;

    const auto& request = registry.get<tmt::SteeringRequest>(agent);
    return request.completed;
}

void PrepareExplode::on_finished(tmt::Entity agent) {
    auto& registry = tmt::engine.ecs.get_registry();
    if (registry.any_of<tmt::SteeringRequest>(agent)) {
        registry.remove<tmt::SteeringRequest>(agent);
    }
}

void PrepareExplode::on_interrupt(tmt::Entity agent) {
    on_finished(agent);
}

}  // namespace game
