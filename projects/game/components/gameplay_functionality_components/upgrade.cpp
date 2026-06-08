#include "upgrade.hpp"

#include "fuel.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/mining_component.hpp"
#include "player.hpp"
#include "projects/game/components/development_tools/save_data.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/weapon.hpp"
#include "weapon_and_tool_components/explosion.hpp"
#include "engine/systems/ui/ui.hpp"
#include "engine/steam/achievements.hpp"
#include "engine/steam/steam_api.hpp"

namespace game {

void Upgrade::start() {
    if (auto* name_component { tmt::engine.ecs.try_get_component<tmt::Name>(entity) }) {
        // check if this should be disabled
        if (tmt::engine.player_data.get<bool>(name_component->name, false)) {
            apply_entity_enable_disable();
            tmt::engine.ecs.disable(entity);
        }
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

void Upgrade::button_apply(tmt::Button::Context context) {
    if (context.disabled) return;

    for (auto required_upgrade_entity : required_upgrade_entities) {
        if (tmt::engine.ecs.is_enabled(required_upgrade_entity)) {
            tmt::Log::info("Required upgrades have not been purchased, canceling upgrade on entity: {}", entity);
            return;
        }
    }

    auto* ui = tmt::engine.ecs.systems.try_get<tmt::UI>();
    if (apply_upgrade()) {
        ui->menu_sounds.sounds.button_accept.play();
        tmt::engine.ecs.disable(entity);
        tmt::Log::info("Applied upgrade.");
        if (auto* name_component { tmt::engine.ecs.try_get_component<tmt::Name>(entity) }) {
            tmt::engine.player_data.get<bool>(name_component->name) = true;
        }

        // Check if we got an achievement for buying an upgrade
        check_achievement_unlock();
    } else {
        ui->menu_sounds.sounds.insufficient_funds.play();
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
            tmt::engine.ecs.try_get_component<Wallet>(Player::get().entity)->total_resource_limit = static_cast<int>(upgrade_to);
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
            tmt::engine.player_data.get<MiningData>(MINING_DATA).rays_per_second = upgrade_to;
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

void Upgrade::apply_entity_enable_disable() const {
    for (tmt::Entity curr_entity : entities_to_disable) {
        disable(curr_entity);
    }

    for (tmt::Entity curr_entity : entities_to_enable) {
        enable(curr_entity);
    }
}
void Upgrade::check_achievement_unlock() const {
    // TODO: dont make it dogshit

    // Check Drone upgrades
    bool drone_upgrades_unlocked = true;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_A1", false)) drone_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_E1", false)) drone_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_E2", false)) drone_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_E3", false)) drone_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_I1", false)) drone_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_I2", false)) drone_upgrades_unlocked = false;

    // Unlock drone upgrades achievement
    if (drone_upgrades_unlocked) {
        tmt::engine.steam.achievement->set_achievement("ACH_DRONE_UPGRADES");
    }

    // Check Drill upgrades
    bool drill_upgrades_unlocked = true;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_DS1", false)) drill_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_DS2", false)) drill_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_DS3", false)) drill_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_DSI1", false)) drill_upgrades_unlocked = false;

    // Unlock drill upgrades achievement
    if (drill_upgrades_unlocked) {
        tmt::engine.steam.achievement->set_achievement("ACH_DRILL_UPGRADES");
    }

    // Check Barge upgrades
    bool barge_upgrades_unlocked = true;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_F1", false)) barge_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_F2", false)) barge_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_F2 (1)", false)) barge_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_S1", false)) barge_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_S2", false)) barge_upgrades_unlocked = false;

    // Unlock barge upgrades achievement
    if (barge_upgrades_unlocked) {
        tmt::engine.steam.achievement->set_achievement("ACH_BARGE_UPGRADES");
    }

    // Check Rifle upgrades
    bool rifle_upgrades_unlocked = true;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_RP1", false)) rifle_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_RP2", false)) rifle_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_RS1", false)) rifle_upgrades_unlocked = false;
    if (!tmt::engine.player_data.get<bool>("B_Purchase_RS2", false)) rifle_upgrades_unlocked = false;

    // Unlock rifle upgrades achievement
    if (rifle_upgrades_unlocked) {
        tmt::engine.steam.achievement->set_achievement("ACH_RIFLE_UPGRADES");
    }
}

bool Upgrade::apply_upgrade() {
    auto* wallet_component = tmt::engine.ecs.try_get_component<Wallet>(game::Player::get().entity);

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

    modify_upgrade_entities();

    apply_entity_enable_disable();
    return true;
}

}  // namespace game
