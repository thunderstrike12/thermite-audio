#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/data_headers/events.hpp"
#include "engine/tools/types//bezier_curve.hpp"
namespace game {

void trigger_icon_fade(const std::vector<entt::entity>& ui_entities, float start_time, float time_to_fade, float min_value, bool fade_out = false);

class OpacityFader : public tmt::GameComponent<OpacityFader> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "OpacityFader"; }
    void start() override;
    void update(const tmt::FrameData& time) override {};
    void end() override;

    tmt::BezierCurve curve;

   private:
    void on_transition(const game::IconTransitionEvent& event) const;
};

}  // namespace game
TMT_GAME_COMPONENT(game::OpacityFader, (curve));
