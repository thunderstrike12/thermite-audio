#include "clear_save_data_button.hpp"

#include "engine/tools/player_data.hpp"

namespace game {

void ClearSaveButtonComponent::start() {
    // Guard for button
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        tmt::Log::info("Found button component, adding self destruct to on click.");
        button_component->on_click.add(this, &ClearSaveButtonComponent::on_click);
    } else {
        tmt::Log::warn("No button found for self destruct component!");
    }
}

void ClearSaveButtonComponent::update(const tmt::FrameData& time) {}

void ClearSaveButtonComponent::end() {
    // Guard for button
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        button_component->on_click.clear();
    }
}

void ClearSaveButtonComponent::on_click(tmt::Button::Context context) {
    if (context.disabled) {
        return;
    }
    tmt::engine.player_data.clear();
}

}  // namespace game
