#include "upgrade.hpp"

#include "mining_component.hpp"
#include "player.hpp"
#include "weapon.hpp"

namespace game {

void Upgrade::start() {
    // if entities are not set, try to set them automatically
    if (player_entity==entt::null)
    player_entity = tmt::engine.ecs.view<Player>().front().entity;  // Assuming there's only one player entity in the game
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

    for (std::tuple<UpgradeResource, float> cost : upgrade_costs) {
        auto resource = std::get<0>(cost);
        auto upgrade_cost = std::get<1>(cost);
        switch (resource) {
            case UpgradeResource::DOLLARS:
                if (wallet_component->dollars < upgrade_cost) return false;
                wallet_component->dollars -= upgrade_cost;
                break;
            case UpgradeResource::GOLD:
                if (wallet_component->gold < upgrade_cost) return false;
                wallet_component->gold -= upgrade_cost;
                break;
            case UpgradeResource::SILVER:
                if (wallet_component->silver < upgrade_cost) return false;
                wallet_component->silver -= upgrade_cost;
                break;
            case UpgradeResource::COPPER:
                if (wallet_component->copper < upgrade_cost) return false;
                wallet_component->copper -= upgrade_cost;
                break;
            case UpgradeResource::IRON:
                if (wallet_component->iron < upgrade_cost) return false;
                wallet_component->iron -= upgrade_cost;
                break;
            case UpgradeResource::ENEMY_CORES:
                if (wallet_component->enemy_cores < upgrade_cost) return false;
                wallet_component->enemy_cores -= upgrade_cost;
                break;
            default:
                tmt::Log::warn("No valid resource type for this upgrade.");
                return false;  // Invalid resource type, cannot apply upgrade
        }
    }

    auto player_component = tmt::engine.ecs.try_get_component<Player>(upgrade_target);
    auto weapon_component = tmt::engine.ecs.try_get_component<Weapon>(upgrade_target);

    switch (type) {
        case UpgradeType::MAX_HEALTH:
            if (!player_component) {
                tmt::Log::error("Upgrade target does not have valid player component, could not apply max health upgrade.");
                return false;
            }
            player_component->max_health += upgrade_value;
            break;
        case UpgradeType::MAX_BATTERY:
            if (!player_component) {
                tmt::Log::error("Upgrade target does not have valid player component, could not apply max battery upgrade.");
                return false;
            }
            player_component->max_battery += upgrade_value;
            break;
        case UpgradeType::MAX_SPEED:
            if (!player_component) {
                tmt::Log::error("Upgrade target does not have valid player component, could not apply max speed upgrade.");
                return false;
            }
            player_component->max_speed += upgrade_value;
            break;
        case UpgradeType::ACCELERATION:
            if (!player_component) {
                tmt::Log::error("Upgrade target does not have valid player component, could not apply acceleration upgrade.");
                return false;
            }
            player_component->acceleration += upgrade_value;
            break;
        case UpgradeType::PRIMARY_ATK_SPEED:
            if (!weapon_component) {
                tmt::Log::error("Upgrade target does not have valid weapon component, could not apply primary atk speed upgrade");
                return false;
            }
            weapon_component->primary_fire_rate.shots_per_second += upgrade_value;
            break;
        case UpgradeType::SECONDARY_ATK_SPEED:
            if (!weapon_component) {
                tmt::Log::error("Upgrade target does not have valid weapon component, could not apply secondary atk speed upgrade");
                return false;
            }
            weapon_component->secondary_fire_rate.shots_per_second += upgrade_value;
            break;
        case UpgradeType::GUN_DMG:
            tmt::Log::warn("Weapon damage upgrade not implemented yet.");
            return false;
            break;
        case UpgradeType::BARGE_FUEL:
            tmt::Log::warn("Barge fuel upgrade not implemented yet.");
            return false;
            break;
        default:
            tmt::Log::error("Invalid upgrade type, could not apply upgrade.");
            return false;  // Invalid upgrade type, cannot apply upgrade
    }

    return true;           // Upgrade applied successfully
}

}                          // namespace game