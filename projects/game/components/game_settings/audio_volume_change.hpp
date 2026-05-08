#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/audio.hpp"

namespace game {

class AudioVolumeChangeComponent : public tmt::GameComponent<AudioVolumeChangeComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "AudioVolumeChangeComponent"; }
    void start() override {};
    void update(const tmt::FrameData& time) override;
    void end() override {};

    tmt::VolumeControl volume_control;
};

}  // namespace game
TMT_OBJECT(game::AudioVolumeChangeComponent, (volume_control));
