#include "resource_bar_current.hpp"

#include "engine/core/components/text_renderer.hpp"
#include "projects/game/components/gameplay_functionality_components/fuel.hpp"

namespace game {

void ResourceBarCurrentComponent::start() {
    if (player_entity == entt::null) {
        player_entity = Player::get().entity;
    }
    if (barge_fuel_entity == entt::null) {
        auto view_fuel = tmt::engine.ecs.view<FuelComponent>();
        barge_fuel_entity = view_fuel.front().entity;
    }

    auto* player_component = tmt::engine.ecs.try_get_component<Player>(player_entity);
    if (!player_component) {
        tmt::Log::error("Player component cannot be found for resource bar, please check entity: {} (resource bar) and {} (player)", entity, player_entity);
        return;
    }
    auto barge_fuel_component = tmt::engine.ecs.try_get_component<FuelComponent>(barge_fuel_entity);
    if (!barge_fuel_component) {
        tmt::Log::error("Barge fuel component missing for resource bar, please check entity {} (resource bar) and {} (player)", entity, barge_fuel_entity);
    }

    switch (resource) {
        case DisplayTypeResourceBar::HEALTH:
            tmt::engine.ecs.get_dispatcher().sink<PlayerHealthChanged>().connect<&ResourceBarCurrentComponent::health_changed>(this);
            resource_bar_update(static_cast<int>(player_component->health.value), player_component->health.max_value);
            break;
        case DisplayTypeResourceBar::ENERGY:
            tmt::engine.ecs.get_dispatcher().sink<PlayerEnergyChanged>().connect<&ResourceBarCurrentComponent::energy_changed>(this);
            resource_bar_update(static_cast<int>(player_component->energy.value), player_component->energy.max_value);
            break;
        case DisplayTypeResourceBar::FUEL:
            tmt::engine.ecs.get_dispatcher().sink<BargeFuelChanged>().connect<&ResourceBarCurrentComponent::fuel_changed>(this);
            resource_bar_update(static_cast<int>(barge_fuel_component->current_fuel), barge_fuel_component->fuel_data.max_fuel);
            break;
        default:
            return;
            break;
    }
}

void ResourceBarCurrentComponent::update(const tmt::FrameData& time) {}

void ResourceBarCurrentComponent::end() {
    if (resource == DisplayTypeResourceBar::HEALTH) {
        tmt::engine.ecs.get_dispatcher().sink<PlayerHealthChanged>().disconnect<&ResourceBarCurrentComponent::health_changed>(this);
    } else if (resource == DisplayTypeResourceBar::ENERGY) {
        tmt::engine.ecs.get_dispatcher().sink<PlayerEnergyChanged>().disconnect<&ResourceBarCurrentComponent::energy_changed>(this);
    } else if (resource == DisplayTypeResourceBar::FUEL) {
        tmt::engine.ecs.get_dispatcher().sink<BargeFuelChanged>().disconnect<&ResourceBarCurrentComponent::fuel_changed>(this);
    }
}

void ResourceBarCurrentComponent::health_changed(PlayerHealthChanged player_health_changed) {
    int new_val = static_cast<int>(player_health_changed.new_value);
    auto* player_component = tmt::engine.ecs.try_get_component<Player>(player_entity);
    if (!player_component) {
        tmt::Log::error("Player component cannot be found for resource bar, please check entity: {} (resource bar) and {} (player)", entity, player_entity);
        return;
    }
    float max_resource_value = player_component->health.max_value;
    resource_bar_update(new_val, max_resource_value);
}

void ResourceBarCurrentComponent::energy_changed(PlayerEnergyChanged player_energy_changed) {
    int new_val = static_cast<int>(player_energy_changed.new_value);
    auto* player_component = tmt::engine.ecs.try_get_component<Player>(player_entity);
    if (!player_component) {
        tmt::Log::error("Player component cannot be found for resource bar, please check entity: {} (resource bar) and {} (player)", entity, player_entity);
        return;
    }
    float max_resource_value = player_component->energy.max_value;

    resource_bar_update(new_val, max_resource_value);
}

void ResourceBarCurrentComponent::fuel_changed(BargeFuelChanged fuel_changed) {
    int new_val = static_cast<int>(fuel_changed.new_value);

    float max_resource_value = fuel_changed.max_value;

    resource_bar_update(new_val, max_resource_value);
}

void ResourceBarCurrentComponent::resource_bar_update(int new_val, float max_resource_value) {
    auto* text_renderer_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(entity);

    // guards
    if (!text_renderer_component) {
        tmt::Log::error("Text renderer not found for resource bar, please check entity: {}", entity);
        return;
    }
    if (resource_per_segment == 0) {
        tmt::Log::error("Resource per segment cannot be 0, please check entity: {}", entity);
        return;
    }

    // segment counts
    int displayed_bars = new_val / resource_per_segment;
    int max_segments = static_cast<int>(max_resource_value) / resource_per_segment;

    if (new_val > 0 && displayed_bars <= 0) {
        displayed_bars = 1;
    }

    std::string render_string;

    for (int i = 0; i < displayed_bars; i++) {
        render_string.append(rendered_string);
    }

    text_renderer_component->text = render_string;
    if (max_segments == 1) {
        tmt::Log::warn("Cannot set letter spacing because max segments is 1. Check entity {}", entity);
        return;
    }
    float max_w = text_renderer_component->max_width;
    float font_size_in_pixels = text_renderer_component->font_size;
    tmt::ResourceRef<tmt::Font> curr_font = text_renderer_component->font;
    float segment_width = curr_font->measure_text(rendered_string, font_size_in_pixels);
    float chars_per_segment = static_cast<float>(rendered_string.size());
    text_renderer_component->letter_spacing = ((max_w - (segment_width * static_cast<float>(max_segments))) / (static_cast<float>(max_segments) - 1.0f)) / chars_per_segment;
}

}  // namespace game