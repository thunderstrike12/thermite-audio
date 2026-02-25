#include "text_component.hpp"

#include "engine/core/polyline.hpp"
void game::TextComponent::draw_text() const {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    config.set_values();
    tmt::engine.polyline.draw_text(transform.get_world_position(), text, scale);
}
void game::TextComponent::update(const tmt::FrameData& time) {
    draw_text();
}
void game::TextComponent::draw_debug_lines() const {
    draw_text();
}
