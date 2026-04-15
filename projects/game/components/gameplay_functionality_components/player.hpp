#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/shared/ray.hpp"
#include "projects/game/data_headers/events.hpp"
#include "projects/game/data_headers/layer_mask.hpp"

namespace game {

struct PlayerStat {
    float max_value = 100.f;
    float value = 100.f;
    float increase_multiplier = 1.0f;
};
struct RayCollisionCheck {
    LayerMask collision_layer {};

    float player_radius = 0.3f;

    float collision_speed_damping = 0.4f;
    float camera_near_distance = 0.3f;
};
enum class PlayerState { FREEMOVING, ATTACHED, PAUSED };

class Player : public tmt::GameComponent<Player> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Player"; }

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

    // Helper functions
    tmt::Transform& get_transform() const { return tmt::engine.ecs.get_component<tmt::Transform>(entity); }
    tmt::Camera& get_camera() const { return tmt::engine.ecs.get_component<tmt::Camera>(entity); }
    void set_state(PlayerState new_state) { state = new_state; };

    // UI
    tmt::Entity hp_bar_max = entt::null;
    tmt::Entity hp_bar_current = entt::null;

    tmt::Entity energy_bar_max = entt::null;
    tmt::Entity energy_bar_current = entt::null;

    tmt::Entity boost_availability = entt::null;

    // Barge point
    tmt::Entity barge = entt::null;

    float recharge_distance = 20.0f;
    RayCollisionCheck ray_check;
    bool player_ended_run = false;

   private:
    void refill(float delta);
    void drain_energy(float delta);
    void reset_action_time(std::string_view action_name);
    void apply_boost();
    void reset_boost(float delta_time);
    PlayerState state = PlayerState::FREEMOVING;
    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };
    glm::vec3 input_dir { 0.0f };
    float max_speed_calculated = max_speed;
    float acceleration_calculated = acceleration;

    bool boost_was_applied = false;
    bool boost_available = false;
    float previous_max_health = health.max_value;
    float previous_health = health.value;
    float previous_max_energy = energy.max_value;
    float previous_energy = energy.value;
    float out_of_energy_timer = 0.0f;
};

}  // namespace game
TMT_OBJECT(game::PlayerStat, (max_value, value, increase_multiplier));
TMT_OBJECT(game::RayCollisionCheck, (collision_layer, player_radius, collision_speed_damping, camera_near_distance));
TMT_OBJECT(
    game::Player, (camera_sensitivity, acceleration, deceleration, drag, max_speed, boost_max_speed_multiplier, boost_acceleration_multiplier, boost_deceleration_factor,
                   boost_cost_per_second_per_additional_speed_above_max, boost_initial_cost, boost_availability, health, energy, energy_drain_per_second, out_of_energy_time_till_death,
                   hp_bar_max, hp_bar_current, energy_bar_max, energy_bar_current, barge, recharge_distance, ray_check)

);
