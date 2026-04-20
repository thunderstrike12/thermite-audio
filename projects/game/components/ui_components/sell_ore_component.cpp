#include "sell_ore_component.hpp"

namespace game {

void SellOreComponent::start() {
    if (!entity_check()) {
        return;
    }

    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        button_component->on_click.add(this, &SellOreComponent::down_toggle);
        button_component->on_release.add(this, &SellOreComponent::up_toggle);
    } else {
        tmt::Log::warn("No button found on entity {}", entity);
    }
}

void SellOreComponent::update(const tmt::FrameData& time) {
    if (is_held) {
        timer -= time.delta_time;

        if (timer <= 0.0f) {
            sell_ore();
            current_cooldown /= speed_up_factor;
            timer = current_cooldown;
        }
    }
}

void SellOreComponent::end() {
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        button_component->on_click.clear();
        button_component->on_release.clear();
    } else {
        tmt::Log::warn("No button found on entity {}", entity);
    }
}

bool SellOreComponent::entity_check() const {
    if (entity_with_wallet == entt::null) {
        tmt::Log::error("Wallet entity missing on {}", entity);
        return false;
    }
    if (ore_property_entity == entt::null) {
        tmt::Log::error("OreProperty entity missing on {}", entity);
        return false;
    }
    return true;
}

void SellOreComponent::sell_ore() {
    auto* wallet_component = tmt::engine.ecs.try_get_component<Wallet>(entity_with_wallet);
    auto ore_entry = tmt::engine.ecs.try_get_component<OreProperties>(ore_property_entity)->ores.at(material_to_sell);

    if (wallet_component->currencies.resource_counts.at(ore_entry.ore_resource) > 1) {
        wallet_component->currencies.resource_counts.at(ore_entry.ore_resource) -= 1;
        wallet_component->currencies.dollars += static_cast<int>(ore_entry.value);
    }
}

void SellOreComponent::down_toggle() {
    is_held = true;
}

void SellOreComponent::up_toggle() {
    // reset all values for rampup
    timer = 0.0f;
    current_cooldown = initial_hold_cooldown;
    is_held = false;
}

}  // namespace game