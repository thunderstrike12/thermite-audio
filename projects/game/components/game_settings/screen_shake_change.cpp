#include "screen_shake_change.hpp"

#include "engine/tools/player_data.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"
#include "projects/game/data_headers/save_entries.hpp"

void game::ScreenShakeChangeComponent::start() {
    auto* slider { tmt::engine.ecs.try_get_component<tmt::Slider>(entity) };
    if (slider == nullptr) {
        return;
    }
    const auto& cam_shake { tmt::engine.player_data.get<float>(CAMERA_SHAKE, 1.0f) };

    slider->value = cam_shake;
    slider->on_value_changed.add(this, &ScreenShakeChangeComponent::on_value_changed);
}

void game::ScreenShakeChangeComponent::on_value_changed(tmt::Slider::Context context) {
    if (context.disabled) return;

    auto& cam_shake { tmt::engine.player_data.get<float>(CAMERA_SHAKE, 1.0f) };

    cam_shake = context.value;
}
void game::ScreenShakeChangeComponent::end() {
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Slider>(entity)) {
        button_component->on_value_changed.clear();
    }
}