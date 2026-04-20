#include "self_destruct_component.hpp"
#include "engine/core/components/button.hpp"

#include "projects/game/components/gameplay_functionality_components/player.hpp"

namespace game {

void SelfDestructComponent::start() {
    // Guard for button
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        tmt::Log::info("Found button component, adding self destruct to on click.");
        button_component->on_click.add(this, &SelfDestructComponent::self_destruct);
    } else {
        tmt::Log::warn("No button found for self destruct component!");
    }
}

void SelfDestructComponent::update(const tmt::FrameData& time) {}

void SelfDestructComponent::end() {
    // Guard for button
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        button_component->on_click.clear();
    }
}

// Cannot be made const because button does not take const functions
void SelfDestructComponent::self_destruct(tmt::Button::Context) {
    // Guard for player entity
    if (player_entity == entt::null) {
        tmt::Log::error("Self destruct button does not have a player entity set.");
        return;
    }

    // Guard for player component
    if (auto* player_component = tmt::engine.ecs.try_get_component<Player>(player_entity)) {
        player_component->health.value = -0.1f;
    } else {
        tmt::Log::error("Player entity that was assigned to self destruct button does not have a player component.");
    }
}

}  // namespace game