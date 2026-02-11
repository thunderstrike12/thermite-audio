#include "player.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/core/input/input.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/window.hpp"

void Player::start() {
    tmt::engine.input.lock_mouse(true);
    tmt::engine.input.set_mouse_relative_to_window(true);
    glm::vec2 screen_size = { tmt::engine.window.width, tmt::engine.window.height };
    tmt::engine.input.warp_mouse({ screen_size.x / 2.0f, screen_size.y / 2.0f }, true);
}

void Player::update(const tmt::FrameData& time) {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto& camera = tmt::engine.ecs.get_component<tmt::Camera>(entity);
    const float dx = tmt::engine.input.get_mouse_delta_x();
    const float dy = tmt::engine.input.get_mouse_delta_y();

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

    // Calculate desired movement direction
    glm::vec3 input_dir = { 0.0f, 0.0f, 0.0f };
    if (tmt::engine.input.is_keyboard_button_pressed(tmt::Key::W)) input_dir += transform.get_forward();
    if (tmt::engine.input.is_keyboard_button_pressed(tmt::Key::S)) input_dir -= transform.get_forward();
    if (tmt::engine.input.is_keyboard_button_pressed(tmt::Key::D)) input_dir += transform.get_right();
    if (tmt::engine.input.is_keyboard_button_pressed(tmt::Key::A)) input_dir -= transform.get_right();
    if (tmt::engine.input.is_keyboard_button_pressed(tmt::Key::SPACE)) input_dir += glm::vec3(0.0f, 1.0f, 0.0f);
    if (tmt::engine.input.is_keyboard_button_pressed(tmt::Key::LEFT_SHIFT)) input_dir -= glm::vec3(0.0f, 1.0f, 0.0f);

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