#pragma once
#include "events.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"

namespace game {

class GravityManipulationComponent : public tmt::GameComponent<GravityManipulationComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Gravity Manipulation Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    float range = 2.0f;             // size of the gravity zone
    float max_mass = 200.0f;        // max mass that will be able to be manipulated
    float pull_strength = 8.0f;    // the speed of the objects affected by the gravity zone
    float push_strength = 10.0f;     // how fast objects should be pushed away
    float attraction_acceleration = 0.5f;   //how quickly objects accelerate when they enter the gravity zone
    float push_cooldown = 1.0f;     // how long it takes to push objects away again in seconds
    entt::entity attraction_point_entity;   // point of attraction
    bool active = true;         // self-explanatory
    bool input_active_on_non_player = false;  // if needs to be active on player input but is not on player entity

   private:
    std::vector<entt::entity> currently_manipulated_entities;
    void grav_point_check();
    void grav_attract();
    void grav_shoot();
    float push_cooldown_counter = 0.0f;
};

}  // namespace game
TMT_OBJECT(
    game::GravityManipulationComponent, (range, max_mass, pull_strength, push_strength, attraction_acceleration, push_cooldown, attraction_point_entity, active, input_active_on_non_player)
);