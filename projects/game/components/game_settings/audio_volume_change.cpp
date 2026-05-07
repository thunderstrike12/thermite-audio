#include "audio_volume_change.hpp"
void game::AudioVolumeChangeComponent::update(const tmt::FrameData& time) {
    auto t { tmt::engine.frame_data().elapsed_time };
    volume_control.set_volume((std::sin(t) + 1.0f) / 2.0f);
}