#include "attach_tip.hpp"

#include <algorithm>

#include "opacity_fader.hpp"
#include "projects/game/data_headers/events.hpp"
void game::AttachTip::start() {
    tmt::engine.ecs.get_dispatcher().sink<game::InRangeEvent>().connect<&AttachTip::on_in_range>(this);
}
void game::AttachTip::update(const tmt::FrameData& time) {
    auto percentage = tmt::engine.frame_data().elapsed_time - elapsed_time;
    percentage = std::min(percentage, time_to_fade);
    percentage /= time_to_fade;

    tmt::engine.ecs.get_dispatcher().trigger(
        IconTransitionEvent {
            .icon_entity = ui_entity,
            .current_value = in_range ? (1.0f - percentage) : percentage,
            .min_value = percentage_for_starting_to_fade,
            .max_value = 1.0f,
        }
    );
}
void game::AttachTip::end() {
    tmt::engine.ecs.get_dispatcher().sink<game::InRangeEvent>().disconnect<&AttachTip::on_in_range>(this);
}
void game::AttachTip::on_in_range(const InRangeEvent& e) {
    if (e.entity != entity) {
        return;
    }
    if (in_range != e.in_range) {
        in_range = e.in_range;
        elapsed_time = tmt::engine.frame_data().elapsed_time;
    }
}
