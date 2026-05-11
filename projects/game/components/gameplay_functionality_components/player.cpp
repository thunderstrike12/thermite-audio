#include "player.hpp"

#include "engine/core/polyline.hpp"
#include "projects/game/data_headers/events.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/core/input/input.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/window.hpp"
#include "engine/core/input/input_map.hpp"
#include "projects/game/data_headers/game_input.hpp"
#include "engine/core/components/ui_component.hpp"
#include "engine/shared/ray.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/components/development_tools/debug_line_helper.hpp"
#include "projects/game/data_headers/save_entries.hpp"
#include "projects/game/data_headers/wallet.hpp"

// TODO before we have a serializer for input, you can add all the needed keybindings here.
//  TODO we still have to add the gamepad inputs here

static glm::vec2 hash_noise(float t) {
    glm::vec2 n;
    n.x = glm::fract(std::sin(t * 12.9898f) * 43758.5453f);
    n.y = glm::fract(std::sin((t + 1.0f) * 78.233f) * 43758.5453f);

    return n * 2.0f - 1.0f;  // range [-1, 1]
}

static float move_towards(float current, float target, float max_delta) {
    float delta = target - current;

    if (glm::abs(delta) <= max_delta) return target;

    return current + glm::sign(delta) * max_delta;
}

namespace game {

void setup_inputs(tmt::InputMap& input_map) {
    // movement
    input_map.add_action(action::MOVE_FORWARD);
    input_map.add_key_to_action(action::MOVE_FORWARD, tmt::Key::W);

    input_map.add_action(action::MOVE_BACKWARD);
    input_map.add_key_to_action(action::MOVE_BACKWARD, tmt::Key::S);

    input_map.add_action(action::MOVE_LEFT);
    input_map.add_key_to_action(action::MOVE_LEFT, tmt::Key::A);

    input_map.add_action(action::MOVE_RIGHT);
    input_map.add_key_to_action(action::MOVE_RIGHT, tmt::Key::D);

    input_map.add_action(action::MOVE_UP);
    input_map.add_key_to_action(action::MOVE_UP, tmt::Key::SPACE);

    input_map.add_action(action::MOVE_DOWN);
    input_map.add_key_to_action(action::MOVE_DOWN, tmt::Key::LEFT_SHIFT);

    input_map.add_action(action::BREAK);
    input_map.add_key_to_action(action::BREAK, tmt::Key::LEFT_CTRL);

    input_map.add_action(action::BOOST);
    input_map.add_key_to_action(action::BOOST, tmt::Key::R);

    // combat
    input_map.add_action(action::SHOOT);
    input_map.add_action_mouse(action::SHOOT, tmt::MouseButton::LEFT);

    input_map.add_action(action::SWITCH_RIFLE);
    input_map.add_key_to_action(action::SWITCH_RIFLE, tmt::Key::NUM_1);
    input_map.add_action(action::SWITCH_GRAVITY);
    input_map.add_key_to_action(action::SWITCH_GRAVITY, tmt::Key::NUM_2);
    input_map.add_action(action::SWITCH_MINING);
    input_map.add_key_to_action(action::SWITCH_MINING, tmt::Key::NUM_3);
    input_map.add_action(action::SECONDARY_TOOL_USE);
    input_map.add_action_mouse(action::SECONDARY_TOOL_USE, tmt::MouseButton::RIGHT);

    // menus
    input_map.add_action(action::OPEN_INVENTORY);
    input_map.add_key_to_action(action::OPEN_INVENTORY, tmt::Key::I);
    input_map.add_action(action::OPEN_PAUSE_MENU);
    input_map.add_key_to_action(action::OPEN_PAUSE_MENU, tmt::Key::P);
    input_map.add_action(action::OPEN_UPGRADE_MENU);
    input_map.add_key_to_action(action::OPEN_UPGRADE_MENU, tmt::Key::U);

    // barge
    input_map.add_action(action::ATTACH_KEY);
    input_map.add_key_to_action(action::ATTACH_KEY, tmt::Key::E);
    input_map.add_action(action::TRIGGER_BARGE_MOVEMENT);
    input_map.add_key_to_action(action::TRIGGER_BARGE_MOVEMENT, tmt::Key::SPACE);
    input_map.add_action(action::TRIGGER_RUN_END);
    input_map.add_key_to_action(action::TRIGGER_RUN_END, tmt::Key::LEFT_CTRL);
}

Player& Player::get() {
    auto view = tmt::engine.ecs.view<Player>(entt::exclude_t {});
    if (view.empty()) {
        tmt::Log::error("[CRITICAL] No Player component found in the scene");
        throw std::runtime_error("No player entity found");
    }
    return view.front();
}

void Player::load_upgrades() {
    // TODO only load if there is data there
    health = tmt::engine.player_data.get<PlayerStat>(PLAYER_HEALTH_DATA, health);
    energy = tmt::engine.player_data.get<PlayerStat>(PLAYER_ENERGY_DATA, energy);

    auto movement_data = tmt::engine.player_data.try_get<PlayerMovement>(PLAYER_MOVEMENT_DATA);
    if (movement_data.has_value()) {
        acceleration = movement_data->acceleration;
        max_speed = movement_data->max_speed;
        boost_max_speed_multiplier = movement_data->boost_max_speed_multiplier;
    }

    auto recharge_data = tmt::engine.player_data.try_get<PlayerRecharge>(PLAYER_RECHARGE_DATA);
    if (recharge_data.has_value()) {
        out_of_energy_time_till_death = recharge_data->out_of_energy_time_till_death;
        recharge_distance = recharge_data->recharge_distance;
    }
}
void Player::start() {
    auto& resources { tmt::engine.player_data.get<Currencies>(PERSISTENT_RESOURCES) };
    // initialize wallet
    auto* wallet = tmt::engine.ecs.try_get_component<Wallet>(entity);
    if (wallet == nullptr) {
        tmt::Log::error("No Wallet component found");

        return;
    }
    wallet->currencies = resources;
    if (tmt::engine.ecs.is_enabled(entity)) {
        tmt::engine.input.set_mouse_relative_to_window(true);
        glm::vec2 screen_size = { tmt::engine.window.width, tmt::engine.window.height };
        tmt::engine.input.warp_mouse({ screen_size.x / 2.0f, screen_size.y / 2.0f }, true);
        tmt::engine.input.lock_mouse(true);
        setup_inputs(tmt::engine.input_map);
        tmt::engine.ecs.get_dispatcher().sink<AttachEvent>().connect<&Player::on_attach>(this);
    }
    load_upgrades();
    set_crosshair(rifle_crosshair);
}
void Player::end() {
    tmt::engine.ecs.get_dispatcher().sink<AttachEvent>().disconnect<&Player::on_attach>(this);
    // this is a forced closing of the game, behaves like the player just dies
}

void Player::look_camera() {
    auto& input = tmt::engine.input;

    auto camera = tmt::engine.ecs.try_get_component<tmt::Camera>(entity);
    if (!camera) {
        state = PlayerState::PAUSED;

        if (input.is_mouse_locked()) {
            input.lock_mouse(false);
            input.set_mouse_relative_to_window(false);
        }
        return;
    }

    if (!input.is_mouse_locked()) return;

    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    const float dx = input.get_mouse_delta_x();
    const float dy = input.get_mouse_delta_y();

    base_yaw -= dx * camera_sensitivity;
    base_pitch -= dy * camera_sensitivity;

    base_pitch = glm::clamp(base_pitch, -89.0f, 89.0f);

    float yaw = base_yaw;
    float pitch = base_pitch;

    if (camera_shake_settings.enabled) {
        yaw += recoil_offset.x;
        pitch += recoil_offset.y;
    }

    glm::vec3 front;

    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    front = glm::normalize(front);

    glm::vec3 shake_offset(0.0f);

    if (camera_shake_settings.enabled && current_shake > 0.0f) {
        glm::vec2 n = hash_noise(tmt::engine.frame_data().elapsed_time);

        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
        glm::vec3 up = glm::normalize(glm::cross(right, front));

        glm::vec3 target_shake = right * n.x * current_shake + up * n.y * current_shake;

        float smooth = 12.0f * tmt::engine.frame_data().delta_time;

        static glm::vec3 shake_current(0.0f);
        shake_current = glm::mix(shake_current, target_shake, smooth);

        shake_offset = shake_current;
    }

    transform.look_at(transform.get_world_position() + front + shake_offset, glm::vec3(0.0f, 1.0f, 0.0f));
}

tmt::Hit Player::check_collision() const {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto& physics = tmt::engine.ecs.systems.get<tmt::Physics>();

    if (glm::length(velocity) < 0.001f) return tmt::Hit {};

    glm::vec3 pos = transform.get_world_position();
    glm::vec3 vel_dir = glm::normalize(velocity);

    tmt::Hit closest = physics.raycast(tmt::Ray(pos, vel_dir), ray_check.collision_layer);

    glm::vec3 up = glm::vec3(0, 1, 0);
    if (glm::abs(glm::dot(vel_dir, up)) > 0.99f) up = glm::vec3(1, 0, 0);

    glm::vec3 right = glm::normalize(glm::cross(vel_dir, up));
    glm::vec3 local_up = glm::cross(right, vel_dir);

    glm::vec3 offsets[] = { right * ray_check.player_radius, -right * ray_check.player_radius, local_up * ray_check.player_radius, -local_up * ray_check.player_radius };

    for (auto& offset : offsets) {
        auto hit = physics.raycast(tmt::Ray(pos + offset, vel_dir), ray_check.collision_layer);
        if (!hit.miss() && (closest.miss() || hit.distance < closest.distance)) {
            closest = hit;
        }
    }
    return closest;
}
void Player::apply_impulse(const glm::vec3& direction, float force) {
    if (glm::length(direction) < 0.001f) return;
    velocity += glm::normalize(direction) * force;
}
void Player::move_player() {
    auto& input = tmt::engine.input;

    input_dir = glm::vec3 { 0.0f, 0.0f, 0.0f };
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    /* If not locked don't move player */
    if (input.is_action_pressed(action::MOVE_FORWARD)) input_dir += transform.get_forward();
    if (input.is_action_pressed(action::MOVE_BACKWARD)) input_dir -= transform.get_forward();
    if (input.is_action_pressed(action::MOVE_RIGHT)) input_dir += transform.get_right();
    if (input.is_action_pressed(action::MOVE_LEFT)) input_dir -= transform.get_right();
    if (input.is_action_pressed(action::MOVE_UP)) input_dir += glm::vec3(0.0f, 1.0f, 0.0f);
    if (input.is_action_pressed(action::MOVE_DOWN)) input_dir -= glm::vec3(0.0f, 1.0f, 0.0f);

    // Added button for breaking, port from prototype
    bool breaking = input.is_action_pressed(action::BREAK);

    auto delta_time = tmt::engine.frame_data().delta_time;
    // Apply acceleration or drag
    if (glm::length(input_dir) > 0.0f) {
        // Accelerate in input direction
        input_dir = glm::normalize(input_dir);
        velocity += input_dir * acceleration_calculated * delta_time;
    } else {
        // Apply drag when no input
        float current_speed = glm::length(velocity);
        if (current_speed > 0.0f) {
            float drag_force = drag * delta_time;
            float new_speed = glm::max(0.0f, current_speed - drag_force);
            velocity = glm::normalize(velocity) * new_speed;
        }
    }

    // Apply breaking force if breaking
    if (breaking) {
        float current_speed = glm::length(velocity);
        if (current_speed > 0.0f) {
            float break_force = deceleration * delta_time;
            float new_speed = glm::max(0.0f, current_speed - break_force);
            velocity = glm::normalize(velocity) * new_speed;
        }
    }

    // Clamp velocity to max speed
    float current_speed = glm::length(velocity);
    if (current_speed > max_speed_calculated) {
        velocity = glm::normalize(velocity) * max_speed_calculated;
    }

    auto hit = check_collision();
    if (!hit.miss() && hit.distance < ray_check.player_radius) {
        float into_wall = glm::dot(velocity, -hit.normal);
        if (into_wall > 0.0f) {
            velocity += hit.normal * into_wall;

            velocity *= ray_check.collision_speed_damping;
        }
    }

    // Move
    transform.translate(velocity * delta_time);

    resolve_penetration();

    prevent_camera_clip();
}
// try to get out of inside a voxel
void Player::resolve_penetration() {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto& physics = tmt::engine.ecs.systems.get<tmt::Physics>();
    glm::vec3 pos = transform.get_world_position();

    constexpr glm::vec3 dirs[] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };

    glm::vec3 push_out { 0.f };

    for (const auto& dir : dirs) {
        auto hit = physics.raycast(tmt::Ray(pos, dir), ray_check.collision_layer);
        if (!hit.miss() && hit.distance < ray_check.player_radius) {
            float penetration = ray_check.player_radius - hit.distance;
            push_out -= dir * penetration;
        }
    }

    if (glm::length(push_out) < 0.0001f) return;

    pos += push_out;
    transform.set_world_position(pos);

    // Kill velocity component that points into the collision
    glm::vec3 push_dir = glm::normalize(push_out);
    float into = glm::dot(velocity, -push_dir);
    if (into > 0.f) {
        velocity += push_dir * into;
    }

    velocity *= ray_check.collision_speed_damping;
}

void Player::prevent_camera_clip() const {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto& physics = tmt::engine.ecs.systems.get<tmt::Physics>();

    glm::vec3 pos = transform.get_world_position();
    glm::vec3 forward = transform.get_forward();

    const auto hit = physics.raycast(tmt::Ray(pos, forward), ray_check.collision_layer);

    if (!hit.miss() && hit.distance < ray_check.camera_near_distance) {
        float pushback = ray_check.camera_near_distance - hit.distance;
        transform.set_world_position(pos - forward * pushback);
    }
}

void Player::update(const tmt::FrameData& time) {
    auto& input = tmt::engine.input;

    update_shake(time.delta_time);

    // --- Recoil recovery ---
    float dt = glm::min(time.delta_time, 1.0f / 30.0f);

    // how fast it returns
    float return_speed = camera_shake_settings.recoil_return_speed;

    // move back toward zero at a constant speed
    recoil_offset.x = move_towards(recoil_offset.x, 0.0f, return_speed * dt);
    recoil_offset.y = move_towards(recoil_offset.y, 0.0f, return_speed * dt);

    // Swapping crosshair
    if (input.is_action_just_pressed(action::SWITCH_RIFLE)) {
        set_crosshair(rifle_crosshair);
    }
    if (input.is_action_just_pressed(action::SWITCH_GRAVITY)) {
        set_crosshair(gravity_crosshair);
    }
    if (input.is_action_just_pressed(action::SWITCH_MINING)) {
        set_crosshair(mine_crosshair);
    }

    switch (state) {
        case game::PlayerState::FREEMOVING:
            // Handle player movement boost (initial cost)

            if (input.is_action_just_pressed(action::BOOST)) {
                energy.value = glm::min(energy.max_value, energy.value - boost_initial_cost);
            }
            if (input.is_action_pressed(action::BOOST)) {
                apply_boost();

                add_camera_shake(camera_shake_settings.boost_intensity);
            } else {
                reset_boost(time.delta_time);
            }

            attempt_attach(input);
            look_camera();
            move_player();

            // Handle energy drain for boost
            if (boost_was_applied) {
                float current_speed = glm::length(velocity);
                energy.value = glm::min(energy.max_value, energy.value - time.delta_time * boost_cost_per_second_per_additional_speed_above_max * glm::max(0.0f, current_speed - max_speed));
            }

            // Handle recharging and draining
            if (tmt::engine.ecs.valid(barge)) {
                if (glm::distance(tmt::engine.ecs.get_component<tmt::Transform>(entity).get_world_position(), tmt::engine.ecs.get_component<tmt::Transform>(barge).get_world_position()) >=
                    recharge_distance) {
                    drain_energy(time.delta_time);
                } else {
                    refill(time.delta_time);
                }
            }
            break;
        case game::PlayerState::ATTACHED:
            attempt_attach(input);
            look_camera();
            refill(time.delta_time);
            break;
            // TODO this state might disappear
        case game::PlayerState::PAUSED:
            break;
        default:
            break;
    }
    // TODO state will be much easier to handle
    // triggers the event for shooting

    // TODO events, could also use entt on modifcation component for the UI components

    // UI bar updates
    // Fire event max health changed
    if (previous_max_health != health.max_value) {
        tmt::engine.ecs.get_dispatcher().trigger(PlayerMaxHealthChanged { entity, health.max_value, previous_max_health });
    }
    previous_max_health = health.max_value;
    // Fire event health changed
    if (previous_health != health.value) {
        tmt::engine.ecs.get_dispatcher().trigger(PlayerHealthChanged { entity, health.value, previous_health });
    }
    previous_health = health.value;
    // Fire event max energy changed
    if (previous_max_energy != energy.max_value) {
        tmt::engine.ecs.get_dispatcher().trigger(PlayerMaxEnergyChanged { entity, energy.max_value, previous_max_energy });
    }
    previous_max_energy = energy.max_value;
    // Fire event energy changed
    if (previous_energy != energy.value) {
        tmt::engine.ecs.get_dispatcher().trigger(PlayerEnergyChanged { entity, energy.value, previous_energy });
    }
    previous_energy = energy.value;

    // --- Energy low pop up ---
    {
        float energy_percent = energy.value / energy.max_value;

        // First warning
        // Trigger when crossing from above to below
        if (was_above_threshold && energy_percent <= low_energy_threshold) {
            set_hud_enabled(low_energy_hud, true);

            low_energy_timer = low_energy_duration;
            low_energy_active = true;
        }

        // Handle timer
        if (low_energy_active) {
            low_energy_timer -= time.delta_time;

            if (low_energy_timer <= 0.0f) {
                set_hud_enabled(low_energy_hud, false);
                low_energy_active = false;
            }
        }

        // Update state for next frame
        was_above_threshold = (energy_percent > low_energy_threshold);

        // Second wanrning
        if (use_second_warning) {
            if (was_above_threshold_2 && energy_percent <= low_energy_threshold_2) {
                set_hud_enabled(low_energy_hud, true);
                low_energy_timer_2 = low_energy_duration_2;
                low_energy_active_2 = true;
            }

            if (low_energy_active_2) {
                low_energy_timer_2 -= time.delta_time;
                if (low_energy_timer_2 <= 0.0f) {
                    set_hud_enabled(low_energy_hud, false);
                    low_energy_active_2 = false;
                }
            }

            was_above_threshold_2 = (energy_percent > low_energy_threshold_2);
        }
    }

    // Death handling logic

    // Energy death (timer)
    if (energy.value <= 0.0f) {
        out_of_energy_timer += time.delta_time;
    } else {
        // reset out of battery timer
        out_of_energy_timer = 0.0f;
    }

    if (out_of_energy_timer >= out_of_energy_time_till_death && !player_ended_run) {
        tmt::engine.ecs.get_dispatcher().trigger(EndRun { true });
        player_ended_run = true;
        state = PlayerState::PAUSED;
    }

    // Health death (instant)
    if (health.value <= 0.0f && !player_ended_run) {
        // player ded -> call end run event with player ded
        tmt::engine.ecs.get_dispatcher().trigger(EndRun { true });
        player_ended_run = true;
        state = PlayerState::PAUSED;
    }

    // UI boost availability
    if (tmt::engine.ecs.valid(boost_availability)) {
        if (boost_was_applied) {
            tmt::engine.ecs.disable(boost_availability);
        } else {
            tmt::engine.ecs.enable(boost_availability);
        }
    }
}

void Player::draw_debug_lines() const {
    const auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    const auto& physics = tmt::engine.ecs.systems.get<tmt::Physics>();
    const auto pos = transform.get_world_position();

    // player radius
    {
        game::DebugLineConfig cf;
        cf.color = tmt::RGBA { glm::vec4 { 0.2f, 0.8f, 0.2f, 0.5f } };  // soft green
        cf.set_values();
        tmt::engine.polyline.draw_sphere(pos, ray_check.player_radius);
    }

    // Green  = clear (no geometry within radius)
    // Yellow = surface nearby but outside radius
    // Red    = penetrating (surface closer than player_radius)
    {
        constexpr glm::vec3 dirs[] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };

        for (const auto& dir : dirs) {
            auto hit = physics.raycast(tmt::Ray(pos, dir), ray_check.collision_layer);

            game::DebugLineConfig cf;
            if (!hit.miss() && hit.distance < ray_check.player_radius) {
                // Penetrating — red
                cf.color = tmt::RGBA { glm::vec4 { 1.0f, 0.0f, 0.0f, 1.0f } };
            } else if (!hit.miss() && hit.distance < ray_check.player_radius * 2.0f) {
                // Close but not penetrating — yellow warning
                cf.color = tmt::RGBA { glm::vec4 { 1.0f, 1.0f, 0.0f, 0.8f } };
            } else {
                // Clear — green
                cf.color = tmt::RGBA { glm::vec4 { 0.0f, 1.0f, 0.0f, 0.5f } };
            }
            cf.set_values();

            // Draw ray out to player_radius length
            tmt::engine.polyline.draw_arrow(pos, dir, ray_check.player_radius);

            // If there's a hit, draw a small cross at the hit point
            if (!hit.miss() && hit.distance < ray_check.player_radius * 2.0f) {
                glm::vec3 hit_point = pos + dir * hit.distance;
                glm::vec3 up = glm::abs(dir.y) > 0.9f ? glm::vec3 { 1, 0, 0 } : glm::vec3 { 0, 1, 0 };
                glm::vec3 perp_a = glm::normalize(glm::cross(dir, up));
                glm::vec3 perp_b = glm::cross(dir, perp_a);
                float cross_size = 0.05f;
                tmt::engine.polyline.draw_line(hit_point - perp_a * cross_size, hit_point + perp_a * cross_size);
                tmt::engine.polyline.draw_line(hit_point - perp_b * cross_size, hit_point + perp_b * cross_size);
            }
        }
    }

    // camera check
    {
        glm::vec3 forward = transform.get_forward();
        auto hit = physics.raycast(tmt::Ray(pos, forward), ray_check.collision_layer);

        game::DebugLineConfig cf;
        if (!hit.miss() && hit.distance < ray_check.camera_near_distance) {
            cf.color = tmt::RGBA { glm::vec4 { 1.0f, 0.0f, 1.0f, 1.0f } };  // magenta = clipping
        } else {
            cf.color = tmt::RGBA { glm::vec4 { 0.0f, 1.0f, 1.0f, 0.6f } };  // cyan = clear
        }
        cf.set_values();
        tmt::engine.polyline.draw_arrow(pos, forward, ray_check.camera_near_distance);
    }

    // velocity
    if (glm::length(velocity) > 0.001f) {
        const auto arrow_origin = pos + transform.get_forward() - glm::vec3 { 0.0f, 0.1f, 0.0f };

        game::DebugLineConfig cf;
        auto hit = check_collision();
        if (!hit.miss() && hit.distance < ray_check.player_radius) {
            cf.color = tmt::RGBA { glm::vec4 { 1.0f, 0.0f, 0.0f, 1.0f } };  // red = blocked
        } else if (!hit.miss() && hit.distance < ray_check.player_radius * 1.5f) {
            cf.color = tmt::RGBA { glm::vec4 { 1.0f, 0.5f, 0.0f, 1.0f } };  // orange = close
        } else {
            cf.color = tmt::RGBA { glm::vec4 { 1.0f, 1.0f, 1.0f, 0.6f } };  // white = clear
        }
        cf.set_values();

        tmt::engine.polyline.draw_arrow(arrow_origin, glm::normalize(velocity), ray_check.player_radius * 2.0f);
    }
}

void Player::attempt_attach(tmt::Input& input) {
    if (input.is_action_just_pressed(action::ATTACH_KEY)) {
        tmt::engine.ecs.get_dispatcher().trigger<AttachAttemptEvent>({ .entity = entity });
    }
}

void Player::refill(float delta) {
    // TODO probably an event here for audio, graphics etc.

    health.value = glm::min(health.max_value, health.value + delta * health.increase_multiplier);
    energy.value = glm::min(energy.max_value, energy.value + delta * energy.increase_multiplier);
}

void Player::drain_energy(float delta) {
    energy.value = glm::min(energy.max_value, energy.value - delta * energy_drain_per_second);
}

void Player::on_attach(const AttachEvent& event) {
    if (event.entity != entity) {
        return;
    }
    if (event.is_attached) {
        state = PlayerState::ATTACHED;
        // this is used so if the player was already holding the button, it does not instantly move
        reset_action_time(action::TRIGGER_BARGE_MOVEMENT);
        reset_action_time(action::TRIGGER_RUN_END);
        velocity = glm::vec3 { 0.0f };

        set_hud_enabled(player_hud, false);
        set_hud_enabled(barge_hud, true);

    } else {
        state = PlayerState::FREEMOVING;

        set_hud_enabled(player_hud, true);
        set_hud_enabled(barge_hud, false);
    }
}

void Player::apply_boost() {
    max_speed_calculated = max_speed * boost_max_speed_multiplier;
    acceleration_calculated = acceleration * boost_acceleration_multiplier;
    boost_was_applied = true;
}

void Player::reset_boost(float delta_time) {
    /*
    max_speed_calculated = max_speed;
    acceleration_calculated = acceleration;*/
    float lerp_factor = std::min(1.0f, boost_deceleration_factor * delta_time);

    boost_was_applied = false;
    max_speed_calculated = glm::mix(max_speed_calculated, max_speed, lerp_factor);
    acceleration_calculated = glm::mix(acceleration_calculated, acceleration, lerp_factor);

    // Snap to exact values when close enough
    if (glm::abs(max_speed_calculated - max_speed) < 0.001f) {
        max_speed_calculated = max_speed;
    }
    if (glm::abs(acceleration_calculated - acceleration) < 0.001f) {
        acceleration_calculated = acceleration;
    }
}

void Player::reset_action_time(std::string_view action_name) {
    auto& input_map = tmt::engine.input_map;
    auto* action = input_map.get_action(std::string { action_name });
    action->time_since_being_pressed = 0.0f;
}

void Player::set_hud_enabled(tmt::Entity hud_root, bool enabled) {
    auto& ecs = tmt::engine.ecs;

    if (!ecs.valid(hud_root)) return;

    // Toggle this entity
    if (enabled) {
        ecs.remove_component<tmt::Disable>(hud_root);
    } else {
        ecs.add_or_get_component<tmt::Disable>(hud_root);
    }

    // Get transform
    if (!ecs.has_component<tmt::Transform>(hud_root)) return;

    auto& transform = ecs.get_component<tmt::Transform>(hud_root);

    // Recurse children
    for (auto child : transform.get_all_children()) {
        set_hud_enabled(child, enabled);
    }
}

void Player::add_camera_shake(float intensity) {
    if (!camera_shake_settings.enabled) return;

    current_shake = glm::min(camera_shake_settings.max_intensity, current_shake + intensity);
}

void Player::update_shake(float dt) {
    current_shake -= dt * camera_shake_settings.decay_speed;
    current_shake = glm::max(0.0f, current_shake);
}

void Player::add_recoil() {
    recoil_offset.y += camera_shake_settings.recoil_strength;

    float rand_x = (rand() / (float)RAND_MAX - 0.5f) * 2.0f;
    recoil_offset.x += rand_x * camera_shake_settings.recoil_horizontal;
}

void Player::set_crosshair(tmt::Entity active) {
    auto& ecs = tmt::engine.ecs;

    auto toggle = [&](auto&& self, tmt::Entity e, bool enabled) -> void {
        if (!ecs.valid(e)) return;

        // Toggle this entity
        if (enabled) {
            ecs.remove_component<tmt::Disable>(e);
        } else {
            ecs.add_or_get_component<tmt::Disable>(e);
        }

        // Toggle children
        if (!ecs.has_component<tmt::Transform>(e)) return;

        auto& transform = ecs.get_component<tmt::Transform>(e);
        for (auto child : transform.get_all_children()) {
            self(self, child, enabled);
        }
    };

    // Disable all crosshairs
    toggle(toggle, rifle_crosshair, false);
    toggle(toggle, gravity_crosshair, false);
    toggle(toggle, mine_crosshair, false);

    // Enable selected crosshair
    toggle(toggle, active, true);
}

}  // namespace game
