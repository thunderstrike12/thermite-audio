#pragma once
#include "projects/game/data_headers/events.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/core/components/audio_emitter.hpp"

namespace game {

struct GravitySounds {
    tmt::AudioEvent gravity_hold_object;     // done
    tmt::AudioEvent graviry_launch_project;  // done
};

class GravityManipulationComponent : public tmt::GameComponent<GravityManipulationComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Gravity Manipulation Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    void draw_debug_lines() const override;
    void on_weapon_fired(const WeaponFiredEvent& e);

    float range = 2.0f;                    // size of the gravity zone
    float max_mass = 20.0f;                // max mass that will be able to be manipulated
    float pull_strength = 8.0f;            // the speed of the objects affected by the gravity zone
    float push_strength = 10.0f;           // how fast objects should be pushed away
    float attraction_acceleration = 0.5f;  // how quickly objects accelerate when they enter the gravity zone
    entt::entity attraction_point_entity;  // point of attraction
    entt::entity animated_tool_entity;     // entity with the animation state machine to animate

    GravitySounds sounds;

   private:
    std::vector<entt::entity> currently_manipulated_entities;
    tmt::Entity player = entt::null;
    void grav_point_check();
    void grav_attract();
    void grav_shoot();

    void apply_enemy_override();
    void clear_enemy_override();

    void on_release(const ReleaseShootEvent& e);

    bool gravity_shoot_sound_played = false;
    tmt::AudioInstance gravity_hold_instance;

    // TODO the same, this could be primarily handled through weapon, a second timer could be used for the second shot
};

}  // namespace game
TMT_OBJECT(game::GravitySounds, (gravity_hold_object, graviry_launch_project));
TMT_GAME_COMPONENT(game::GravityManipulationComponent, (range, max_mass, pull_strength, push_strength, attraction_acceleration, attraction_point_entity, animated_tool_entity, sounds));
