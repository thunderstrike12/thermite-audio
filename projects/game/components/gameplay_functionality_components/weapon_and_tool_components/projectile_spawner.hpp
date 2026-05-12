#pragma once
#include "projects/game/data_headers/events.hpp"
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class ProjectileSpawner : public tmt::GameComponent<ProjectileSpawner> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name();

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    entt::entity animated_tool_entity;  // entity with the animation state machine to animate

   private:
    void on_shoot(const WeaponFiredEvent& e) const;
    tmt::Entity player_entity = entt::null;
};

}  // namespace game

TMT_OBJECT(game::ProjectileSpawner, (animated_tool_entity));
