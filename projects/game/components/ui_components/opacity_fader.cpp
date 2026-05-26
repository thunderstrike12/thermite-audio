#include "opacity_fader.hpp"

#include "engine/core/components/image_renderer.hpp"
#include "engine/core/components/text_renderer.hpp"
#include "projects/game/data_headers/events.hpp"
void game::OpacityFader::start() {
    tmt::engine.ecs.get_dispatcher().sink<game::IconTransitionEvent>().connect<&OpacityFader::on_transition>(this);
}
void game::OpacityFader::end() {
    tmt::engine.ecs.get_dispatcher().sink<game::IconTransitionEvent>().disconnect<&OpacityFader::on_transition>(this);
}
void game::trigger_icon_fade(const std::vector<entt::entity>& ui_entities, const float start_time, const float time_to_fade, const float min_value, const bool fade_out) {
    auto percentage = tmt::engine.frame_data().elapsed_time - start_time;
    percentage = std::min(percentage, time_to_fade);
    percentage /= time_to_fade;

    const float current_value = fade_out ? (1.0f - percentage) : percentage;

    for (const auto ui_entity : ui_entities) {
        tmt::engine.ecs.get_dispatcher().trigger(
            IconTransitionEvent {
                .icon_entity = ui_entity,
                .current_value = current_value,
                .min_value = min_value,
                .max_value = 1.0f,
            }
        );
    }
}
void game::OpacityFader::on_transition(const game::IconTransitionEvent& event) const {
    if (event.icon_entity != entity) return;

    auto* ui { tmt::engine.ecs.try_get_component<tmt::ImageRenderer>(entity) };
    auto* text { tmt::engine.ecs.try_get_component<tmt::TextRenderer>(entity) };
    // inverse lerp
    auto factor { event.current_value - event.min_value };
    factor /= event.max_value - event.min_value;

    if (ui) {
        ui->color.get().a = event.max_value - curve.eval(factor);
    }
    if (text) {
        text->color.get().a = event.max_value - curve.eval(factor);
    }
}
