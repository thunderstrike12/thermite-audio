#include "tutorial_button.hpp"

#include "engine/tools/player_data.hpp"
void game::TutorialButton::start() {
    if (entities_to_disable.empty()) {
        entities_to_disable.push_back(entity);
    }
    auto is_disabled = tmt::engine.player_data.try_get<bool>(saved_name);
    if (is_disabled.has_value()) {
        for (auto ent : entities_to_disable) {
            if (tmt::engine.ecs.valid(ent) == false) {
                continue;
            }
            tmt::engine.ecs.disable(ent);
        }
        for (auto ent : entities_to_enable) {
            if (tmt::engine.ecs.valid(ent) == false) {
                continue;
            }
            tmt::engine.ecs.enable(ent);
        }
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
    for (auto ent : entities_to_disable) {
        if (tmt::engine.ecs.valid(ent) == false) {
            continue;
        }
        tmt::engine.ecs.disable(ent);
    }
    for (auto ent : entities_to_enable) {
        if (tmt::engine.ecs.valid(ent) == false) {
            continue;
        }
        tmt::engine.ecs.enable(ent);
    }
}
