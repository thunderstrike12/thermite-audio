#include "weapon_manager.hpp"
#include "projects/game/data_headers/game_input.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/weapon.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/polyline.hpp"
#include "engine/tools/second_order_solver.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "engine/tools/random.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/norm.hpp"
#include "projects/game/components/ui_components/weapon_tip.hpp"

void game::WeaponManager::switch_to(WeaponType weapon_slot) {
    if (weapon_slot == current_weapon) {
        tmt::Log::info("[WeaponManager] Already on {}, ignoring switch", magic_enum::enum_name(current_weapon));
        return;
    }
    tmt::Log::info("[WeaponManager] Switching: {} -> {}", magic_enum::enum_name(current_weapon), magic_enum::enum_name(weapon_slot));

    // unsubscribe only the first time we are in a transition stage
    if (switching == false) {
        unsubscribe_weapon(current_weapon);
    }

    auto* rig_controller = tmt::engine.ecs.try_get_component<tmt::RigController>(tool_rig);
    if (rig_controller) {
        rig_controller->set_parameter_int("ToolType", static_cast<int>(weapon_slot));
        rig_controller->set_parameter_trigger("SwitchTool");
    } else {
        tmt::Log::error("[WeaponManager] Weapon entity is not set, can't animate transitions!");
    }

    // the new subscription happens when the switching is done
    switching_remaining_time = switching_time;
    pending_weapon = weapon_slot;
    switching = true;

    auto& weapon_procanim_data = weapons.at(weapon_slot).proc_anim_data;
    if (tmt::engine.ecs.valid(weapon_procanim_data.root)) {
        auto& root_eff_trans = tmt::engine.ecs.get_component<tmt::Transform>(weapon_procanim_data.root);

        target_pos = root_eff_trans.get_local_position();
    } else {
        tmt::Log::info("[WeaponManager] Weapon Proc Anim should have a root effector entity set!");
    }
}

void game::WeaponManager::set_new_weapon(game::WeaponType weapon_slot) {
    current_weapon = weapon_slot;
    subscribe_weapon(current_weapon);
}

void game::WeaponManager::start() {
    // Connect to the GamePausedEvent and GameUnpausedEvent
    tmt::engine.ecs.get_dispatcher().sink<game::GamePausedEvent>().connect<&WeaponManager::on_game_paused>(this);
    tmt::engine.ecs.get_dispatcher().sink<game::GameUnpausedEvent>().connect<&WeaponManager::on_game_unpaused>(this);
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().connect<&WeaponManager::on_weapon_shoot>(this);

    current_weapon = starting_weapon;
    tmt::Log::info("[WeaponManager] Starting with weapon: {}", magic_enum::enum_name(starting_weapon));

    auto shooter_player = tmt::engine.ecs.try_get_component<game::Player>(shooting_entity);
    if (shooter_player) {
        last_pitch = shooter_player->base_pitch;
        last_yaw = shooter_player->base_yaw;
    }

    for (auto& [type, weapon] : weapons) {
        auto& weapon_procanim_data = weapon.proc_anim_data;
        if (tmt::engine.ecs.valid(weapon_procanim_data.root)) {
            auto root_eff_trans = tmt::engine.ecs.try_get_component<tmt::Transform>(weapons[type].proc_anim_data.root);

            if (root_eff_trans) {
                if (type == current_weapon) {
                    target_pos = root_eff_trans->get_local_position();
                }
                rest_poses[type] = { .translation = root_eff_trans->get_local_position(), .rotation = root_eff_trans->get_local_rotation() };
            }
        }
    }

    subscribe_weapon(current_weapon);

    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().connect<&WeaponManager::on_overheat>(this);
}

void game::WeaponManager::end() {
    // Disconnect from the events
    tmt::engine.ecs.get_dispatcher().sink<game::GamePausedEvent>().disconnect<&WeaponManager::on_game_paused>(this);
    tmt::engine.ecs.get_dispatcher().sink<game::GameUnpausedEvent>().disconnect<&WeaponManager::on_game_unpaused>(this);

    tmt::Log::info("[WeaponManager] Ending, unsubscribing {}", magic_enum::enum_name(current_weapon));
    unsubscribe_weapon(current_weapon);
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().disconnect<&WeaponManager::on_overheat>(this);
}

void game::WeaponManager::on_overheat(const game::WeaponFiredEvent& event) {
    // Here as an example we overheat after a secondary shot, so we cannot do anything for less than a second
    if (event.secondary_shot) {
        overheat_remaining_time = overheat_time;
    }
}

void game::WeaponManager::check_trigger_shoot_event() {
    auto& input = tmt::engine.input;

    if (input.is_action_pressed(action::SHOOT)) {
        tmt::engine.ecs.get_dispatcher().trigger(ShootEvent { shooting_entity, false });

        auto* rig_controller = tmt::engine.ecs.try_get_component<tmt::RigController>(tool_rig);
        if (rig_controller) {
            rig_controller->set_parameter_bool("InUse", true);
        }

    } else if (input.is_action_just_released(action::SHOOT)) {
        tmt::engine.ecs.get_dispatcher().trigger(ReleaseShootEvent { shooting_entity });

        auto* rig_controller = tmt::engine.ecs.try_get_component<tmt::RigController>(tool_rig);
        if (rig_controller) {
            rig_controller->set_parameter_bool("InUse", false);
        }
    }
}

void game::WeaponManager::complete_switch() {
    current_weapon = pending_weapon;
    switching = false;
    subscribe_weapon(current_weapon);
}

void game::WeaponManager::update(const tmt::FrameData& time) {
    // TODO replace with proper state
    if (switching) {
        switching_remaining_time -= tmt::engine.frame_data().delta_time;

        if (switching_remaining_time < 0.0f) {
            complete_switch();
        }
    } else {
        switch (current_weapon) {
            case game::WeaponType::RIFLE:
                check_trigger_shoot_event();

                break;
            case game::WeaponType::GRAVITY:
                if (overheat_remaining_time < 0.0f) {
                    check_trigger_shoot_event();
                }
                if (tmt::engine.input.is_action_pressed(action::SECONDARY_TOOL_USE)) {
                    tmt::engine.ecs.get_dispatcher().trigger(ShootEvent { shooting_entity, true });
                }
                break;
            case game::WeaponType::MINING:
                check_trigger_shoot_event();

                break;
        }
    }

    update_procedural_motion(time.delta_time);

    transition_to_other_weapons();

    // state timers update
    if (overheat_remaining_time > 0.0f) {
        overheat_remaining_time -= tmt::engine.frame_data().delta_time;
    }
}
void game::WeaponManager::transition_to_other_weapons() {
    auto& input = tmt::engine.input;

    if (input.is_action_just_pressed(action::SWITCH_RIFLE)) {
        switch_to(WeaponType::RIFLE);
    } else if (input.is_action_just_pressed(action::SWITCH_MINING)) {
        switch_to(WeaponType::MINING);
    } else if (input.is_action_just_pressed(action::SWITCH_GRAVITY)) {
        switch_to(WeaponType::GRAVITY);
    }
}

void game::WeaponManager::show_weapon_tips(game::WeaponType slot) {
    auto view { tmt::engine.ecs.view<WeaponTip>(entt::exclude_t {}) };
    if (view.empty() == false) {
        for (auto tip : view) {
            std::get<0>(tip.components).on_weapon_switched(slot);
        }
    }
}
void game::WeaponManager::subscribe_weapon(WeaponType slot) {
    auto e = weapons.at(slot).entity;
    if (e == entt::null) {
        tmt::Log::warn("[WeaponManager] subscribe_weapon({}): entity is null", magic_enum::enum_name(slot));
        return;
    }
    tmt::engine.ecs.enable(e);
    auto* weapon = tmt::engine.ecs.try_get_component<Weapon>(e);
    if (!weapon) {
        tmt::Log::warn("[WeaponManager] subscribe_weapon({}): entity {} has no Weapon component", magic_enum::enum_name(slot), static_cast<uint32_t>(e));
        return;
    }
    tmt::Log::info("[WeaponManager] Subscribed {} (entity: {})", magic_enum::enum_name(slot), static_cast<uint32_t>(e));
    tmt::engine.ecs.get_dispatcher().sink<ShootEvent>().connect<&Weapon::on_shoot>(weapon);
    // trigger weapon tips
    show_weapon_tips(slot);
}

void game::WeaponManager::unsubscribe_weapon(WeaponType slot) {
    auto e = weapons.at(slot).entity;
    if (e == entt::null) {
        tmt::Log::warn("[WeaponManager] unsubscribe_weapon({}): entity is null", magic_enum::enum_name(slot));
        return;
    }
    tmt::engine.ecs.disable(e);
    auto* weapon = tmt::engine.ecs.try_get_component<Weapon>(e);
    if (!weapon) {
        tmt::Log::warn("[WeaponManager] unsubscribe_weapon({}): entity {} has no Weapon component", magic_enum::enum_name(slot), static_cast<uint32_t>(e));
        return;
    }
    tmt::Log::info("[WeaponManager] Unsubscribed {} (entity: {})", magic_enum::enum_name(slot), static_cast<uint32_t>(e));
    tmt::engine.ecs.get_dispatcher().sink<ShootEvent>().disconnect<&Weapon::on_shoot>(weapon);
    tmt::engine.ecs.get_dispatcher().trigger(ReleaseShootEvent { shooting_entity });
}

void game::WeaponManager::draw_debug_lines() const {
    auto& weapon_procanim_data = weapons.at(starting_weapon).proc_anim_data;

    auto root_effector_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(weapon_procanim_data.root);
    if (!root_effector_transform || rest_poses.empty()) {
        return;
    }
    auto parent = root_effector_transform->get_parent();
    auto& parent_world = tmt::engine.ecs.get_component<tmt::Transform>(parent).get_world_matrix();

    const RotTrans& rest_pose = rest_poses.at(current_weapon);

    glm::vec3 fwd = glm::quat_cast(parent_world * glm::mat4_cast(rest_pose.rotation)) * glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 start = glm::vec3(parent_world * glm::vec4(rest_pose.translation, 1.f)) + fwd * weapon_procanim_data.anti_overlap.start_offset;

    tmt::engine.polyline.use_color(glm::vec4(0.f, 1.f, 0.f, 1.f));
    tmt::engine.polyline.use_line_width(2.f);
    tmt::engine.polyline.draw_line(start, start + fwd * weapon_procanim_data.anti_overlap.check_length);
}

void game::WeaponManager::on_weapon_shoot(const WeaponFiredEvent& event) {
    fired = true;
    upward_response = true;
}

game::WeaponManager::RotTrans game::WeaponManager::animate_antioverlap() {
    auto& weapon_procanim_data = weapons.at(current_weapon).proc_anim_data;

    auto root_effector_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(weapon_procanim_data.root);
    if (!root_effector_transform) {
        tmt::Log::info("[WeaponManager] no root effector transform assigned, can't do procanim");
        return {};
    }
    auto parent = root_effector_transform->get_parent();
    auto& parent_world = tmt::engine.ecs.get_component<tmt::Transform>(parent).get_world_matrix();

    glm::quat world_rot = glm::quat_cast(parent_world * glm::mat4_cast(rest_poses[current_weapon].rotation));
    glm::vec3 fwd = world_rot * glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 up = world_rot * glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 right = world_rot * glm::vec3(1.f, 0.f, 1.f);
    glm::vec3 start = glm::vec3(parent_world * glm::vec4(rest_poses[current_weapon].translation, 1.f)) + fwd * weapon_procanim_data.anti_overlap.start_offset;

    tmt::Physics& phys_sys = tmt::engine.ecs.systems.get<tmt::Physics>();

    static const int aa_ray_count = 4;
    float flipper = 1.f;
    tmt::Hit aa_hits[aa_ray_count];
    for (int i = 0; i < aa_ray_count; i++) {
        float angle = atan(weapon_procanim_data.anti_overlap.scan_extent / weapon_procanim_data.anti_overlap.check_length);
        const glm::vec3& axis = i >= (aa_ray_count / 2) ? up : right;
        auto rot = glm::angleAxis(angle * flipper, axis);
        flipper *= -1.f;

        tmt::Ray aa_ray(start, rot * fwd);

        aa_hits[i] = phys_sys.raycast(aa_ray, 1);
    }

    float closest_dist = std::numeric_limits<float>::max();
    for (int i = 0; i < aa_ray_count; i++) {
        if (aa_hits[i].distance < closest_dist) closest_dist = aa_hits[i].distance;
    }

    if (closest_dist < weapon_procanim_data.anti_overlap.check_length) {
        float diff = weapon_procanim_data.anti_overlap.check_length - closest_dist;

        // target pos constrained
        glm::vec3 safe_world_pos = start - fwd * diff;

        glm::vec3 safe_local_pos = glm::inverse(parent_world) * glm::vec4(safe_world_pos, 1.f);
        glm::quat safe_rot = rest_poses[current_weapon].rotation;

        bool shoulder = glm::distance2(rest_poses[current_weapon].translation, safe_local_pos) >= weapon_procanim_data.anti_overlap.length_to_shoulder;
        if (shoulder) {
            safe_rot = glm::quat(glm::radians(weapon_procanim_data.anti_overlap.shoulder_eulers)) * safe_rot;
        }

        return RotTrans { .translation = safe_local_pos, .rotation = safe_rot };

    } else
        return rest_poses[current_weapon];
}

game::WeaponManager::RotTrans game::WeaponManager::animate_sway(RotTrans aa_rottrans, tmt::Transform* root_eff_transform, const WeaponProcAnimData& weapon_procanim_data) {
    if (tmt::engine.ecs.valid(shooting_entity)) {
        auto shooter_player = tmt::engine.ecs.try_get_component<game::Player>(shooting_entity);
        if (shooter_player) {
            static glm::vec3 last_player_velocity = {}, player_velocity = {};

            float d_pitch = (shooter_player->base_pitch - last_pitch);
            float d_yaw = (shooter_player->base_yaw - last_yaw);

            last_player_velocity = player_velocity;
            player_velocity = shooter_player->get_velocity();

            glm::vec3 vel_delta = (player_velocity - last_player_velocity);

            glm::vec3 local_delta = glm::inverse(shooter_player->get_transform().get_world_matrix()) * glm::vec4(vel_delta, 0.f);
            glm::vec3 local_vel = glm::inverse(shooter_player->get_transform().get_world_matrix()) * glm::vec4(player_velocity, 0.f);

            aa_rottrans.translation += -local_delta * weapon_procanim_data.sway.displacement_player_acceleration_multiplier;
            aa_rottrans.translation += -local_vel * weapon_procanim_data.sway.displacement_player_velocity_multiplier;

            if (d_pitch || d_yaw) {
                glm::vec3 displaced_vec = glm::vec3 { d_yaw, -d_pitch, 0.f } * 0.1f;

                float displaced_mag = std::min(glm::length(displaced_vec) * 0.001f * weapon_procanim_data.sway.look_sensitivity, 1.f);

                aa_rottrans.translation.x -= displaced_vec.x * weapon_procanim_data.sway.look_displacement;
                aa_rottrans.translation.y -= displaced_vec.y * weapon_procanim_data.sway.look_displacement;

                auto parent = root_eff_transform->get_parent();
                auto& parent_world = tmt::engine.ecs.get_component<tmt::Transform>(parent).get_world_matrix();

                const RotTrans& rest_pose = rest_poses.at(current_weapon);

                glm::vec3 fwd = glm::quat_cast(parent_world * glm::mat4_cast(rest_pose.rotation)) * glm::vec3(0.f, 0.f, 1.f);

                auto target_rot = glm::quatLookAt(glm::normalize(displaced_vec), glm::vec3(0.f, 1.f, 0.f));
                aa_rottrans.rotation = glm::slerp(aa_rottrans.rotation, target_rot, weapon_procanim_data.sway.look_max_rotation_factor * displaced_mag);
                aa_rottrans.rotation *= glm::angleAxis(glm::radians(d_yaw) * weapon_procanim_data.sway.look_roll_factor, glm::vec3(0.f, 0.f, 1.f));
            }

            last_pitch = shooter_player->base_pitch;
            last_yaw = shooter_player->base_yaw;

            return aa_rottrans;
        }
        return {};
    }
    return {};
}

void game::WeaponManager::update_procedural_motion(float dt) {
    auto& weapon_procanim_data = weapons.at(current_weapon).proc_anim_data;

    if (fired || updating_recoil_impulse) {
        static tmt::SecondOrderSolver::State<glm::quat> rot_state { .current_state = recoil_impulse.rotation };
        static tmt::SecondOrderSolver::State<glm::vec3> pos_state { .current_state = recoil_impulse.translation };
        static float upward_timer = 0.f;

        if (fired) {
            upward_timer = 0.f;
            shot_rand_roll = Random::rand_range(weapon_procanim_data.recoil.min_max_roll_deviation.x, weapon_procanim_data.recoil.min_max_roll_deviation.y);
            shot_rand_yaw = Random::rand_range(weapon_procanim_data.recoil.min_max_yaw_deviation.x, weapon_procanim_data.recoil.min_max_yaw_deviation.y);
        }

        fired = false;
        updating_recoil_impulse = true;

        if (upward_response) {
            auto& recoil_pos_motion = weapon_procanim_data.recoil.displacement_motion;
            auto& recoil_rot_motion = weapon_procanim_data.recoil.rotational_motion;

            if (upward_timer > weapon_procanim_data.recoil.upward_allowed_time) {
                upward_response = false;
                upward_timer = 0.f;
            }

            if (upward_timer > weapon_procanim_data.recoil.rot_time_offset)
                tmt::SecondOrderSolver::solve(
                    rot_state, glm::quat(glm::radians(weapon_procanim_data.recoil.angle_impulse + glm::vec3 { 0.f, shot_rand_yaw, shot_rand_roll })), recoil_rot_motion.frequency,
                    recoil_rot_motion.damping, recoil_rot_motion.initial_response, dt
                );
            tmt::SecondOrderSolver::solve(pos_state, weapon_procanim_data.recoil.pos_impulse, recoil_pos_motion.frequency, recoil_pos_motion.damping, recoil_pos_motion.initial_response, dt);
            upward_timer += dt;
        } else {
            auto& recoil_pos_motion = weapon_procanim_data.recoil.out_displacement_motion;
            auto& recoil_rot_motion = weapon_procanim_data.recoil.out_rotational_motion;

            tmt::SecondOrderSolver::solve(rot_state, glm::identity<glm::quat>(), recoil_rot_motion.frequency, recoil_rot_motion.damping, recoil_rot_motion.initial_response, dt);
            tmt::SecondOrderSolver::solve(pos_state, glm::vec3(0.f, 0.f, 0.f), recoil_pos_motion.frequency, recoil_pos_motion.damping, recoil_pos_motion.initial_response, dt);
        }

        recoil_impulse.rotation = rot_state.current_state;
        recoil_impulse.translation = pos_state.current_state;
    }

    auto* root_eff_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(weapon_procanim_data.root);
    if (root_eff_transform) {
        auto aa_rottrans = animate_antioverlap();
        auto sway_rottrans = animate_sway(aa_rottrans, root_eff_transform, weapon_procanim_data);

        target_pos = sway_rottrans.translation;
        auto target_rot = sway_rottrans.rotation;

        static tmt::SecondOrderSolver::State<glm::vec3> state { .current_state = rest_poses[current_weapon].translation };
        static tmt::SecondOrderSolver::State<glm::quat> rot_state { .current_state = rest_poses[current_weapon].rotation };

        auto& rot_motion = weapon_procanim_data.sway.rotational_motion;
        auto& pos_motion = weapon_procanim_data.sway.displacement_motion;

        tmt::SecondOrderSolver::solve(state, target_pos, pos_motion.frequency, pos_motion.damping, pos_motion.initial_response, dt);
        tmt::SecondOrderSolver::solve(rot_state, target_rot, rot_motion.frequency, rot_motion.damping, rot_motion.initial_response, dt);

        root_eff_transform->set_local_position(state.current_state + recoil_impulse.translation);
        root_eff_transform->set_local_rotation(rot_state.current_state * recoil_impulse.rotation);
    }
}

void game::WeaponManager::on_game_paused(const game::GamePausedEvent& event) {
    // Unsubscribe from the current weapon
    unsubscribe_weapon(get_active_weapon());
    // You can save the state of the active weapon if needed
    last_used_weapon = get_active_weapon();
}

void game::WeaponManager::on_game_unpaused(const game::GameUnpausedEvent& event) {
    // Resubscribe to the last used weapon
    set_new_weapon(last_used_weapon);
}
