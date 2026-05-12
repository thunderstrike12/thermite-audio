#include "sensor_system.hpp"
#include "../gameplay_functionality_components/enemy_components/small_enemy.hpp"
#include "../gameplay_functionality_components/enemy_components/medium_enemy.hpp"
#include "../gameplay_functionality_components/player.hpp"
#include "engine/systems/ai/steering/steering_system.hpp"
#include "engine/systems/ai/steering/components/steering_agent.hpp"
#include "engine/systems/ai/goap/components/world_state.hpp"
#include "engine/systems/ai/goap/components/goap_agent.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"

namespace game {

void SensorsSystem::on_start() {
    tmt::Log::info("Sensor system on_start");

    if (!tmt::engine.ecs.valid(player_entity)) {
        auto player_view = tmt::engine.ecs.view<Player>(entt::exclude_t {});

        if (!player_view.empty()) {
            player_entity = player_view.front().entity;  // Only safe if the view is not empty
            tmt::Log::info("Player entity found: {}", player_entity);
        } else {
            player_entity = tmt::Entity {};              // invalid / null entity
            tmt::Log::info("No player entity found in current scene");
        }
    }
}

void SensorsSystem::on_update(const tmt::FrameData& /*time*/) {
    auto& ecs = tmt::engine.ecs;

    ecs.view<tmt::WorldState, tmt::GoapAgent>().each([&](tmt::Entity /*agent_entity*/, tmt::WorldState& ws, tmt::GoapAgent& agent) {
        // propagate world state change to agent
        if (ws.needs_replan) {
            agent.needs_replan = true;
            ws.needs_replan = false;
        }
    });

    auto* steering = ecs.systems.try_get<tmt::SteeringSystem>();
    if (!steering) return;
    if (!ecs.valid(player_entity)) return;

    auto* player_transform = ecs.try_get_component<tmt::Transform>(player_entity);
    if (!player_transform) return;
    glm::vec3 player_pos = player_transform->get_world_position();

    // Update each agent with a SteeringAgent component
    ecs.view<SteeringAgent>().each([&](tmt::Entity agent, SteeringAgent& /*sa*/) {
        // check cores
        auto& registry = tmt::engine.ecs.get_registry();
        auto& steering_agent = registry.get<SteeringAgent>(agent);
        auto* small_enemy = tmt::engine.ecs.try_get_component<SmallEnemy>(agent);

        // try get small enemy component, and the core entity, if no core enemy dies
        if (!small_enemy || small_enemy->core_destroyed) {
            // remove GOAP so it doesn't keep acting
            tmt::engine.ecs.remove_component<tmt::GoapAgent>(agent);
            // tmt::engine.ecs.remove_component<SteeringAgent>(agent); // if I keep the other enemies will avoid flying into it

            return;  // nothing to explode
        }

        // check world states
        auto* agent_transform = ecs.try_get_component<tmt::Transform>(agent);
        if (!agent_transform) return;

        auto* ws = ecs.try_get_component<tmt::WorldState>(agent);
        if (!ws) return;

        glm::vec3 agent_pos = agent_transform->get_world_position();
        float distance = glm::length(player_pos - agent_pos);

        ws->set_fact(tmt::FactId("s_player_in_range"), distance <= small_enemy->logic_paramaters.activation_range);
        ws->set_fact(tmt::FactId("s_player_in_explosion_zone"), distance <= small_enemy->logic_paramaters.max_explosion_range);

        if (!ws->facts[tmt::FactId("s_player_in_explosion_zone").id]) {
            ws->set_fact(tmt::FactId("s_ready_to_explode"), false);
        }
    });
}

void SensorsSystem::on_end() {
    tmt::Log::info("Sensor System on_end");
}

}  // namespace game
