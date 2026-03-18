#include "menu_controller.hpp"
#include "engine/core/input/input.hpp"
#include "projects/game/data_headers/game_input.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"

namespace game {

void MenuController::end() {}

void MenuController::update(const tmt::FrameData& time) {
    auto& input = tmt::engine.input;

    if (input.is_action_just_pressed(action::OPEN_INVENTORY)) {
        if (inventory_menu_entity == entt::null) {
            tmt::Log::error("No inventory menu entity has been set, cannot open inventory.");
            return;
        }

        if (tmt::engine.ecs.is_disabled(inventory_menu_entity) && tmt::engine.ecs.is_disabled(end_run_menu_entity)) {
            enable_inventory_menu();
        } else {
            disable_inventory_menu();
        }
    }

    if (input.is_action_just_pressed(action::OPEN_PAUSE_MENU)) {
        if (pause_menu_entity == entt::null) {
            tmt::Log::error("No pause menu entity has been set, cannot open pause menu.");
            return;
        }

        if (tmt::engine.ecs.is_disabled(pause_menu_entity) && tmt::engine.ecs.is_disabled(end_run_menu_entity)) {
            enable_pause_menu();
        } else {
            disable_pause_menu();
        }
    }

    if (input.is_action_just_pressed(action::OPEN_UPGRADE_MENU)) {
        if (upgrade_menu_entity == entt::null) {
            tmt::Log::error("No upgrade menu entity has been set, cannot open upgrade menu.");
            return;
        }

        if (tmt::engine.ecs.is_disabled(upgrade_menu_entity) && tmt::engine.ecs.is_disabled(end_run_menu_entity)) {
            enable_upgrade_menu();
        } else {
            disable_upgrade_menu();
        }
    }
}

void MenuController::start() {
    // Bind end run to event
    tmt::engine.ecs.get_dispatcher().sink<EndRun>().connect<&MenuController::enable_end_of_game_menu>(this);
}

void MenuController::enable_pause_menu() const {
    if (pause_menu_entity == entt::null) {
        tmt::Log::error("No pause menu found, please add the menu to the menu controller.");
        return;
    }
    if (check_for_open_menus()) return;
    unlock_mouse();
    tmt::engine.ecs.enable(pause_menu_entity);
}

void MenuController::disable_pause_menu() const {
    if (pause_menu_entity == entt::null) {
        tmt::Log::error("No pause menu found, please add the menu to the menu controller.");
        return;
    }
    lock_mouse();
    tmt::engine.ecs.disable(pause_menu_entity);
}

void MenuController::enable_inventory_menu() const {
    if (inventory_menu_entity == entt::null) {
        tmt::Log::error("No inventory menu found, please add the menu to the menu controller.");
        return;
    }
    if (check_for_open_menus()) return;
    unlock_mouse();
    tmt::engine.ecs.enable(inventory_menu_entity);
}

void MenuController::disable_inventory_menu() const {
    if (inventory_menu_entity == entt::null) {
        tmt::Log::error("No inventory menu found, please add the menu to the menu controller.");
        return;
    }
    lock_mouse();
    tmt::engine.ecs.disable(inventory_menu_entity);
}

void MenuController::enable_upgrade_menu() const {
    if (upgrade_menu_entity == entt::null) {
        tmt::Log::error("No upgrade menu found, please add the menu to the menu controller.");
        return;
    }
    if (check_for_open_menus()) return;
    unlock_mouse();
    tmt::engine.ecs.enable(upgrade_menu_entity);
}

void MenuController::disable_upgrade_menu() const {
    if (upgrade_menu_entity == entt::null) {
        tmt::Log::error("No upgrade menu found, please add the menu to the menu controller.");
        return;
    }
    lock_mouse();
    tmt::engine.ecs.disable(upgrade_menu_entity);
}

void MenuController::enable_end_of_game_menu(const EndRun& event) const {
    // Open end of game menu
    if (event.player_dead) {
        if (death_menu_entity == entt::null) {
            tmt::Log::error("No death menu found, please add the menu to the menu controller.");
            return;
        }
        unlock_mouse();
        tmt::engine.ecs.enable(death_menu_entity);
    } else {
        if (end_run_menu_entity == entt::null) {
            tmt::Log::error("No end run menu found, please add the menu to the menu controller.");
            return;
        }
        unlock_mouse();
        tmt::engine.ecs.enable(end_run_menu_entity);
    }
}

void MenuController::lock_mouse() {
    tmt::engine.input.lock_mouse(true);
    tmt::engine.input.set_mouse_relative_to_window(true);

    auto player_entity = tmt::engine.ecs.view<Player>().front().entity;  // Assuming there's only one player entity in the game
    // TODO this will get removed when proper game state are implemented
    tmt::engine.ecs.get_component<Player>(player_entity).set_state(PlayerState::FREEMOVING);
}

void MenuController::unlock_mouse() {
    tmt::engine.input.lock_mouse(false);
    tmt::engine.input.set_mouse_relative_to_window(false);

    auto player_entity = tmt::engine.ecs.view<Player>().front().entity;  // Assuming there's only one player entity in the game
    // TODO this will get removed when proper game state are implemented
    tmt::engine.ecs.get_component<Player>(player_entity).set_state(PlayerState::PAUSED);
}

bool MenuController::check_for_open_menus() const {
    if (tmt::engine.ecs.is_enabled(pause_menu_entity)) return true;
    if (tmt::engine.ecs.is_enabled(upgrade_menu_entity)) return true;
    if (tmt::engine.ecs.is_enabled(inventory_menu_entity)) return true;
    if (tmt::engine.ecs.is_enabled(end_run_menu_entity)) return true;
    if (tmt::engine.ecs.is_enabled(death_menu_entity)) return true;
    return false;
}

}  // namespace game
