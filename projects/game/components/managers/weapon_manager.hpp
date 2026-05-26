#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/data_headers/events.hpp"
namespace game {

enum class WeaponType { RIFLE, GRAVITY, MINING };
class WeaponManager : public tmt::GameComponent<WeaponManager> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "WeaponManager"; }

    WeaponType get_active_weapon() const { return current_weapon; }

    void switch_to(WeaponType weapon_slot);
    void set_new_weapon(game::WeaponType weapon_slot);
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    void subscribe_weapon(WeaponType slot);
    void unsubscribe_weapon(WeaponType slot);
    void draw_debug_lines() const override;
    void on_weapon_shoot(const WeaponFiredEvent& event);

    struct MotionParams {
        float frequency = 1.f;
        float damping = 1.f;
        float initial_response = 0.f;
    };

    struct WeaponProcAnimData {
        tmt::Entity root = entt::null;

        struct OverlapParams {
            float start_offset = 0.f;
            float check_length = 0.5f;
            float scan_extent = 0.1f;
            float length_to_shoulder = 0.1f;
            glm::vec3 shoulder_eulers {};
        } anti_overlap;
        struct SwayParams {
            float look_displacement = 1.f;
            float look_max_rotation_factor = 0.3f;
            float look_sensitivity = 1.f;
            float look_roll_factor = 1.f;
            float displacement_player_acceleration_multiplier = 0.1f;
            float displacement_player_velocity_multiplier = 0.1f;

            MotionParams displacement_motion;
            MotionParams rotational_motion;
        } sway;
        struct RecoilParams {
            float upward_allowed_time = 0.3f;
            float rot_time_offset = 0.1f;
            glm::vec3 angle_impulse {};
            glm::vec2 min_max_roll_deviation {};
            glm::vec2 min_max_yaw_deviation {};
            glm::vec3 pos_impulse {};
            MotionParams displacement_motion;
            MotionParams rotational_motion;
            MotionParams out_displacement_motion;
            MotionParams out_rotational_motion;
        } recoil;
    };
    struct WeaponEntry {
        WeaponProcAnimData proc_anim_data;
        tmt::Entity entity = entt::null;
    };

    std::unordered_map<WeaponType, WeaponEntry> weapons;
    WeaponType starting_weapon { WeaponType::RIFLE };

    tmt::Entity shooting_entity = entt::null;

    float overheat_time = .8f;
    float switching_time = .5f;

    WeaponType last_used_weapon = WeaponType::RIFLE;  // default ?
    tmt::Entity tool_rig = entt::null;

   private:
    void complete_switch();
    float overheat_remaining_time = -0.1f;
    float switching_remaining_time = -0.1f;
    bool switching = false;
    /// <summary>
    /// Handles the weapon overheat event when a weapon is fired.
    /// </summary>
    /// <param name="event">The weapon fired event containing information about the fired weapon.</param>
    void on_overheat(const WeaponFiredEvent& event);
    void check_trigger_shoot_event();
    void transition_to_other_weapons();
    void show_weapon_tips(game::WeaponType slot);
    WeaponType current_weapon;
    // weapon that is being switched to by the player
    WeaponType pending_weapon;

    // Weapon procanim related
    glm::vec3 target_pos;
    struct RotTrans {
        glm::vec3 translation;
        glm::quat rotation;
    };
    float last_pitch = 0.f;
    float last_yaw = 0.f;
    std::unordered_map<WeaponType, RotTrans> rest_poses;
    RotTrans animate_antioverlap();
    RotTrans animate_sway(RotTrans input_pose, tmt::Transform* root_eff_transform, const WeaponProcAnimData& weapon_procanim_data);
    void update_procedural_motion(float dt);
    void on_game_paused(const game::GamePausedEvent& event);
    void on_game_unpaused(const game::GameUnpausedEvent& event);

    bool fired = false;
    bool updating_recoil_impulse = false;
    bool upward_response = false;
    float shot_rand_roll;
    float shot_rand_yaw;
    RotTrans recoil_impulse { .translation = glm::vec3 { 0.f }, .rotation = glm::identity<glm::quat>() };
};

}  // namespace game
TMT_OBJECT(game::WeaponManager::MotionParams, (frequency, damping, initial_response));

TMT_OBJECT(game::WeaponManager::WeaponProcAnimData::OverlapParams, (start_offset, check_length, scan_extent, length_to_shoulder, shoulder_eulers));
TMT_OBJECT(
    game::WeaponManager::WeaponProcAnimData::SwayParams, (look_displacement, look_max_rotation_factor, look_sensitivity, look_roll_factor, displacement_player_acceleration_multiplier,
                                                          displacement_player_velocity_multiplier, displacement_motion, rotational_motion)
);
TMT_OBJECT(
    game::WeaponManager::WeaponProcAnimData::RecoilParams, (upward_allowed_time, rot_time_offset, angle_impulse, min_max_roll_deviation, min_max_yaw_deviation, pos_impulse,
                                                            displacement_motion, rotational_motion, out_displacement_motion, out_rotational_motion)
);
TMT_OBJECT(game::WeaponManager::WeaponProcAnimData, (root, anti_overlap, sway, recoil));
TMT_OBJECT(game::WeaponManager::WeaponEntry, (proc_anim_data, entity));
TMT_GAME_COMPONENT(game::WeaponManager, (tool_rig, weapons, starting_weapon, shooting_entity, overheat_time, switching_time));
