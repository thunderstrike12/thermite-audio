#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/data_headers/events.hpp"
#include "engine/tools/types/bezier_curve.hpp"
namespace game {

class MovementTip : public tmt::GameComponent<MovementTip> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "MovementTip"; }
    void start() override{};
    void update(const tmt::FrameData& time) override;
    void end() override{};
    void on_entity_disabled() override;
    void on_entity_enabled() override;

    float time_to_fade = 1.0f;
    float percentage_for_starting_to_fade = .2f;
    tmt::Entity ui_entity = entt::null;

   private:
    float elapsed_time = 0.0f;
    bool first_time = true;
};

}  // namespace game
TMT_GAME_COMPONENT(game::MovementTip, (time_to_fade, percentage_for_starting_to_fade, ui_entity));
