#include "entity_control_component.hpp"
#include "engine/core/components/button.hpp"

void game::EntityControlComponent::start() {
    if (auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        tmt::Log::info("Found button component, adding switch scene to on click.");
        button_component->on_click.add(this, &EntityControlComponent::button_functionality);
    } else {
        tmt::Log::warn("No button found for entity control component.");
    }
}

void game::EntityControlComponent::update(const tmt::FrameData& time) {}

void game::EntityControlComponent::end() {
    if (auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        button_component->on_click.clear();
    }
}

void game::EntityControlComponent::button_functionality(tmt::Button::Context context) {
    if (context.disabled) return;
    for (auto currett : entities_to_enable) {
        enable(currett);
    }
    for (auto currett : entities_to_disable) {
        disable(currett);
    }
}

void game::EntityControlComponent::disable(tmt::Entity ett_to_dis) {
    if (tmt::engine.ecs.valid(ett_to_dis)) {
        tmt::engine.ecs.disable(ett_to_dis);
    }
}

void game::EntityControlComponent::enable(tmt::Entity ett_to_en) {
    if (tmt::engine.ecs.valid(ett_to_en)) {
        tmt::engine.ecs.enable(ett_to_en);
    }
}