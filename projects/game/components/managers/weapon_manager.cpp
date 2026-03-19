#include "weapon_manager.hpp"
#include "projects/game/data_headers/game_input.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/weapon.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"

void game::WeaponManager::switch_to(WeaponType weapon_slot) {
    if (weapon_slot == current_weapon) {
        tmt::Log::info("[WeaponManager] Already on {}, ignoring switch", magic_enum::enum_name(current_weapon));
        return;
    }
    tmt::Log::info("[WeaponManager] Switching: {} -> {}", magic_enum::enum_name(current_weapon), magic_enum::enum_name(weapon_slot));

    // unsubscribe only the first time we are in a transition stage
    if (switching == false) {
        unsubscribe_weapon(current_weapon);
    }

    // the new subscription happens when the switching is done
    switching_remaining_time = switching_time;
    pending_weapon = weapon_slot;
    switching = true;
}

void game::WeaponManager::set_new_weapon(game::WeaponType weapon_slot) {
    current_weapon = weapon_slot;
    subscribe_weapon(current_weapon);
}

void game::WeaponManager::start() {
    current_weapon = starting_weapon;
    tmt::Log::info("[WeaponManager] Starting with weapon: {}", magic_enum::enum_name(starting_weapon));

    subscribe_weapon(current_weapon);

    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().connect<&WeaponManager::on_overheat>(this);
}

void game::WeaponManager::end() {
    tmt::Log::info("[WeaponManager] Ending, unsubscribing {}", magic_enum::enum_name(current_weapon));
    unsubscribe_weapon(current_weapon);
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().disconnect<&WeaponManager::on_overheat>(this);
}

void game::WeaponManager::on_overheat(const game::WeaponFiredEvent& event) {
    auto weapon_entity = weapons.at(current_weapon);
    if (event.weapon_entity != weapon_entity) {
        return;
    }
    // Here as an example we overheat after a secondary shot, so we cannot do anything for less than a second
    if (event.secondary_shot) {
        overheat_remaining_time = overheat_time;
    }
}

void game::WeaponManager::check_trigger_shoot_event() {
    auto& input = tmt::engine.input;

    if (input.is_action_pressed(action::SHOOT)) {
        tmt::engine.ecs.get_dispatcher().trigger(ShootEvent { shooting_entity, false });
    } else if (input.is_action_just_released(action::SHOOT)) {
        tmt::engine.ecs.get_dispatcher().trigger(ReleaseShootEvent { shooting_entity });
    }
}

void game::WeaponManager::complete_switch() {
    current_weapon = pending_weapon;
    switching = false;
    subscribe_weapon(current_weapon);
}
void game::WeaponManager::update(const tmt::FrameData& time) {
    // TODO replace with proper state

    if (switching) {
        switching_remaining_time -= tmt::engine.frame_data().delta_time;

        if (switching_remaining_time < 0.0f) {
            complete_switch();
        }
    }
    if (switching == false) {
        switch (current_weapon) {
            case game::WeaponType::RIFLE:
                check_trigger_shoot_event();

                break;
            case game::WeaponType::GRAVITY:
                if (overheat_remaining_time < 0.0f) {
                    check_trigger_shoot_event();
                }
                if (tmt::engine.input.is_action_pressed(action::SECONDARY_TOOL_USE)) {
                    tmt::engine.ecs.get_dispatcher().trigger(ShootEvent { shooting_entity, true });
                }
                break;
            case game::WeaponType::MINING:
                check_trigger_shoot_event();

                break;
        }
    }

    transition_to_other_weapons();

    // state timers update
    if (overheat_remaining_time > 0.0f) {
        overheat_remaining_time -= tmt::engine.frame_data().delta_time;
    }
}
void game::WeaponManager::transition_to_other_weapons() {
    auto& input = tmt::engine.input;

    if (input.is_action_just_pressed(action::SWITCH_RIFLE)) {
        switch_to(WeaponType::RIFLE);
    } else if (input.is_action_just_pressed(action::SWITCH_MINING)) {
        switch_to(WeaponType::MINING);
    } else if (input.is_action_just_pressed(action::SWITCH_GRAVITY)) {
        switch_to(WeaponType::GRAVITY);
    }
}

void game::WeaponManager::subscribe_weapon(WeaponType slot) {
    auto e = weapons.at(slot);
    if (e == entt::null) {
        tmt::Log::warn("[WeaponManager] subscribe_weapon({}): entity is null", magic_enum::enum_name(slot));
        return;
    }
    tmt::engine.ecs.enable(e);
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
    tmt::engine.ecs.disable(e);
    auto* weapon = tmt::engine.ecs.try_get_component<Weapon>(e);
    if (!weapon) {
        tmt::Log::warn("[WeaponManager] unsubscribe_weapon({}): entity {} has no Weapon component", magic_enum::enum_name(slot), static_cast<uint32_t>(e));
        return;
    }
    tmt::Log::info("[WeaponManager] Unsubscribed {} (entity: {})", magic_enum::enum_name(slot), static_cast<uint32_t>(e));
    tmt::engine.ecs.get_dispatcher().sink<ShootEvent>().disconnect<&Weapon::on_shoot>(weapon);
    tmt::engine.ecs.get_dispatcher().trigger(ReleaseShootEvent { shooting_entity });
}
