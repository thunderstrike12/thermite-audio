#include "upgrade.hpp"

#include "fuel.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/mining_component.hpp"
#include "player.hpp"
#include "projects/game/components/development_tools/save_data.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/weapon.hpp"
#include "weapon_and_tool_components/explosion.hpp"

namespace game {

void Upgrade::start() {
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

void Upgrade::button_apply(tmt::Button::Context context) {
    if (context.disabled) return;

    if (apply_upgrade()) {
        tmt::engine.ecs.disable(entity);
        tmt::Log::info("Applied upgrade.");

    } else {
        tmt::Log::warn("Did not apply upgrade.");
    }
}

void Upgrade::modify_upgrade_entities() const {
    switch (type) {
        case UpgradeType::MAX_HEALTH:
            tmt::engine.player_data.get<PlayerStat>(PLAYER_HEALTH_DATA).max_value = upgrade_to;
            break;
        case UpgradeType::MAX_BATTERY:
            tmt::engine.player_data.get<PlayerStat>(PLAYER_ENERGY_DATA).max_value = upgrade_to;
            break;
        case UpgradeType::MAX_SPEED:
            tmt::engine.player_data.get<PlayerMovement>(PLAYER_MOVEMENT_DATA).max_speed = upgrade_to;
            break;
        case UpgradeType::MAX_BOOST_SPEED:
            tmt::engine.player_data.get<PlayerMovement>(PLAYER_MOVEMENT_DATA).boost_max_speed_multiplier = upgrade_to;
            break;
        case UpgradeType::ACCELERATION:
            tmt::engine.player_data.get<PlayerMovement>(PLAYER_MOVEMENT_DATA).acceleration = upgrade_to;

            break;
        case UpgradeType::HEALTH_RESTORE_SPEED:
            tmt::engine.player_data.get<PlayerStat>(PLAYER_HEALTH_DATA).increase_multiplier = upgrade_to;
            break;
        case UpgradeType::BATTERY_RESTORE_SPEED:
            tmt::engine.player_data.get<PlayerStat>(PLAYER_ENERGY_DATA).increase_multiplier = upgrade_to;
            break;
        case UpgradeType::EMERGENCY_BATTERY_TIME:
            tmt::engine.player_data.get<PlayerRecharge>(PLAYER_RECHARGE_DATA).out_of_energy_time_till_death = upgrade_to;
            break;
        case UpgradeType::STORAGE_LIMIT:
            tmt::engine.player_data.get<WalletData>(WALLET_DATA).total_resource_limit = static_cast<int>(upgrade_to);
            break;

        case UpgradeType::BARGE_MAX_FUEL:
            tmt::engine.player_data.get<FuelData>(FUEL_DATA).max_fuel = upgrade_to;
            break;
        case UpgradeType::BARGE_SPEED:
            tmt::engine.player_data.get<float>(BARGE_MOVE_DATA) = upgrade_to;
            break;
        case UpgradeType::BARGE_RECHARGE_DISTANCE:
            tmt::engine.player_data.get<PlayerRecharge>(PLAYER_RECHARGE_DATA).recharge_distance = upgrade_to;
            break;

        case UpgradeType::DRILL_RADIUS:
            tmt::engine.player_data.get<MiningData>(MINING_DATA).base_radius = upgrade_to;
            break;
        case UpgradeType::DRILL_DISTANCE:
            tmt::engine.player_data.get<MiningData>(MINING_DATA).ray_distance = upgrade_to;
            break;
        case UpgradeType::DRILL_CONSISTENCY:
            tmt::engine.player_data.get<MiningData>(MINING_DATA).ray_amount = static_cast<uint32_t>(upgrade_to);
            break;
        case UpgradeType::DRILL_SPEED:
            tmt::engine.player_data.get<MiningData>(RIFLE_FIRE_DATA).rays_per_second = upgrade_to;
            break;

        // ── Gravity Tool ──
        case UpgradeType::MAX_GRAB_SIZE:
            tmt::engine.player_data.get<float>(GRAVITY_GUN_DATA) = upgrade_to;
            break;

        // ── Rifle ──
        case UpgradeType::RIFLE_IMPACT_RADIUS:
            tmt::engine.player_data.get<ExplosionParameters>(RIFLE_PROJECTILE_DATA).radius = upgrade_to;
            break;
        case UpgradeType::RIFLE_EXPLOSION_POWER:
            tmt::engine.player_data.get<ExplosionParameters>(RIFLE_PROJECTILE_DATA).explosion_power = upgrade_to;
            break;
        default:
            tmt::Log::warn("Unknown upgrade type {}", magic_enum::enum_name(type));
            break;
    }
}

bool Upgrade::apply_upgrade() {
    auto& currencies { tmt::engine.player_data.get<Currencies>(PERSISTENT_RESOURCES) };

    for (auto& [resource, cost] : new_upgrade_costs) {
        if (currencies.resource_counts[resource] < cost) {
            tmt::Log::info("Unable to buy upgrade, insufficient {}.", magic_enum::enum_name(resource));
            return false;
        }
    }
    if (currencies.dollars <= dollar_cost) {
        tmt::Log::info("Unable to buy upgrade, insufficient {} dollars", currencies.dollars);
        return false;
    }

    for (auto& [resource, cost] : new_upgrade_costs) {
        currencies.resource_counts[resource] -= cost;
    }
    currencies.dollars -= dollar_cost;

    modify_upgrade_entities();

    return true;
}

}  // namespace game
