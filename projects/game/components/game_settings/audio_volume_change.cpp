#include "audio_volume_change.hpp"
void game::AudioVolumeChangeComponent::start() {
    auto* slider { tmt::engine.ecs.try_get_component<tmt::Slider>(entity) };
    if (slider == nullptr) {
        return;
    }
    slider->value = volume_control.get_volume();
    slider->on_value_changed.add(this, &AudioVolumeChangeComponent::on_value_changed);
}
void game::AudioVolumeChangeComponent::on_value_changed(tmt::Slider::Context context) {
    if (context.disabled) return;
    volume_control.set_volume(context.value);
}
void game::AudioVolumeChangeComponent::update(const tmt::FrameData& time) {}
void game::AudioVolumeChangeComponent::end() {
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Slider>(entity)) {
        button_component->on_value_changed.clear();
    }
}