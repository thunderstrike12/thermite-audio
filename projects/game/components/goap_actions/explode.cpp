#include "explode.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/goap/components/world_state.hpp"

#include "engine/systems/ai/steering/steering_system.hpp"
#include "../gameplay_functionality_components/player.hpp"

namespace game {

void Explode::on_start(tmt::Entity agent) {
    auto& registry = tmt::engine.ecs.get_registry();

    if (!tmt::engine.ecs.valid(player_entity)) {
        player_entity = tmt::engine.ecs.view<Player>().front().entity;  // Assuming there's only one player entity in the game}
    }
}

void Explode::on_tick(tmt::Entity agent, float) {}

bool Explode::is_done(tmt::Entity agent) const {
    //auto& ws = tmt::engine.ecs.get_component<tmt::WorldState>(agent);
    //// Only explode if player is still in explosion zone
    //return !ws.facts[(uint32_t)std::hash<std::string>()("player_in_explosion_zone")];
    return false;
}

void Explode::on_finished(tmt::Entity agent) {}

void Explode::on_interrupt(tmt::Entity agent) {}

}  // namespace game
