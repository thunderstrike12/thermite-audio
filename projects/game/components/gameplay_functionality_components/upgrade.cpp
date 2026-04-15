#include "upgrade.hpp"

#include "fuel.hpp"
#include "engine/core/components/button.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/mining_component.hpp"
#include "player.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/weapon.hpp"

namespace game {

void Upgrade::start() {
    // if entities are not set, try to set them automatically
    if (!tmt::engine.ecs.valid(player_entity)) {
        tmt::Log::error("Player entity was invalid, trying to get it automatically.");
        player_entity = tmt::engine.ecs.view<Player>().front().entity;  // Assuming there's only one player entity in the game
    }

    auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity);
    if (button_component) {
        tmt::Log::info("Found button component, adding apply function to button.");
        button_component->on_click.add(this, &Upgrade::button_apply);
    } else {
        tmt::Log::warn("No button found for upgrade!");
    }
}

void Upgrade::end() {
    auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity);
    if (button_component) {
        button_component->on_click.clear();
    }
}

void Upgrade::button_apply() {
    if (apply_upgrade()) {
        tmt::engine.ecs.disable(entity);
        tmt::Log::info("Applied upgrade.");

    } else {
        tmt::Log::warn("Did not apply upgrade.");
    }
}

bool Upgrade::apply_upgrade() {
    if (upgrade_target == entt::null) {
        tmt::Log::error("Upgrade has no valid target entity.");
        return false;  // no player entity in scene, return false
    }
    if (player_entity == entt::null) {
        tmt::Log::error("Upgrade could not find valid player entity.");
        return false;
    }

    auto wallet_component = tmt::engine.ecs.try_get_component<Wallet>(player_entity);

    if (!wallet_component) {
        tmt::Log::error("Player entity does not have valid Wallet component, could not apply upgrade.");
        return false;
    }

    for (auto& [resource, cost] : new_upgrade_costs) {
        if (wallet_component->currencies.resource_counts[resource] < cost) {
            tmt::Log::info("Unable to buy upgrade, insufficient {}.", magic_enum::enum_name(resource));
            return false;
        }
    }
    if (wallet_component->currencies.dollars <= dollar_cost) {
        tmt::Log::info("Unable to buy upgrade, insufficient {} dollars", wallet_component->currencies.dollars);
        return false;
    }

    for (auto& [resource, cost] : new_upgrade_costs) {
        wallet_component->currencies.resource_counts[resource] -= cost;
    }
    wallet_component->currencies.dollars -= dollar_cost;

    auto player_component = tmt::engine.ecs.try_get_component<Player>(upgrade_target);
    auto weapon_component = tmt::engine.ecs.try_get_component<Weapon>(upgrade_target);
    auto fuel_component = tmt::engine.ecs.try_get_component<FuelComponent>(upgrade_target);

    switch (type) {
        case UpgradeType::MAX_HEALTH:
            if (!player_component) {
                tmt::Log::error("Upgrade target does not have valid player component, could not apply max health upgrade.");
                return false;
            }
            player_component->health.max_value = upgrade_to;
            break;
        case UpgradeType::MAX_BATTERY:
            if (!player_component) {
                tmt::Log::error("Upgrade target does not have valid player component, could not apply max battery upgrade.");
                return false;
            }
            player_component->energy.max_value = upgrade_to;
            break;
        case UpgradeType::MAX_SPEED:
            if (!player_component) {
                tmt::Log::error("Upgrade target does not have valid player component, could not apply max speed upgrade.");
                return false;
            }
            player_component->max_speed = upgrade_to;
            break;
        case UpgradeType::ACCELERATION:
            if (!player_component) {
                tmt::Log::error("Upgrade target does not have valid player component, could not apply acceleration upgrade.");
                return false;
            }
            player_component->acceleration = upgrade_to;
            break;
        case UpgradeType::PRIMARY_ATK_SPEED:
            if (!weapon_component) {
                tmt::Log::error("Upgrade target does not have valid weapon component, could not apply primary atk speed upgrade");
                return false;
            }
            weapon_component->primary_fire_rate.shots_per_second = upgrade_to;
            break;
        case UpgradeType::SECONDARY_ATK_SPEED:
            if (!weapon_component) {
                tmt::Log::error("Upgrade target does not have valid weapon component, could not apply secondary atk speed upgrade");
                return false;
            }
            weapon_component->secondary_fire_rate.shots_per_second = upgrade_to;
            break;
        case UpgradeType::GUN_DMG:
            tmt::Log::warn("Weapon damage upgrade not implemented yet.");
            return false;
            break;
        case UpgradeType::BARGE_MAX_FUEL:
            if (!fuel_component) {
                tmt::Log::error("Upgrade target does not have valid fuel component, could not apply max barge fuel upgrade");
                return false;
            }
            fuel_component->max_fuel = upgrade_to;
            break;
        default:
            tmt::Log::error("Invalid upgrade type, could not apply upgrade.");
            return false;  // Invalid upgrade type, cannot apply upgrade
    }

    return true;           // Upgrade applied successfully
}

}  // namespace game
