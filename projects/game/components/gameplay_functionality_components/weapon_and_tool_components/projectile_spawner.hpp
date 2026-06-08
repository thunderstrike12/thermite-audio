#pragma once

#include "projects/game/data_headers/events.hpp"

#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/audio.hpp"

#include "engine/core/audio.hpp"

namespace game {

struct RifleSounds {
    tmt::AudioEvent rifle_impact;
    tmt::AudioEvent rifle_shoot;  // done
};

class ProjectileSpawner : public tmt::GameComponent<ProjectileSpawner> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name();

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    entt::entity animated_tool_entity;  // entity with the animation state machine to animate

    RifleSounds sounds;

   private:
    void on_shoot(const WeaponFiredEvent& e) const;
    tmt::Entity player_entity = entt::null;
};

}  // namespace game
TMT_OBJECT(game::RifleSounds, (rifle_impact, rifle_shoot));
TMT_GAME_COMPONENT(game::ProjectileSpawner, (animated_tool_entity, sounds));
