#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/audio.hpp"
#include "engine/core/components/button.hpp"

namespace game {

class AudioVolumeChangeComponent : public tmt::GameComponent<AudioVolumeChangeComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "AudioVolumeChangeComponent"; }
    void start() override;
    void on_value_changed(tmt::Slider::Context context);

    void update(const tmt::FrameData& time) override;

    void end() override;

    tmt::VolumeControl volume_control;
};

}  // namespace game
TMT_COMPONENT_DEPENDENCIES(game::AudioVolumeChangeComponent, tmt::Slider);

TMT_GAME_COMPONENT(game::AudioVolumeChangeComponent, (volume_control));
