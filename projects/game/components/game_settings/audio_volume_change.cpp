#include "audio_volume_change.hpp"

#include "engine/tools/player_data.hpp"
void game::AudioVolumeChangeComponent::start() {
    auto* slider { tmt::engine.ecs.try_get_component<tmt::Slider>(entity) };
    if (slider == nullptr) {
        return;
    }
    auto name { tmt::engine.ecs.get_component<tmt::Name>(entity).name };
    auto& saved_value { tmt::engine.player_data.get<float>("Slider_" + name, volume_control.get_volume()) };
    volume_control.set_volume(saved_value);
    slider->value = saved_value;
    slider->on_value_changed.add(this, &AudioVolumeChangeComponent::on_value_changed);
}
void game::AudioVolumeChangeComponent::on_value_changed(tmt::Slider::Context context) {
    if (context.disabled) return;
    volume_control.set_volume(context.value);
    auto name { tmt::engine.ecs.get_component<tmt::Name>(entity).name };
    auto& saved_value { tmt::engine.player_data.get<float>("Slider_" + name, volume_control.get_volume()) };
    saved_value = context.value;
}
void game::AudioVolumeChangeComponent::update(const tmt::FrameData& time) {}
void game::AudioVolumeChangeComponent::end() {
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Slider>(entity)) {
        button_component->on_value_changed.clear();
    }
}
