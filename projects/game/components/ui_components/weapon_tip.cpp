#include "weapon_tip.hpp"

#include "engine/tools/player_data.hpp"

void game::WeaponTip::start() {
    Tweening::tween(fade_tween).from(1.0f).to(1.0f).ease(Tweening::Ease::OUT_QUAD).on_update([this](float alpha, float*) {
        const auto& s = fade_tween->get_start();
        const auto& e = fade_tween->get_end();
        current_alpha = s + (e - s) * alpha;

        for (auto ui_entity : ui_entities) {
            tmt::engine.ecs.get_dispatcher().trigger(
                IconTransitionEvent {
                    .icon_entity = ui_entity,
                    .current_value = current_alpha,
                    .min_value = 0.0f,
                    .max_value = 1.0f,
                }
            );
        }
    });

    current_alpha = 1.0f;
    for (auto ui_entity : ui_entities) {
        tmt::engine.ecs.get_dispatcher().trigger(
            IconTransitionEvent {
                .icon_entity = ui_entity,
                .current_value = 1.0f,
                .min_value = 0.0f,
                .max_value = 1.0f,
            }
        );
    }

    started = true;
    if (pending_show) {
        pending_show = false;
        show();
    }
}

void game::WeaponTip::on_entity_enabled() {}

void game::WeaponTip::on_weapon_switched(WeaponType new_weapon) {
    const bool match = (new_weapon == weapon_type);
    if (!started) {
        pending_show = match;
        return;
    }
    if (match) {
        show();
    } else {
        hide();
    }
}

void game::WeaponTip::show() {
    fade_tween->from(current_alpha).to(0.0f).duration(time_to_fade_in).end_delay(never_fade_out ? 0.0f : time_to_hold).on_complete(never_fade_out ? std::function<void()> {} : [this] {
        hide();
    });
    fade_tween->restart();
}

void game::WeaponTip::hide() {
    fade_tween->from(current_alpha).to(1.0f).duration(time_to_fade_out).end_delay(0.0f).on_complete(nullptr);
    fade_tween->restart();
}
