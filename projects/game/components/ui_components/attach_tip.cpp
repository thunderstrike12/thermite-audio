#include "attach_tip.hpp"

#include "projects/game/data_headers/events.hpp"

void game::AttachTip::start() {
    Tweening::tween(fade_tween).from(0.0f).to(0.0f).ease(Tweening::Ease::OUT_QUAD).on_update([this](float alpha, float*) {
        const auto& s = fade_tween->get_start();
        const auto& e = fade_tween->get_end();
        current_alpha = s + (e - s) * alpha;

        for (auto ui_entity : ui_entities) {
            tmt::engine.ecs.get_dispatcher().trigger(
                IconTransitionEvent {
                    .icon_entity = ui_entity,
                    .current_value = current_alpha,
                    .min_value = percentage_for_starting_to_fade,
                    .max_value = 1.0f,
                }
            );
        }
    });

    tmt::engine.ecs.get_dispatcher().sink<game::InRangeEvent>().connect<&AttachTip::on_in_range>(this);
}

void game::AttachTip::end() {
    tmt::engine.ecs.get_dispatcher().sink<game::InRangeEvent>().disconnect<&AttachTip::on_in_range>(this);
}

void game::AttachTip::on_in_range(const InRangeEvent& e) {
    if (e.entity != entity) return;
    if (in_range == e.in_range) return;  // already in this state
    in_range = e.in_range;

    if (in_range) {
        show();
    } else {
        hide();
    }
}

void game::AttachTip::show() {
    fade_tween->from(current_alpha).to(0.0f).duration(time_to_fade_in);
    fade_tween->restart();
}

void game::AttachTip::hide() {
    fade_tween->from(current_alpha).to(1.0f).duration(time_to_fade_out);
    fade_tween->restart();
}
