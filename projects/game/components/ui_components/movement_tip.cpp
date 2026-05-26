#include "movement_tip.hpp"

#include "opacity_fader.hpp"
#include "engine/core/input/input.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/data_headers/game_input.hpp"
void game::MovementTip::try_to_enable(bool& first_enabled) {
    if (auto* name_component { tmt::engine.ecs.try_get_component<tmt::Name>(entity) }) {
        // check if this should be disabled
        auto& was_played_already { tmt::engine.player_data.get<bool>(name_component->name, false) };

        if (was_played_already == true) {
            apply_enable_disable();
        } else if (first_enabled) {
            tmt::engine.ecs.enable(entity);
            first_enabled = false;
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
void game::MovementTip::on_entity_enabled() {
    for (auto ui_entity : ui_entities) {
        tmt::engine.ecs.get_dispatcher().trigger(
            IconTransitionEvent {
                .icon_entity = ui_entity,
                .current_value = 1.0f,
                .min_value = percentage_for_starting_to_fade,
                .max_value = 1.0f,
            }
        );
    }

    // Fade in
    Tweening::tween(fade_tween).from(1.0f).to(0.0f).duration(time_to_fade_in).ease(Tweening::Ease::OUT_QUAD).on_update([this](float alpha, float*) {
        const auto& s = fade_tween->get_start();
        const auto& e = fade_tween->get_end();
        const float v = s + (e - s) * alpha;
        for (auto ui_entity : ui_entities) {
            tmt::engine.ecs.get_dispatcher().trigger(
                IconTransitionEvent {
                    .icon_entity = ui_entity,
                    .current_value = v,
                    .min_value = percentage_for_starting_to_fade,
                    .max_value = 1.0f,
                }
            );
        }
    });
}
void game::MovementTip::start() {}
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

    if (pressed_move_key && !started) {
        started = true;

        if (auto* name_component { tmt::engine.ecs.try_get_component<tmt::Name>(entity) }) {
            tmt::engine.player_data.get<bool>(name_component->name, false) = true;
        }

        fade_tween->from(0.0f).to(1.0f).duration(time_to_fade_out).ease(Tweening::Ease::IN_QUAD).on_complete([this] { apply_enable_disable(); });
        fade_tween->restart();
    }
}
