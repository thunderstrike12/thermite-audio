#include "movement_tip.hpp"

#include "opacity_fader.hpp"
#include "engine/core/input/input.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/data_headers/events.hpp"
#include "projects/game/data_headers/game_input.hpp"
void game::MovementTip::on_entity_enabled() {
    if (auto* name_component { tmt::engine.ecs.try_get_component<tmt::Name>(entity) }) {
        // check if this should be disabled
        if (tmt::engine.player_data.get<bool>(name_component->name, false)) {
            tmt::engine.ecs.disable(entity);
        }
    }
}
void game::MovementTip::update(const tmt::FrameData& time) {
    auto& input { tmt::engine.input };
    bool pressed_move_key { input.is_action_pressed(game::action::MOVE_UP) || input.is_action_pressed(game::action::MOVE_LEFT) || input.is_action_pressed(game::action::MOVE_BACKWARD) ||
                            input.is_action_pressed(game::action::MOVE_RIGHT) };
    if (pressed_move_key) {
        if (first_time) {
            elapsed_time = tmt::engine.frame_data().elapsed_time;
            first_time = false;
            // save disable of this component
            if (auto* name_component { tmt::engine.ecs.try_get_component<tmt::Name>(entity) }) {
                tmt::engine.player_data.get<bool>(name_component->name, false) = true;
            }
        }
    }
    if (first_time == false) {
        auto percentage = tmt::engine.frame_data().elapsed_time - elapsed_time;
        percentage = std::min(percentage, time_to_fade);
        percentage /= time_to_fade;
        tmt::engine.ecs.get_dispatcher().trigger(
            IconTransitionEvent {
                .icon_entity = ui_entity,
                .current_value = percentage,
                .min_value = percentage_for_starting_to_fade,
                .max_value = 1.0f,
            }
        );
    }
}
void game::MovementTip::on_entity_disabled() {}
