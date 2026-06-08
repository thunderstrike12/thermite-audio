#include "sell_ore_component.hpp"
#include "engine/core/components/button.hpp"
#include "engine/systems/ui/ui.hpp"

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

    auto* ui = tmt::engine.ecs.systems.try_get<tmt::UI>();

    if (wallet_component->currencies.resource_counts.at(ore_entry.ore_resource) > 1) {
        wallet_component->currencies.resource_counts.at(ore_entry.ore_resource) -= 1;
        wallet_component->currencies.dollars += static_cast<int>(ore_entry.value);
        if (!ui->money_gained_instance.is_valid()) ui->money_gained_instance = ui->menu_sounds.sounds.money_gained.play();
    } else {
        if (!ui->insufficient_funds_instance.is_valid()) ui->insufficient_funds_instance = ui->menu_sounds.sounds.insufficient_funds.play();
    }
}

void SellOreComponent::down_toggle(tmt::Button::Context) {
    is_held = true;
}

void SellOreComponent::up_toggle(tmt::Button::Context context) {
    if (context.disabled) return;

    auto* ui = tmt::engine.ecs.systems.try_get<tmt::UI>();
    ui->money_gained_instance.stop();

    // reset all values for rampup
    timer = 0.0f;
    current_cooldown = initial_hold_cooldown;
    is_held = false;
}

}  // namespace game
