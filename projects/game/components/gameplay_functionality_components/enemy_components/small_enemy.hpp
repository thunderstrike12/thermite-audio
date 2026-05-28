#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/explosion.hpp"
#include "engine/core/entity.hpp"
#include "engine/core/audio.hpp"
#include "../../../editor/all.hpp"

namespace game {

struct LogicParameters {
    float activation_range = 50.f;     // Range it will start to follow you

    float min_explosion_range = 5.f;   // Minimum explosion range
    float max_explosion_range = 10.f;  // Maximum explosion range
    float charge_time = 1.5f;          // Time it takes to charge up the explosion

    float explosion_damage = 10.f;     // Damage the explosion does to the player
    float explosion_force = 10.f;      // force of which it pushes objects away
    float push_radius = 10.f;          // radius where it pushes physics objects away
};

struct MovementParameters {
    float max_speed = 5.f;             // Max movement speed
    float max_force = 20.f;            // Max turn speed
    float arrive_radius = 6.f;         // Radius it will start to slow down at
    float wander_radius_limit = 25.f;  // Max distance it can wander
};

struct SmallEnemySounds {
    tmt::AudioEvent idle_chatter_audio;
    glm::vec2 chatter_play_intervals;
    tmt::AudioEvent aggroed_audio;
    tmt::AudioEvent explosion_charge_audio;
    tmt::AudioParameter explode_audio_param;
};

class SmallEnemy : public tmt::GameComponent<SmallEnemy> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Small Enemy"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override {}

    void die(tmt::Entity agent);
    void play_aggro_sound() const;

    tmt::Entity core = entt::null;
    tmt::Entity explosion = entt::null;
    tmt::Entity thermite = entt::null;

    ExplosionParameters explosion_parameters;
    LogicParameters logic_paramaters;
    MovementParameters movement_paramaters;

    SmallEnemySounds sound_parameters;

    bool core_destroyed = false;
    tmt::AudioInstance3D explosion_audio_instance;
    float next_chatter_time = 0.0f;

    bool in_active_range = false;

   private:
    uint32_t core_voxels = 0u;
};

}  // namespace game
TMT_OBJECT(game::LogicParameters, (min_explosion_range, max_explosion_range, charge_time, explosion_damage, explosion_force, activation_range, push_radius));
TMT_OBJECT(game::MovementParameters, (max_speed, max_force, arrive_radius, wander_radius_limit));
TMT_OBJECT(game::SmallEnemySounds, (idle_chatter_audio, chatter_play_intervals, aggroed_audio, explosion_charge_audio, explode_audio_param));
TMT_GAME_COMPONENT(game::SmallEnemy, (core, explosion, thermite, explosion_parameters, logic_paramaters, movement_paramaters, sound_parameters));
