#include "pin_upgrade_button.hpp"

namespace game {

void UpgradePinButtonComponent::start() {
    tmt::engine.ecs.get_dispatcher().sink<UpgradesWasPurchasedEvent>().connect<&UpgradePinButtonComponent::handle_purchase_event>(this);

    auto& saved_upgrade_pins = tmt::engine.player_data.get<SavedUpgradePins>(PINNED_UPGRADES, SavedUpgradePins());

    for (auto& entry : saved_upgrade_pins.pinned_upgrades) {
        if (!tmt::engine.ecs.valid(entry.upgrade_pin_button_entity)) {
            entry.active = false;
        }

        if (entity == entry.upgrade_pin_button_entity) {
            if (entry.active) {
                is_pinned = true;
            } else {
                is_pinned = false;
            }
        }
    }

    auto* text_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(entity);
    if (!text_component) {
        return;
    }

    if (is_pinned) {
        text_component->text = "UNPIN";
    } else {
        text_component->text = "PIN";
    }
    if (upgrade_purchased) {
        text_component->text = "---";
    }

    auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity);
    if (!button_component) {
        return;
    }

    button_component->on_click.add(this, &UpgradePinButtonComponent::handle_button_press);

    auto* upgrade_component = tmt::engine.ecs.try_get_component<Upgrade>(upgrade_entity);
    if (!upgrade_component) {
        return;
    }

    // pinned upgrade data setup
    pinned_upgrade_data.active = is_pinned;
    pinned_upgrade_data.upgrade_pin_button_entity = entity;
    pinned_upgrade_data.shown_name = shown_name;
    pinned_upgrade_data.dollar_cost = static_cast<int>(upgrade_component->dollar_cost);
    for (const auto& [resource, cost] : upgrade_component->new_upgrade_costs) {
        switch (resource) {
            case OreProperties::OreResources::COPPER:
                pinned_upgrade_data.copper_cost = static_cast<int>(cost);
                break;
            case OreProperties::OreResources::SCRAP:
                pinned_upgrade_data.scrap_cost = static_cast<int>(cost);
                break;
            case OreProperties::OreResources::THERMITE:
                pinned_upgrade_data.thermite_cost = static_cast<int>(cost);
                break;
            case OreProperties::OreResources::TITANIUM:
                pinned_upgrade_data.titanium_cost = static_cast<int>(cost);
                break;
            case OreProperties::OreResources::NONE:
                break;
        }
    }
}

void UpgradePinButtonComponent::update(const tmt::FrameData& time) {}

void UpgradePinButtonComponent::end() {
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        button_component->on_click.clear();
    }
}

bool UpgradePinButtonComponent::add_pin_to_player_data() {
    if (upgrade_purchased) return false;
    auto& saved_upgrade_pins = tmt::engine.player_data.get<SavedUpgradePins>(PINNED_UPGRADES, SavedUpgradePins());

    for (auto& entry : saved_upgrade_pins.pinned_upgrades) {
        if (!entry.active) {
            // found inactive pin, replace with this pin
            is_pinned = true;
            pinned_upgrade_data.active = true;
            entry = pinned_upgrade_data;
            break;
        }
    }

    if (!is_pinned) {
        tmt::Log::info("[Upgrade pins] Could not pin upgrade, max upgrades reached.");
        return false;
    }
    return true;
}

void UpgradePinButtonComponent::remove_pin_from_player_data() {
    auto& saved_upgrade_pins = tmt::engine.player_data.get<SavedUpgradePins>(PINNED_UPGRADES, SavedUpgradePins());

    for (auto& entry : saved_upgrade_pins.pinned_upgrades) {
        if (entry.upgrade_pin_button_entity == entity) {
            // found pin with same entity, set to false
            is_pinned = false;
            entry.active = false;
        }
    }

    if (is_pinned) {
        tmt::Log::error("[Upgrade pins] Could not unpin upgrade. Something went wrong, probably not set up correctly.");
    }
}

void UpgradePinButtonComponent::handle_button_press(tmt::Button::Context context) {
    if (upgrade_purchased) return;
    auto* text_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(entity);
    if (!text_component) {
        return;
    }

    // Something is wrong heres
    if (is_pinned) {
        remove_pin_from_player_data();
        text_component->text = "PIN";
    } else {
        if (add_pin_to_player_data()) {
            text_component->text = "UNPIN";
        }
    }
}

void UpgradePinButtonComponent::handle_purchase_event(UpgradesWasPurchasedEvent event_data) {
    tmt::Entity purchased_upgrade = event_data.purchased_upgrade_entity;
    auto* text_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(entity);
    if (!text_component) {
        return;
    }
    if (purchased_upgrade == upgrade_entity) {
        upgrade_purchased = true;
        is_pinned = false;
        text_component->text = "---";
        remove_pin_from_player_data();
    }
}

}  // namespace game