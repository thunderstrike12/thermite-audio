#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/data_headers/events.hpp"
#include "engine/tools/types/bezier_curve.hpp"
namespace game {

enum class MovementTypes { WASD, UP_DOWN, BOOST, BREAK };
class MovementTip : public tmt::GameComponent<MovementTip> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "MovementTip"; }
    void start() override {};
    void update(const tmt::FrameData& time) override;
    void end() override {};

    void try_to_enable();
    void apply_enable_disable();

    float time_to_fade = 1.0f;
    float percentage_for_starting_to_fade = .2f;
    std::vector<tmt::Entity> ui_entities {};
    std::vector<tmt::Entity> enable_entities_on_fade_out {};
    std::vector<tmt::Entity> disable_entities_on_fade_out {};
    MovementTypes movement_tip_type { MovementTypes::WASD };

   private:
    float elapsed_time = 0.0f;
    bool first_time = true;
};

}  // namespace game
TMT_GAME_COMPONENT(game::MovementTip, (time_to_fade, percentage_for_starting_to_fade, ui_entities, enable_entities_on_fade_out, disable_entities_on_fade_out, movement_tip_type));
