#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/tools/types/bezier_curve.hpp"
#include "engine/shared/ray.hpp"
#include "engine/tools/tweening.hpp"
#include "engine/tools/tweening/transform.hpp"
#include "projects/game/data_headers/events.hpp"
#include "projects/game/data_headers/layer_mask.hpp"
#include "engine/core/components/audio_emitter.hpp"

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

struct PlayerVFXSettings {
    tmt::ResourceRef<tmt::Json> recharge_vfx_prefab;
};

enum class PlayerState { FREEMOVING, ATTACHED, PAUSED };
struct AttachedCameraSettings {
    float distance = 12.0f;
    float height_offset = 3.0f;
    float look_at_height_offset = 1.5f;
    float rotation_sensitivity = 0.08f;
    float pitch_min = -25.0f;
    float pitch_max = 45.0f;
    float default_yaw = 0.0f;
    float default_pitch = 15.0f;
    float fov = 75.0f;
};
struct DetachCameraTransitionSettings {
    bool enabled = true;
    bool align_player_to_camera = true;
    float duration = 0.6f;
    Tweening::Ease ease = Tweening::Ease::IN_OUT_SINE;
};
struct AttachCameraTransitionSettings {
    bool enabled = true;
    bool play_on_first_attach = false;
    bool inherit_player_facing_on_attach = true;
    bool blend_look_over_tween = true;
    float start_look_blend_time = 0.15f;
    float duration = 0.6f;
    Tweening::Ease ease = Tweening::Ease::IN_OUT_SINE;
};
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

    float recoil_frequency = 1.f;
    float recoil_damping = 0.2f;
    float recoil_initial_response = 0.f;

    float shake_frequency = 1.f;
    float shake_damping = 0.2f;
    float shake_initial_response = 0.f;

    float damage_shake_multiplier = 1.f;

   private:
    // loaded from the saved data
    float multiplier { 1.0f };
};
struct PlayerSounds {
    tmt::AudioEvent destroyed_death;     // done
    tmt::AudioEvent drone_boost;         // done
    tmt::AudioEvent hit;                 // done
    tmt::AudioEvent battery_half;        // done
    tmt::AudioEvent battery_almost_out;  // done
    tmt::AudioEvent battery_out;         // done
};

enum class ToolType { RIFLE, GRAVITY, MINING };

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
    void take_damage(float damage);
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
    float distance_traveled = 0.0f;

    // Helper functions
    tmt::Transform& get_transform() const { return tmt::engine.ecs.get_component<tmt::Transform>(entity); }
    tmt::Camera& get_camera() const { return tmt::engine.ecs.get_component<tmt::Camera>(entity); }
    PlayerState get_state() { return state; };
    void set_state(PlayerState new_state) { state = new_state; };
    PlayerState get_state_before_pause() { return state_before_pause; };
    void set_state_before_pause(PlayerState new_state) { state_before_pause = new_state; }
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

    PlayerVFXSettings player_vfx_settings;
    // Barge point
    tmt::Entity barge = entt::null;

    float recharge_distance = 20.0f;
    RayCollisionCheck ray_check;
    bool player_ended_run = false;

    void add_camera_shake(float intensity);
    void add_recoil();

    // screen shake
    CameraShakeSettings camera_shake_settings;
    AttachedCameraSettings attached_camera_settings;
    AttachCameraTransitionSettings attach_camera_transition_settings;
    DetachCameraTransitionSettings detach_camera_transition_settings;
    float current_shake = 0.0f;

    float base_yaw = 0.0f;
    float base_pitch = 0.0f;
    glm::vec2 recoil_offset = glm::vec2(0.0f);

    void set_hud_enabled(tmt::Entity hud_root, bool enabled);

    // Sounds
    tmt::AudioEmitter* audio_emitter;
    PlayerSounds player_sounds;

   private:
    void refill(float delta);
    void drain_energy(float delta);
    void reset_action_time(std::string_view action_name);
    void apply_boost();
    void reset_boost(float delta_time);
    void update_shake(float dt);
    void set_crosshair(tmt::Entity active);
    void ensure_attached_camera();
    void update_attached_camera();
    void start_attach_camera_transition();
    void start_detach_camera_transition();
    void align_player_camera_to_attached();
    void sync_attached_orbit_from_camera();
    void setup_vfx_emitter(tmt::Entity& entity_to_set_up, tmt::ResourceRef<tmt::Json>& prefab_to_set_up);
    glm::vec3 get_attached_camera_orbit_position(const glm::vec3& barge_pos) const;

    PlayerState state = PlayerState::FREEMOVING;
    PlayerState state_before_pause;
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
    tmt::Entity attached_camera_entity = entt::null;
    float attached_camera_yaw = 0.0f;
    float attached_camera_pitch = 0.0f;
    bool attach_camera_transition_active = false;
    bool detach_camera_transition_active = false;
    bool has_attached_before = false;
    float attach_transition_look_blend_elapsed = 0.0f;
    bool attach_transition_start_forward_set = false;
    glm::vec3 attach_transition_start_forward { 0.0f, 0.0f, 1.0f };

    // sound variables
    bool out_of_energy = false;
    tmt::AudioInstance destroyed_death_instance;
    bool player_destroyed = false;

    ToolType active_tool = ToolType::RIFLE;

    tmt::Entity recharge_emitter_entity = entt::null;
};

}  // namespace game
TMT_OBJECT(game::PlayerStat, (max_value, value, increase_multiplier));
TMT_OBJECT(game::PlayerMovement, (acceleration, max_speed, boost_max_speed_multiplier));
TMT_OBJECT(game::PlayerRecharge, (recharge_distance, out_of_energy_time_till_death));
TMT_OBJECT(game::RayCollisionCheck, (collision_layer, player_radius, collision_speed_damping, camera_near_distance));
TMT_OBJECT(game::AttachedCameraSettings, (distance, height_offset, look_at_height_offset, rotation_sensitivity, pitch_min, pitch_max, default_yaw, default_pitch, fov));
TMT_OBJECT(game::AttachCameraTransitionSettings, (enabled, play_on_first_attach, inherit_player_facing_on_attach, blend_look_over_tween, start_look_blend_time, duration, ease));
TMT_OBJECT(game::DetachCameraTransitionSettings, (enabled, align_player_to_camera, duration, ease));
TMT_OBJECT(
    game::CameraShakeSettings, (enabled, max_intensity, boost_intensity, drill_intensity, decay_speed, recoil_strength, recoil_return_speed, recoil_horizontal, recoil_frequency,
                                recoil_damping, recoil_initial_response, shake_frequency, shake_damping, shake_initial_response, damage_shake_multiplier)
);
TMT_OBJECT(game::PlayerSounds, (destroyed_death, drone_boost, hit, battery_half, battery_almost_out, battery_out));
TMT_OBJECT(game::PlayerVFXSettings, (recharge_vfx_prefab));
TMT_GAME_COMPONENT(
    game::Player, (camera_sensitivity, acceleration, deceleration, drag, max_speed, boost_max_speed_multiplier, boost_acceleration_multiplier, boost_deceleration_factor,
                   boost_cost_per_second_per_additional_speed_above_max, boost_initial_cost, boost_availability, health, energy, energy_drain_per_second, out_of_energy_time_till_death,
                   low_energy_threshold, low_energy_duration, use_second_warning, low_energy_threshold_2, low_energy_duration_2, player_hud, barge_hud, low_energy_hud, black_out_hud,
                   black_out_curve, rifle_crosshair, gravity_crosshair, mine_crosshair, player_vfx_settings, barge, recharge_distance, ray_check, camera_shake_settings,
                   attached_camera_settings, attach_camera_transition_settings, detach_camera_transition_settings, player_sounds)
);
