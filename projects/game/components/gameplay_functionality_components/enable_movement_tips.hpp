#pragma once

#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/data_headers/events.hpp"
#include "engine/tools/types/bezier_curve.hpp"
namespace game {

class EnableMovementTips : public tmt::GameComponent<EnableMovementTips> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "EnableMovementTips"; }
    void start() override {};
    void update(const tmt::FrameData& time) override {};
    void end() override {};

    void enable_entities_in_order() const;
    std::vector<tmt::Entity> movement_tips_entities {};

   private:
};

}  // namespace game
TMT_GAME_COMPONENT(game::EnableMovementTips, (movement_tips_entities));
