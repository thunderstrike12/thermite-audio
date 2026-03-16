#pragma once

#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/data_headers/events.hpp"
#include "projects/game/components/development_tools/debug_line_helper.hpp"
namespace game {

class MoverComponent : public tmt::GameComponent<MoverComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "MoverComponent"; }

    void start() override {};
    void update(const tmt::FrameData& time) override {}
    void end() override {};
    void draw_debug_lines() const;
    void update_movement(const TriggerMovementEvent& event);
    float movement_speed = 1.0f;
    bool can_move = true;
    DebugLineConfig cfg;

    // this has to be the same entity that triggered the event
    tmt::Entity trigger_entity;

   private:
};

}  // namespace game
TMT_OBJECT(game::MoverComponent, (movement_speed, can_move, cfg, trigger_entity));
