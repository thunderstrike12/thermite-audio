#include "player.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/core/input/input.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/window.hpp"
#include "engine/core/input/input_map.hpp"
#include "game_input.hpp"
// TODO before we have a serializer for input, you can add all the needed keybindings here.
//  TODO we still have to add the gamepad inputs here

using namespace game;
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

    // combat
    input_map.add_action(action::SHOOT);
    input_map.add_action_mouse(action::SHOOT, tmt::MouseButton::LEFT);

    input_map.add_action(action::SWITCH_WEAPON_1);
    input_map.add_key_to_action(action::SWITCH_WEAPON_1, tmt::Key::NUM_1);
    input_map.add_action(action::SWITCH_WEAPON_2);
    input_map.add_key_to_action(action::SWITCH_WEAPON_2, tmt::Key::NUM_2);
    input_map.add_action(action::SWITCH_WEAPON_3);
    input_map.add_key_to_action(action::SWITCH_WEAPON_3, tmt::Key::NUM_3);
}

void Player::start() {
    tmt::engine.input.set_mouse_relative_to_window(true);
    glm::vec2 screen_size = {tmt::engine.window.width, tmt::engine.window.height};
    tmt::engine.input.warp_mouse({screen_size.x / 2.0f, screen_size.y / 2.0f}, true);
    tmt::engine.input.lock_mouse(true);
    setup_inputs(tmt::engine.input_map);
}

void Player::update(const tmt::FrameData& time) {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto& camera = tmt::engine.ecs.get_component<tmt::Camera>(entity);
    auto& input = tmt::engine.input;
    const float dx = input.get_mouse_delta_x();
    const float dy = input.get_mouse_delta_y();

    // Mouse Look (unchanged)
    camera.yaw -= dx * camera_sensitivity;
    camera.pitch -= dy * camera_sensitivity;
    camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

    glm::vec3 front = {};
    front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    front.y = sin(glm::radians(camera.pitch));
    front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    front = glm::normalize(front);
    transform.look_at(transform.get_world_position() + front, glm::vec3(0.0f, 1.0f, 0.0f));

    // Calculate desired movement direction (unchanged from demo)
    glm::vec3 input_dir = {0.0f, 0.0f, 0.0f};
    if (input.is_action_pressed(action::MOVE_FORWARD)) input_dir += transform.get_forward();
    if (input.is_action_pressed(action::MOVE_BACKWARD)) input_dir -= transform.get_forward();
    if (input.is_action_pressed(action::MOVE_RIGHT)) input_dir += transform.get_right();
    if (input.is_action_pressed(action::MOVE_LEFT)) input_dir -= transform.get_right();
    if (input.is_action_pressed(action::MOVE_UP)) input_dir += glm::vec3(0.0f, 1.0f, 0.0f);
    if (input.is_action_pressed(action::MOVE_DOWN)) input_dir -= glm::vec3(0.0f, 1.0f, 0.0f);

    // Added button for breaking, port from prototype
    bool breaking = input.is_action_pressed(action::BREAK);

    // Apply acceleration or drag
    if (glm::length(input_dir) > 0.0f) {
        // Accelerate in input direction
        input_dir = glm::normalize(input_dir);
        velocity += input_dir * acceleration * time.delta_time;
    } else {
        // Apply drag when no input
        float current_speed = glm::length(velocity);
        if (current_speed > 0.0f) {
            float drag_force = drag * time.delta_time;
            float new_speed = glm::max(0.0f, current_speed - drag_force);
            velocity = glm::normalize(velocity) * new_speed;
        }
    }

    // Apply breaking force if breaking
    if (breaking) {
        float current_speed = glm::length(velocity);
        if (current_speed > 0.0f) {
            float break_force = acceleration * 2.0f * time.delta_time;
            float new_speed = glm::max(0.0f, current_speed - break_force);
            velocity = glm::normalize(velocity) * new_speed;
        }
    }

    // Clamp velocity to max speed
    float current_speed = glm::length(velocity);
    if (current_speed > max_speed) {
        velocity = glm::normalize(velocity) * max_speed;
    }

    // Apply velocity to position
    transform.translate(velocity * time.delta_time);
}

void Player::end() {
    // Cleanup code for the player component
}
