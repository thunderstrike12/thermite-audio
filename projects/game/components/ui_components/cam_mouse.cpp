#include "cam_mouse.hpp"
#include "engine/engine.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/tools/fmt/glm.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace game {

void CameraMouse::start() {
    const auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    base_position = transform.get_world_position();
    base_right = transform.get_right();
    base_up = transform.get_up();

    initial_look_at_position = base_position + transform.get_forward() * distance;
}

void CameraMouse::update(const tmt::FrameData& time) {
    const glm::vec2 mouse_pos = tmt::engine.input.get_mouse_position();
    const glm::uvec2 screen_res = tmt::engine.renderer.render_view.gpu_view.resolution;

    glm::vec2 mouse_centered = (mouse_pos / glm::vec2(screen_res)) - 0.5f;
    mouse_centered = glm::clamp(mouse_centered, -1.0f, 1.0f);

    const glm::vec2 target_offset = { mouse_centered.x * sway_radius_x * (invert_x ? -1.0f : 1.0f), mouse_centered.y * sway_radius_y * (invert_y ? -1.0f : 1.0f) };

    const float alpha = 1.0f - glm::exp(-smoothing * time.delta_time);
    current_offset = glm::mix(current_offset, target_offset, alpha);

    const glm::vec3 swayed_pos = base_position + base_right * current_offset.x + base_up * current_offset.y;

    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    transform.set_world_position(swayed_pos);
    transform.look_at(initial_look_at_position, base_up);
}

void CameraMouse::end() {}

}  // namespace game