#include "movement_tip.hpp"

#include "opacity_fader.hpp"
#include "engine/core/input/input.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/data_headers/events.hpp"
#include "projects/game/data_headers/game_input.hpp"
void game::MovementTip::try_to_enable() {
    if (auto* name_component { tmt::engine.ecs.try_get_component<tmt::Name>(entity) }) {
        // check if this should be disabled
        if (tmt::engine.player_data.get<bool>(name_component->name, false)) {
            apply_enable_disable();
        }
    }
}
void game::MovementTip::apply_enable_disable() {
    auto& ecs { tmt::engine.ecs };

    for (auto disable_ent : disable_entities_on_fade_out) {
        if (ecs.valid(disable_ent)) {
            ecs.disable(disable_ent);
        }
    }
    for (auto enable_ent : enable_entities_on_fade_out) {
        if (ecs.valid(enable_ent)) {
            ecs.enable(enable_ent);
        }
    }
}
void game::MovementTip::update(const tmt::FrameData& time) {
    auto& input { tmt::engine.input };
    bool pressed_move_key {};
    switch (movement_tip_type) {
        case MovementTypes::WASD:
            pressed_move_key = input.is_action_pressed(game::action::MOVE_FORWARD) || input.is_action_pressed(game::action::MOVE_LEFT) ||
                               input.is_action_pressed(game::action::MOVE_BACKWARD) || input.is_action_pressed(game::action::MOVE_RIGHT);

            break;
        case MovementTypes::UP_DOWN:
            pressed_move_key = input.is_action_pressed(game::action::MOVE_UP) || input.is_action_pressed(game::action::MOVE_DOWN);
            break;
        case MovementTypes::BOOST:
            pressed_move_key = input.is_action_pressed(game::action::BOOST);

            break;
        case MovementTypes::BREAK:
            pressed_move_key = input.is_action_pressed(game::action::BREAK);

            break;
    }

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
        for (auto ui_entity : ui_entities) {
            tmt::engine.ecs.get_dispatcher().trigger(
                IconTransitionEvent {
                    .icon_entity = ui_entity,
                    .current_value = percentage,
                    .min_value = percentage_for_starting_to_fade,
                    .max_value = 1.0f,
                }
            );
        }
        const auto elapsed { tmt::engine.frame_data().elapsed_time - elapsed_time };
        if (elapsed > time_to_fade) {
            apply_enable_disable();
        }
    }
}
