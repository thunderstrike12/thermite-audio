#include "quit_game_component.hpp"

namespace game {

void QuitGameComponent::start() {
    // Guard for button
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        tmt::Log::info("Found button component, adding quit game to on click.");
        button_component->on_click.add(this, &QuitGameComponent::quit_game);
    } else {
        tmt::Log::warn("No button found for quit game component!");
    }
}

void QuitGameComponent::update(const tmt::FrameData& time) {}

void QuitGameComponent::end() {
    // Guard for button
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        button_component->on_click.clear();
    }
}

void QuitGameComponent::quit_game(tmt::Button::Context context) {
    if (context.disabled) return;
    tmt::engine.game_controller.end_game();
    tmt::engine.set_is_running(false);
}

}  // namespace game