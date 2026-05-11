#include "tutorial_button.hpp"

#include "engine/tools/player_data.hpp"
void game::TutorialButton::start() {
    auto is_disabled = tmt::engine.player_data.try_get<bool>(saved_name);
    if (is_disabled.has_value()) {
        tmt::engine.ecs.disable(entity);
    }
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        tmt::Log::info("Found button component, adding quit game to on click.");
        button_component->on_click.add(this, &TutorialButton::disable_entity);
    }
}
void game::TutorialButton::disable_entity(tmt::Button::Context context) {
    if (context.disabled) return;

    auto& disabled = tmt::engine.player_data.get<bool>(saved_name);
    disabled = true;
    tmt::engine.ecs.disable(entity);
}