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

    float range = 5.0f;
    float forward_offset = 2.0f;
    float vertical_offset = -0.2f;
    float max_mass = 200.0f;
    float pull_strength = 20.0f;
    float push_strength = 40.0f;
    float attraction_acceleration = 0.1f;
    float push_cooldown = 1.0f;
    entt::entity attraction_point_entity;
    bool active = true;

   private:
    std::vector<entt::entity> currently_manipulated_entities;
    void grav_point_check();
    void grav_attract();
    void grav_shoot();
    float push_cooldown_counter = 0.0f;
};

}  // namespace game
TMT_OBJECT(game::GravityManipulationComponent, (range, max_mass, pull_strength, push_strength, attraction_acceleration, push_cooldown, attraction_point_entity, active));