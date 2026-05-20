#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/tools/types/bezier_curve.hpp"
#include "engine/shared/ray.hpp"
#include "projects/game/data_headers/events.hpp"
#include "projects/game/data_headers/layer_mask.hpp"

namespace game {

struct PlayerMovement {
    float acceleration = 20.0f;
    float max_speed = 10.0f;
    float boost_max_speed_multiplier = 2.0f;
};
struct PlayerStat {
    float max_value = 100.f;
    float value = 100.f;
    float increase_multiplier = 1.0f;
};
struct PlayerRecharge {
    float recharge_distance = 20.0f;
    float out_of_energy_time_till_death = 10.0f;
};
struct RayCollisionCheck {
    LayerMask collision_layer {};

    float player_radius = 0.3f;

    float collision_speed_damping = 0.4f;
    float camera_near_distance = 0.3f;
};
enum class PlayerState { FREEMOVING, ATTACHED, PAUSED };
struct CameraShakeSettings {
    // Camera shake
    bool enabled = true;          // global toggle for designers
    float max_intensity = 0.25f;  // absolute clamp
    float boost_intensity = 0.015f;
    float drill_intensity = 0.05f;
    float decay_speed = 5.0f;

    // recoil
    float recoil_strength = 2.0f;      // how high it kicks UP
    float recoil_return_speed = 5.0f;  // how fast it goes DOWN
    float recoil_horizontal = 0.2f;    // side randomness
    float get_max_intensity() const { return max_intensity * multiplier; }
    float get_recoil_return_speed() const { return recoil_return_speed * multiplier; }
    float get_boost_intensity() const { return boost_intensity * multiplier; }
    float get_decay_speed() const { return decay_speed * multiplier; }
    float get_recoil_strength() const { return recoil_strength * multiplier; }
    float get_recoil_horizontal() const { return recoil_horizontal * multiplier; }
    float get_drill_intensity() const { return drill_intensity * multiplier; }

    void set_multiplier(float _multiplier) { multiplier = _multiplier; }

   private:
    // loaded from the saved data
    float multiplier { 1.0f };
};

class Player : public tmt::GameComponent<Player> {
   public:
    using GameComponent::GameComponent;

    static Player& get();

    static std::string_view get_name() { return "Player"; }

    void load_upgrades();
    void start() override;
    void look_camera();
    tmt::Hit check_collision() const;
    void apply_impulse(const glm::vec3& direction, float force);
    void resolve_penetration();
    void prevent_camera_clip() const;
    void move_player();
    void update(const tmt::FrameData& time) override;
    void draw_debug_lines() const override;
    void attempt_attach(tmt::Input& input);
    void on_attach(const AttachEvent& event);
    void end() override;
    float camera_sensitivity = 0.1f;

    // Movement parameters
    float acceleration = 20.0f;
    float deceleration = 40.0f;
    float drag = 5.0f;
    float max_speed = 10.0f;

    // Movement boost parameters
    float boost_max_speed_multiplier = 2.0f;
    float boost_acceleration_multiplier = 4.0f;
    float boost_cost_per_second_per_additional_speed_above_max = 1.0f;
    float boost_initial_cost = 5.0f;
    float boost_deceleration_factor = 5.0f;

    // Player stats
    PlayerStat health;
    PlayerStat energy;
    float energy_drain_per_second = 0.5f;
    float out_of_energy_time_till_death = 10.0f;
    tmt::BezierCurve black_out_curve;

    // Helper functions
    tmt::Transform& get_transform() const { return tmt::engine.ecs.get_component<tmt::Transform>(entity); }
    tmt::Camera& get_camera() const { return tmt::engine.ecs.get_component<tmt::Camera>(entity); }
    void set_state(PlayerState new_state) { state = new_state; };
    glm::vec3 get_velocity() const { return velocity; };

    // HUD entities
    tmt::Entity player_hud = entt::null;
    tmt::Entity barge_hud = entt::null;
    tmt::Entity low_energy_hud = entt::null;
    tmt::Entity black_out_hud = entt::null;

    // Energy pop up settings
    float low_energy_threshold = 0.3f;    // 30 percent, 0.0/1.0
    float low_energy_duration = 2.0f;     // seconds

    bool use_second_warning = false;
    float low_energy_threshold_2 = 0.1f;  // 10%
    float low_energy_duration_2 = 2.0f;

    // Runtime energy pop up
    float low_energy_timer = 0.0f;
    bool low_energy_active = false;
    bool was_above_threshold = true;

    float low_energy_timer_2 = 0.0f;
    bool low_energy_active_2 = false;
    bool was_above_threshold_2 = true;

    tmt::Entity boost_availability = entt::null;

    // Crosshair entities
    tmt::Entity rifle_crosshair = entt::null;
    tmt::Entity gravity_crosshair = entt::null;
    tmt::Entity mine_crosshair = entt::null;

    // Barge point
    tmt::Entity barge = entt::null;

    float recharge_distance = 20.0f;
    RayCollisionCheck ray_check;
    bool player_ended_run = false;

    void add_camera_shake(float intensity);
    void add_recoil();

    // screen shake
    CameraShakeSettings camera_shake_settings;
    float current_shake = 0.0f;

    float base_yaw = 0.0f;
    float base_pitch = 0.0f;
    glm::vec2 recoil_offset = glm::vec2(0.0f);

   private:
    void refill(float delta);
    void drain_energy(float delta);
    void reset_action_time(std::string_view action_name);
    void apply_boost();
    void reset_boost(float delta_time);
    void set_hud_enabled(tmt::Entity hud_root, bool enabled);
    void update_shake(float dt);
    void set_crosshair(tmt::Entity active);

    PlayerState state = PlayerState::FREEMOVING;
    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };
    glm::vec3 input_dir { 0.0f };
    float max_speed_calculated = max_speed;
    float acceleration_calculated = acceleration;

    bool boost_was_applied = false;
    float previous_max_health = health.max_value;
    float previous_health = health.value;
    float previous_max_energy = energy.max_value;
    float previous_energy = energy.value;
    float out_of_energy_timer = 0.0f;
};

}  // namespace game
TMT_OBJECT(game::PlayerStat, (max_value, value, increase_multiplier));
TMT_OBJECT(game::PlayerMovement, (acceleration, max_speed, boost_max_speed_multiplier));
TMT_OBJECT(game::PlayerRecharge, (recharge_distance, out_of_energy_time_till_death));
TMT_OBJECT(game::RayCollisionCheck, (collision_layer, player_radius, collision_speed_damping, camera_near_distance));
TMT_OBJECT(game::CameraShakeSettings, (enabled, max_intensity, boost_intensity, drill_intensity, decay_speed, recoil_strength, recoil_return_speed, recoil_horizontal));
TMT_GAME_COMPONENT(
    game::Player, (camera_sensitivity, acceleration, deceleration, drag, max_speed, boost_max_speed_multiplier, boost_acceleration_multiplier, boost_deceleration_factor,
                   boost_cost_per_second_per_additional_speed_above_max, boost_initial_cost, boost_availability, health, energy, energy_drain_per_second, out_of_energy_time_till_death,
                   low_energy_threshold, low_energy_duration, use_second_warning, low_energy_threshold_2, low_energy_duration_2, player_hud, barge_hud, low_energy_hud, black_out_hud, black_out_curve, rifle_crosshair,
                   gravity_crosshair, mine_crosshair, barge, recharge_distance, ray_check, camera_shake_settings)
);
