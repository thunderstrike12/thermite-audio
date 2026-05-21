#include "opacity_fader.hpp"

#include "engine/core/components/image_renderer.hpp"
#include "projects/game/data_headers/events.hpp"
void game::OpacityFader::start() {
    tmt::engine.ecs.get_dispatcher().sink<game::IconTransitionEvent>().connect<&OpacityFader::on_transition>(this);
}
void game::OpacityFader::end() {
    tmt::engine.ecs.get_dispatcher().sink<game::IconTransitionEvent>().disconnect<&OpacityFader::on_transition>(this);
}
void game::OpacityFader::on_transition(const game::IconTransitionEvent& event) {
    if (event.icon_entity != entity) return;

    auto* ui { tmt::engine.ecs.try_get_component<tmt::ImageRenderer>(entity) };
    if (ui == nullptr) return;
    // inverse lerp
    auto factor { event.current_value - event.min_value };
    factor /= event.max_value - event.min_value;

    ui->color.get().a = event.max_value - curve.eval(factor);
}
