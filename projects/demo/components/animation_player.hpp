#pragma once
#include <vector>
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/io.hpp"

class AnimationPlayer : public tmt::GameComponent<AnimationPlayer> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "AnimationPlayer"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    tmt::IO::FileLocation rig_location = {};
    std::vector<tmt::IO::FileLocation> animation_location;
    std::string animation_name = "";
};

TMT_OBJECT(AnimationPlayer, (rig_location, animation_location, animation_name));