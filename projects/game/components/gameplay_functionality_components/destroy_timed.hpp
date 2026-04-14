#pragma once
#include "engine/systems/gameplay/game_component.hpp"
namespace game {

class DestroyTimed : public tmt::GameComponent<DestroyTimed> {
    using GameComponent::GameComponent;

   public:
    static std::string_view get_name() { return "Destroy Timed Component"; }
    void start() override {};
    void update(const tmt::FrameData& time) override;
    void end() override {};

    float time_to_destroy = 10.0f;
};

}  // namespace game
TMT_OBJECT(game::DestroyTimed, (time_to_destroy));
