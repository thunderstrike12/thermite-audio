#include "explode.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

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
    return false;
}

void Explode::on_finished(tmt::Entity agent) {}

void Explode::on_interrupt(tmt::Entity agent) {}

}  // namespace game
