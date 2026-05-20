#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/data_headers/events.hpp"
#include "engine/tools/types//bezier_curve.hpp"
namespace game {

class OpacityFader : public tmt::GameComponent<OpacityFader> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "OpacityFader"; }
    void start() override;
    void update(const tmt::FrameData& time) override {};
    void end() override;

    tmt::BezierCurve curve;

   private:
    void on_transition(const game::IconTransitionEvent& event);
};

}  // namespace game
TMT_GAME_COMPONENT(game::OpacityFader, (curve));
