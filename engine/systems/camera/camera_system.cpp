#include "camera_system.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"
#include "core/input//input.hpp"
#include "core/logger.hpp"
#include "core/components/camera.hpp"
#include "core/components/transform.hpp"
#include "engine/core/input/keys.hpp"

namespace tmt {
void CameraSystem::on_start() {
    // Setup Input Actions
    engine.input_map.add_action_keys(SPRINT, Key::LEFT_SHIFT);
    engine.input_map.add_action_keys(FORWARD, Key::W);
    engine.input_map.add_action_keys(BACKWARD, Key::S);
    engine.input_map.add_action_keys(RIGHT, Key::D);
    engine.input_map.add_action_keys(LEFT, Key::A);
    engine.input_map.add_action_keys(UP, Key::E);
    engine.input_map.add_action_keys(DOWN, Key::Q);
    engine.input_map.add_action_keys(UP, Key::SPACE);
    engine.input_map.add_action_keys(DOWN, Key::LEFT_CTRL);
}

void CameraSystem::on_update(const FrameData& time) {
    // Gather Variables to be used
    auto& input = engine.input;
    const Entity camera_entity = Camera::get_active_camera();
    if (camera_entity == entt::null) {
        Log::error(Log::Scope::ENGINE, "Camera Entity is NULL. Are there any active cameras in the scene?");
        return;
    }

    const bool enable_mouse_look = input.is_action_pressed(action::RIGHT_CLICK);
    const bool is_2d_axis_movement = input.is_action_pressed(action::LEFT_CLICK);

    if (!enable_mouse_look) {
        /* Show mouse cursor */
        input.set_mouse_relative_to_window(false);
        input.lock_mouse(false);
        return;
    }
    /* Hide mouse cursor */
    input.set_mouse_relative_to_window(true);
    input.lock_mouse(true);

    Camera& camera = engine.ecs.get_component<Camera>(camera_entity);
    Transform& transform = engine.ecs.get_component<Transform>(camera_entity);

    const float dx = input.get_mouse_delta_x();
    const float dy = input.get_mouse_delta_y();

    if (is_2d_axis_movement == false) {
        // Mouse Look
        camera.yaw -= dx * cam_sensitivity;
        camera.pitch -= dy * cam_sensitivity;
        camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

        glm::vec3 front = {};
        front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        front.y = sin(glm::radians(camera.pitch));
        front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        front = glm::normalize(front);
        transform.look_at(transform.get_world_position() + front, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    const float mouse_wheel_y_delta = input.get_mouse_wheel_y();
    base_speed *= std::pow(2.0f, mouse_wheel_y_delta * 0.15f);
    base_speed = glm::clamp(base_speed, Config::MIN_BASE_SPEED, Config::MAX_BASE_SPEED);

    glm::vec3 pos = transform.get_world_position();

    if (is_2d_axis_movement) {
        // 2D Axis Movement
        const bool sprint = input.is_action_pressed(SPRINT);
        const glm::vec3 horizontal_move = transform.get_right() * dx * cam_sensitivity * 0.1f;
        const glm::vec3 direction = sprint ? transform.get_forward() : transform.get_up();
        const glm::vec3 vertical_move = direction * -dy * cam_sensitivity * 0.1f;
        pos += (horizontal_move + vertical_move);
    } else {
        // Movement (WASD + QE)
        glm::vec3 move_dir = {0.0f, 0.0f, 0.0f};
        if (input.is_action_pressed(FORWARD)) move_dir += transform.get_forward();
        if (input.is_action_pressed(BACKWARD)) move_dir -= transform.get_forward();
        if (input.is_action_pressed(LEFT)) move_dir -= transform.get_right();
        if (input.is_action_pressed(RIGHT)) move_dir += transform.get_right();
        if (input.is_action_pressed(UP)) move_dir += transform.get_up();
        if (input.is_action_pressed(DOWN)) move_dir -= transform.get_up();

        if (glm::length(move_dir) > 0.0f) pos += glm::normalize(move_dir) * base_speed * time.delta_time;
    }

    transform.set_world_position(pos);
}

void CameraSystem::on_end() {}
}  // namespace tmt
