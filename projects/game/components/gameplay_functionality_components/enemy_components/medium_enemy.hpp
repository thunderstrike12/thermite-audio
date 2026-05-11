#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/entity.hpp"
#include "../../../editor/all.hpp"
#include "engine/core/resources/stencil.hpp"
#include "projects/game/data_headers/events.hpp"

#include "projects/game/components/managers/ore_manager.hpp"
#include "projects/game/data_headers/layer_mask.hpp"

namespace game {

class MediumEnemy : public tmt::GameComponent<MediumEnemy> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Medium Enemy"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void kite_player() const;
    void die();

    void on_game_paused(const game::GamePausedEvent&);
    void on_game_unpaused(const game::GameUnpausedEvent&);

    bool paused = false;

    tmt::Entity player = entt::null;
    tmt::Entity walkable_asteroid = entt::null;
    tmt::Entity missile_origin = entt::null;
    tmt::Entity laser_origin = entt::null;
    game::OreManager* ore_manager;
    LayerMask projectile_mask {};
    int projectile_layer = 4;

    glm::vec3 velocity = glm::vec3(0, 0, 0);
    glm::quat rotation = glm::quat(1, 0, 0, 0);
    float rotation_speed = 2.5f;
    float height_above_ground = 0.2f;
    float height_above_ground_offset = 0.0f;
    float walk_speed = 5.0f;
    float back_off_distance = 5.0f;

    float aggro_range = 50.0f;
    float laser_range = 30.0f;
    float stomp_range = 5.0f;

    tmt::ResourceRef<tmt::Stencil> stencil;
    tmt::IO::FileLocation missile_voxel_object;

    float stop_launching_after = 0.5f;
    float start_homing_after = 0.5f;
    float launch_speed = 0.1f;
    float home_speed = 0.2f;
    float slow_homing_accuracy_after = 4.0f;
    float life_time = 0.2f;
    float missile_rotation_speed = 0.2f;
    glm::vec3 target_offset = glm::vec3(0, 0, 0);

    float missile_cooldown = 2.0f;
    float missile_timer = 0.0f;
    int missile_burst = 3;
    float burst_interval = 0.2f;
    float missile_max_randomness = 0.1f;
    float missile_explosion_radius = 1.0f;

    tmt::IO::FileLocation laser_charge_voxel_object;
    tmt::IO::FileLocation laser_voxel_object;
    float laser_cooldown = 5.0f;
    float laser_timer = 0.0f;
    float laser_sitting_down_time = 0.5f;
    float laser_winding_up_time = 0.5f;
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

    float stomp_cooldown = 3.0f;
    float stomp_timer = 0.0f;
    float stomp_windup = 0.5f;
    float stomp_radius = 1.0f;
    float stomp_damage = 0.5f;
};

}  // namespace game

TMT_OBJECT(
    game::MediumEnemy,
    (walkable_asteroid, laser_origin, missile_origin, stencil, missile_voxel_object, projectile_mask, projectile_layer, rotation_speed, height_above_ground, walk_speed, aggro_range,
     laser_range, stomp_range, missile_cooldown, stop_launching_after, start_homing_after, slow_homing_accuracy_after, launch_speed, home_speed, life_time, missile_rotation_speed,
     target_offset, missile_burst, burst_interval, missile_max_randomness, missile_explosion_radius, laser_cooldown, laser_charge_voxel_object, laser_voxel_object, laser_firing_time,
     laser_sitting_down_time, laser_winding_up_time, laser_damage, laser_damage_radius, laser_target_offset, laser_linear_speed, laser_exponential_speed, laser_linear_threshold,
     laser_max_randomness, laser_prediction_length, laser_vel_smoothing, stomp_cooldown, stomp_timer, stomp_windup, stomp_radius, stomp_damage)
);
