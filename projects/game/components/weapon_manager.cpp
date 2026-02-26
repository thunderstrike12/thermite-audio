#include "weapon_manager.hpp"
#include "game_input.hpp"
#include "weapon.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"

void game::WeaponManager::switch_to(WeaponType weapon_slot) {
    if (weapon_slot == current_weapon) {
        tmt::Log::info("[WeaponManager] Already on {}, ignoring switch", magic_enum::enum_name(current_weapon));
        return;
    }
    tmt::Log::info("[WeaponManager] Switching: {} -> {}", magic_enum::enum_name(current_weapon), magic_enum::enum_name(weapon_slot));
    unsubscribe_weapon(current_weapon);
    current_weapon = weapon_slot;
    subscribe_weapon(current_weapon);
}

void game::WeaponManager::start() {
    current_weapon = starting_weapon;
    tmt::Log::info("[WeaponManager] Starting with weapon: {}", magic_enum::enum_name(starting_weapon));

    subscribe_weapon(current_weapon);
}

void game::WeaponManager::update(const tmt::FrameData& time) {
    auto& input = tmt::engine.input;

    if (input.is_action_pressed(action::SHOOT)) {
        tmt::engine.ecs.get_dispatcher().trigger(ShootEvent { shooting_entity, false });
    }
    if (input.is_action_pressed(action::SECONDARY_TOOL_USE)) {
        tmt::engine.ecs.get_dispatcher().trigger(ShootEvent { shooting_entity, true });
    }

    if (input.is_action_just_pressed(action::SWITCH_RIFLE)) {
        switch_to(WeaponType::RIFLE);
    } else if (input.is_action_just_pressed(action::SWITCH_MINING)) {
        switch_to(WeaponType::MINING);
    } else if (input.is_action_just_pressed(action::SWITCH_GRAVITY)) {
        switch_to(WeaponType::GRAVITY);
    }
}

void game::WeaponManager::end() {
    tmt::Log::info("[WeaponManager] Ending, unsubscribing {}", magic_enum::enum_name(current_weapon));
    unsubscribe_weapon(current_weapon);
}

void game::WeaponManager::subscribe_weapon(WeaponType slot) {
    auto e = weapons.at(slot);
    if (e == entt::null) {
        tmt::Log::warn("[WeaponManager] subscribe_weapon({}): entity is null", magic_enum::enum_name(slot));
        return;
    }
    auto* weapon = tmt::engine.ecs.try_get_component<Weapon>(e);
    if (!weapon) {
        tmt::Log::warn("[WeaponManager] subscribe_weapon({}): entity {} has no Weapon component", magic_enum::enum_name(slot), static_cast<uint32_t>(e));
        return;
    }
    tmt::Log::info("[WeaponManager] Subscribed {} (entity: {})", magic_enum::enum_name(slot), static_cast<uint32_t>(e));
    tmt::engine.ecs.get_dispatcher().sink<ShootEvent>().connect<&Weapon::on_shoot>(weapon);
}

void game::WeaponManager::unsubscribe_weapon(WeaponType slot) {
    auto e = weapons.at(slot);
    if (e == entt::null) {
        tmt::Log::warn("[WeaponManager] unsubscribe_weapon({}): entity is null", magic_enum::enum_name(slot));
        return;
    }
    auto* weapon = tmt::engine.ecs.try_get_component<Weapon>(e);
    if (!weapon) {
        tmt::Log::warn("[WeaponManager] unsubscribe_weapon({}): entity {} has no Weapon component", magic_enum::enum_name(slot), static_cast<uint32_t>(e));
        return;
    }
    tmt::Log::info("[WeaponManager] Unsubscribed {} (entity: {})", magic_enum::enum_name(slot), static_cast<uint32_t>(e));
    tmt::engine.ecs.get_dispatcher().sink<ShootEvent>().disconnect<&Weapon::on_shoot>(weapon);
}
