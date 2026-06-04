#include "waypoint_component.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/image_renderer.hpp"
#include "engine/core/components/ui_component.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"
void game::Waypoint::start() {
    if (entity_waypoint == entt::null) {
        entity_waypoint = entity;
    }
}
void game::Waypoint::update(const tmt::FrameData& time) {
    auto transform { tmt::engine.ecs.get_component<tmt::Transform>(entity_waypoint) };
    auto world_pos { transform.get_world_position() };
    auto clip_pos { tmt::engine.renderer.render_view.gpu_view.world_to_clip * glm::vec4(world_pos, 1.0f) };
    glm::vec3 screen_pos { clip_pos.x, clip_pos.y, 0.0f };
    auto& image = tmt::engine.ecs.get_component<tmt::ImageRenderer>(entity);

    // behind
    if (clip_pos.w <= 0.0f) {
        image.color.get().a = 0.0f;
        return;
    }
    screen_pos /= clip_pos.w;
    // remove jitter
    screen_pos.x -= tmt::engine.renderer.render_view.gpu_view.jitter.x;
    screen_pos.y -= tmt::engine.renderer.render_view.gpu_view.jitter.y;

    screen_pos *= glm::vec3 { tmt::UIComponent::REFERENCE_WIDTH * 0.5f, tmt::UIComponent::REFERENCE_HEIGHT * 0.5f, 1.0f };
    tmt::engine.ecs.get_component<tmt::Transform>(entity).set_world_position(screen_pos);

    const float distance = glm::length(world_pos - Player::get().get_transform().get_world_position());

    const float fade_span = glm::max(distance_to_fade_out - hide_radius, 1.0f);

    const float fade_t = glm::clamp((distance - hide_radius) / distance_to_fade_out, 0.0f, 1.0f);
    image.color.get().a = fade_curve.eval(fade_t);

    const float size_span = glm::max(distance_to_min_size - distance_to_fade_out, 1e-4f);
    const float size_t = glm::clamp((distance - distance_to_fade_out) / size_span, 0.0f, 1.0f);
    const float size_px = glm::mix(max_size_icon, min_size_icon, size_curve.eval(size_t));
    tmt::engine.ecs.get_component<tmt::UIComponent>(entity).size = glm::vec2(size_px);
}
