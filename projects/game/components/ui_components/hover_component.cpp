#include "hover_component.hpp"

void game::HoverComponent::start() {
    if (hover_entity == entt::null) {
        tmt::Log::error("Hover entity not set for {}", entity);
        return;
    }

    if (!tmt::engine.ecs.valid(hover_entity)) {
        tmt::Log::error("Hover entity on {} is invalid ({})", entity, hover_entity);
        return;
    }

    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        button_component->on_select.add(this, &HoverComponent::enable);
        button_component->on_deselect.add(this, &HoverComponent::disable);
    } else {
        tmt::Log::warn("No button found on entity {}", entity);
    }
}

void game::HoverComponent::update(const tmt::FrameData& time) {}

void game::HoverComponent::end() {
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        button_component->on_select.clear();
        button_component->on_deselect.clear();
    } else {
        tmt::Log::warn("No button found on entity {}", entity);
    }
}

void game::HoverComponent::enable(tmt::Button::Context) {
    if (hover_entity == entt::null) {
        tmt::Log::error("Hover entity not set for {}", entity);
        return;
    }

    if (!tmt::engine.ecs.valid(hover_entity)) {
        tmt::Log::error("Hover entity on {} is invalid ({})", entity, hover_entity);
        return;
    }

    tmt::engine.ecs.enable(hover_entity);
}

void game::HoverComponent::disable(tmt::Button::Context) {
    if (hover_entity == entt::null) {
        tmt::Log::error("Hover entity not set for {}", entity);
        return;
    }

    if (!tmt::engine.ecs.valid(hover_entity)) {
        tmt::Log::error("Hover entity on {} is invalid ({})", entity, hover_entity);
        return;
    }

    tmt::engine.ecs.disable(hover_entity);
}