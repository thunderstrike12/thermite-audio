#include "camera_system.hpp"

#include "engine.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"
#include "core/components/camera.hpp"
#include "core/components/transform.hpp"
#include "engine/core/keys.hpp"

namespace tmt {
void CameraSystem::on_start() {
    // Setup Input Actions
    engine.input.add_action_keys(SPRINT, Key::LEFT_SHIFT);
    engine.input.add_action_keys(FORWARD, Key::W);
    engine.input.add_action_keys(BACKWARD, Key::S);
    engine.input.add_action_keys(RIGHT, Key::D);
    engine.input.add_action_keys(LEFT, Key::A);
    engine.input.add_action_keys(UP, Key::E);
    engine.input.add_action_keys(DOWN, Key::Q);
    engine.input.add_action_keys(UP, Key::SPACE);
    engine.input.add_action_keys(DOWN, Key::LEFT_CTRL);
}

void CameraSystem::on_update(const FrameData& time) {
    // Gather Variables to be used
    auto& input = engine.input;
    Entity camera_entity = Camera::get_active_camera();
    if (camera_entity == entt::null) {
        Log::error(Log::Scope::ENGINE, "Camera Entity is NULL. Are there any active cameras in the scene?");
        return;
    }
    Camera& camera = engine.ecs.get_component<Camera>(camera_entity);
    Transform& transform = engine.ecs.get_component<Transform>(camera_entity);

    bool enable_mouse_look = input.is_action_pressed(action::RIGHT_CLICK);

    if (enable_mouse_look) {
        float dx = input.get_mouse_delta_x();
        float dy = input.get_mouse_delta_y();

        camera.yaw -= dx * cam_sensitivity;
        camera.pitch -= dy * cam_sensitivity;
        camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

        // Compute Vectors
        glm::vec3 front = {};
        front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        front.y = sin(glm::radians(camera.pitch));
        front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        front = glm::normalize(front);
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::normalize(glm::cross(right, front));

        // Compute world matrix
        const glm::mat4 world = glm::inverse(glm::lookAt(transform.get_world_position(), transform.get_world_position() + front, up));

        transform.set_world_matrix(world);
    }

    base_speed = 4.0f;
    // Movement (WASD + QE)
    if (input.is_action_pressed(SPRINT)) base_speed *= sprint_mult;

    glm::vec3 move_dir = {0.0f, 0.0f, 0.0f};
    if (input.is_action_pressed(FORWARD)) move_dir += transform.get_forward();
    if (input.is_action_pressed(BACKWARD)) move_dir -= transform.get_forward();
    if (input.is_action_pressed(LEFT)) move_dir -= transform.get_right();
    if (input.is_action_pressed(RIGHT)) move_dir += transform.get_right();
    if (input.is_action_pressed(UP)) move_dir += transform.get_up();
    if (input.is_action_pressed(DOWN)) move_dir -= transform.get_up();

    glm::vec3 pos = transform.get_world_position();
    if (glm::length(move_dir) > 0.0f) pos += glm::normalize(move_dir) * base_speed * time.delta_time;

    transform.set_world_position(pos);
}

void CameraSystem::on_end() {}
}  // namespace tmt
