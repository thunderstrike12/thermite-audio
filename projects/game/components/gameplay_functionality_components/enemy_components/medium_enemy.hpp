#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/entity.hpp"
#include "../../../editor/all.hpp"
#include "projects/game/data_headers/events.hpp"
#include "engine/core/components/rig_controller.hpp"
#include "engine/core/components/audio_emitter.hpp"

#include "projects/game/components/managers/ore_manager.hpp"
#include "projects/game/data_headers/layer_mask.hpp"

namespace game {

struct AvailablePosEntry {
    tmt::Entity entity = entt::null;
    float height_offset = 0.0f;
    tmt::Entity reference_entity = entt::null;
    glm::vec3 stored_offset_from_ref_entity = glm::vec3(0, 0, 0);
    glm::vec3 base_offset_from_ref_entity = glm::vec3(0, 0, 0);
};

struct Sounds {
    tmt::AudioEvent sound_aggroed_audio;      // done
    tmt::AudioEvent sound_laser;              // done
    tmt::AudioParameter laser_parameter;
    tmt::AudioEvent sound_missile_explosion;  // done
    tmt::AudioEvent sound_missile_fire;       // done
    tmt::AudioEvent sound_random_chatter;     // not yet implemented anywhere, where do designers want this?
    tmt::AudioEvent sound_shield_slam;        // done
    tmt::AudioEvent sound_taunt;              // not yet implemented, enemy doesnt have a taunt, where do designers want this?
    tmt::AudioEvent sound_walk;               // done
    tmt::AudioEvent sound_core_destroyed;     // done
};

class MediumEnemy : public tmt::GameComponent<MediumEnemy> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Medium Enemy"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void kite_player() const;
    void die();
    void set_stored_offsets_to_ref_entity();
    void set_stored_offsets_to_base();

    void on_game_paused(const game::GamePausedEvent&);
    void on_game_unpaused(const game::GameUnpausedEvent&);

    bool paused = false;
    bool core_destroyed = false;

    tmt::Entity player = entt::null;
    tmt::Entity walkable_asteroid = entt::null;
    tmt::Entity core = entt::null;
    uint32_t core_voxels = 0u;
    tmt::Entity rig_controller = entt::null;
    std::array<AvailablePosEntry, 4> available_positions;
    float available_pos_max_height_diff = 2.0f;
    float available_pos_min_height_diff = 2.0f;
    tmt::Entity missile_origin = entt::null;
    tmt::Entity laser_origin = entt::null;
    game::OreManager* ore_manager;
    LayerMask projectile_mask {};
    LayerMask enemy_mask {};
    LayerMask enemy_projectile_mask {};
    int projectile_layer = 4;
    int enemy_layer = 2;

    glm::vec3 velocity = glm::vec3(0, 0, 0);
    glm::quat rotation = glm::quat(1, 0, 0, 0);
    float rotation_speed = 2.5f;
    float height_above_ground = 0.2f;
    float height_above_ground_offset = 0.0f;
    float walk_speed = 5.0f;
    float back_off_distance = 5.0f;
    float velocity_of_objects_on_death = 2.0f;

    float aggro_range = 50.0f;
    float laser_range = 30.0f;
    float stomp_range = 5.0f;

    tmt::IO::FileLocation missile_voxel_object;

    float stop_launching_after = 0.5f;
    float start_homing_after = 0.5f;
    float launch_speed = 0.1f;
    float home_speed = 0.2f;
    float slow_homing_accuracy_after = 4.0f;
    float life_time = 0.2f;
    float missile_rotation_speed = 0.2f;
    glm::vec3 target_offset = glm::vec3(0, 0, 0);

    tmt::ResourceRef<tmt::Json> missile_vfx_prefab;
    float missile_cooldown = 2.0f;
    float missile_timer = 0.0f;
    int missile_burst = 3;
    float burst_interval = 0.2f;
    float missile_max_randomness = 0.1f;
    float missile_explosion_radius = 1.0f;
    std::vector<tmt::Entity> missile_voxel_entities;
    int missile_voxel_to_lose = 0;
    int missile_voxels = 0;

    tmt::IO::FileLocation laser_charge_voxel_object;
    tmt::IO::FileLocation laser_voxel_object;
    tmt::Entity laser_charge_particle_entity = entt::null;
    tmt::Entity laser_fire_particle_entity = entt::null;
    float laser_cooldown = 5.0f;
    float laser_timer = 0.0f;
    float rotation_speed_during_laser = 0.8f;
    float laser_sitting_down_time = 0.5f;
    float laser_winding_up_time = 0.5f;
    float laser_aim_tween_duration = 1.0f;
    float laser_sitting_down_height_offset = 0.2f;
    float laser_damage = 0.5f;
    float laser_damage_radius = 0.5f;
    glm::vec3 laser_target_offset = glm::vec3(0, 0, 0);
    float laser_firing_time = 1.0f;
    float laser_linear_speed = 0.0f;
    float laser_exponential_speed = 0.0f;
    float laser_linear_threshold = 0.5f;
    float laser_max_randomness = 1.0f;
    float laser_prediction_length = 0.0f;
    float laser_vel_smoothing = 0.25f;
    std::vector<tmt::Entity> laser_voxel_entities;
    int laser_voxel_to_lose = 0;
    int laser_voxels = 0;

    float stomp_cooldown = 3.0f;
    float stomp_timer = 0.0f;
    float stomp_windup = 0.5f;
    float stomp_radius = 1.0f;
    float stomp_damage = 0.5f;

    // sounds
    Sounds sounds;

    tmt::AudioInstance3D walk_instance;
    tmt::AudioInstance3D laser_instance;
    tmt::AudioInstance3D shield_slam_instance;

   private:
    // sounds variables
    bool played_aggro_sound = false;
};

}  // namespace game

TMT_OBJECT(game::AvailablePosEntry, (entity, height_offset, reference_entity));

TMT_OBJECT(
    game::Sounds,
    (sound_aggroed_audio, sound_laser, laser_parameter, sound_missile_explosion, sound_missile_fire, sound_random_chatter, sound_shield_slam, sound_taunt, sound_walk, sound_core_destroyed)
);

TMT_GAME_COMPONENT(
    game::MediumEnemy,
    (walkable_asteroid, laser_origin, missile_origin, core, rig_controller, available_positions, available_pos_max_height_diff, available_pos_min_height_diff, missile_voxel_object,
     projectile_mask, enemy_mask, enemy_projectile_mask, projectile_layer, enemy_layer, rotation_speed, height_above_ground, walk_speed, back_off_distance, velocity_of_objects_on_death,
     aggro_range, laser_range, stomp_range, missile_vfx_prefab, missile_cooldown, stop_launching_after, start_homing_after, slow_homing_accuracy_after, launch_speed, home_speed, life_time,
     missile_rotation_speed, target_offset, missile_burst, burst_interval, missile_max_randomness, missile_explosion_radius, missile_voxel_entities, missile_voxel_to_lose, laser_cooldown,
     rotation_speed_during_laser, laser_charge_voxel_object, laser_voxel_object, laser_charge_particle_entity, laser_fire_particle_entity, laser_firing_time, laser_sitting_down_time,
     laser_winding_up_time, laser_damage, laser_damage_radius, laser_target_offset, laser_linear_speed, laser_exponential_speed, laser_linear_threshold, laser_max_randomness,
     laser_prediction_length, laser_vel_smoothing, laser_voxel_entities, laser_voxel_to_lose, stomp_cooldown, stomp_timer, stomp_windup, stomp_radius, stomp_damage, sounds)
);
