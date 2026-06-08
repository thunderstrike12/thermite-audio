#include "menu_controller.hpp"
#include "engine/core/input/input.hpp"
#include "projects/game/data_headers/game_input.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"
#include "projects/game/components/managers/weapon_manager.hpp"
#include "engine/core/audio.hpp"
#include "engine/systems/ui/ui.hpp"

// systems to disable for pause
#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/navigation/navigation_system.hpp"
#include "engine/systems/ai/steering/steering_system.hpp"
#include "engine/systems/animation/animation_system.hpp"
#include "engine/systems/camera/camera_system.hpp"            // probably not this one
#include "engine/systems/gameplay/gameplay.hpp"               // also probably not disable this one
#include "engine/systems/motion_math/motion_math_system.hpp"  // undure of this pause either
#include "engine/systems/physics/physics_system.hpp"
#include "engine/tools/player_data.hpp"
#include "engine/core/input/input_map.hpp"
#include "projects/game/components/development_tools/save_data.hpp"
#include "projects/game/components/gameplay_functionality_components/ore_collector.hpp"
#include "projects/game/data_headers/save_entries.hpp"
#include "projects/game/data_headers/wallet.hpp"
#include "projects/game/components/ui_components/entity_control_component.hpp"

namespace game {

void MenuController::start() {
    // Bind end run to event
    tmt::engine.ecs.get_dispatcher().sink<EndRun>().connect<&MenuController::enable_end_of_game_menu>(this);
    player_entity = Player::get().entity;  // Assuming there's only one player entity in the game

    // menus
    tmt::engine.input_map.add_action(action::OPEN_PAUSE_MENU);
    tmt::engine.input_map.add_key_to_action(action::OPEN_PAUSE_MENU, tmt::Key::ESCAPE);
}

void MenuController::update(const tmt::FrameData& time) {
    auto& input = tmt::engine.input;

    // if (input.is_action_just_pressed(action::OPEN_INVENTORY)) {
    //     if (inventory_menu_entity == entt::null) {
    //         tmt::Log::error("No inventory menu entity has been set, cannot open inventory.");
    //         return;
    //     }
    //
    //     if (tmt::engine.ecs.is_disabled(inventory_menu_entity)) {
    //         enable_inventory_menu();
    //     } else {
    //         disable_inventory_menu();
    //     }
    // }

    if (input.is_action_just_pressed(action::OPEN_PAUSE_MENU)) {
        if (pause_menu_entity == entt::null) {
            tmt::Log::debug("No pause menu entity has been set, cannot open pause menu.");
            return;
        }

        const bool settings_menu_open = settings_menu_entity != entt::null && tmt::engine.ecs.is_enabled(settings_menu_entity);
        // close_settings_menu_entity
        if (settings_menu_open) {
            if (close_settings_menu_entity != entt::null) {
                const auto& entity_control_component = tmt::engine.ecs.try_get_component<EntityControlComponent>(close_settings_menu_entity);
                if (entity_control_component) {
                    entity_control_component->handle_action();
                } else {
                    tmt::Log::error("No entity control component found for close settings menu entity, cannot close settings menu.");
                }
                return;  // if settings menu is open, we want to close it instead of handling the pause menu input
            } else {
                tmt::Log::error("No close settings menu entity has been set, cannot close settings menu.");
            }
        }

        if (tmt::engine.ecs.is_disabled(pause_menu_entity)) {
            enable_pause_menu();
        } else {
            disable_pause_menu();
        }
    }

    // if (input.is_action_just_pressed(action::OPEN_UPGRADE_MENU)) {
    //     if (upgrade_menu_entity == entt::null) {
    //         tmt::Log::error("No upgrade menu entity has been set, cannot open upgrade menu.");
    //         return;
    //     }
    //
    //     if (tmt::engine.ecs.is_disabled(upgrade_menu_entity)) {
    //         enable_upgrade_menu();
    //     } else {
    //         disable_upgrade_menu();
    //     }
    // }
}

void MenuController::end() {
    tmt::engine.ecs.get_dispatcher().sink<EndRun>().disconnect<&MenuController::enable_end_of_game_menu>(this);

    if (Player::get().player_ended_run == true) {
        handle_saving({ false });
    } else {
        handle_saving({ true });
    }

    enable_systems();
}

void MenuController::enable_pause_menu() const {
    if (pause_menu_entity == entt::null) {
        tmt::Log::error("No pause menu found, please add the menu to the menu controller.");
        return;
    }
    if (check_for_open_menus()) return;
    /*unlock_mouse();
    tmt::engine.ecs.enable(pause_menu_entity);*/

    auto& player = tmt::engine.ecs.get_component<Player>(player_entity);

    player.set_state_before_pause(player.get_state());

    player.set_state(PlayerState::PAUSED);

    unlock_mouse();

    tmt::engine.ecs.enable(pause_menu_entity);

    // Dispatch game paused event
    tmt::engine.ecs.get_dispatcher().trigger(game::GamePausedEvent {});

    // Disable HUD's
    player.set_hud_enabled(player.player_hud, false);
    player.set_hud_enabled(player.barge_hud, false);

    // disable systems
    auto& systems = tmt::engine.ecs.systems;

    if (systems.try_get<tmt::Goap>()) {
        systems.get<tmt::Goap>().disable();
    }
    if (systems.try_get<tmt::NavigationSystem>()) {
        systems.get<tmt::NavigationSystem>().disable();
    }
    if (systems.try_get<tmt::SteeringSystem>()) {
        systems.get<tmt::SteeringSystem>().disable();
    }
    if (systems.try_get<tmt::RigModelManager>()) {
        systems.get<tmt::RigModelManager>().disable();
    }
    if (systems.try_get<tmt::Physics>()) {
        systems.get<tmt::Physics>().disable();
    }

    // unpause any running sounds
    tmt::engine.audio.pause_game_audio();

    auto* ui = tmt::engine.ecs.systems.try_get<tmt::UI>();
    ui->menu_sounds.sounds.open_menu.play();
}

void MenuController::disable_pause_menu() const {
    if (pause_menu_entity == entt::null) {
        tmt::Log::error("No pause menu found, please add the menu to the menu controller.");
        return;
    }
    /*lock_mouse();
     tmt::engine.ecs.disable(pause_menu_entity);*/

    auto& player = tmt::engine.ecs.get_component<Player>(player_entity);

    player.set_state(player.get_state_before_pause());

    lock_mouse();

    tmt::engine.ecs.disable(pause_menu_entity);

    // Dispatch game unpaused event
    tmt::engine.ecs.get_dispatcher().trigger(game::GameUnpausedEvent {});

    // enable HUD's
    if (player.get_state() == PlayerState::FREEMOVING) player.set_hud_enabled(player.player_hud, true);
    if (player.get_state() == PlayerState::ATTACHED) player.set_hud_enabled(player.barge_hud, true);

    enable_systems();
}

void MenuController::enable_systems() const {
    // enable systems
    auto& systems = tmt::engine.ecs.systems;

    if (systems.try_get<tmt::Goap>()) {
        systems.get<tmt::Goap>().enable();
    }
    if (systems.try_get<tmt::NavigationSystem>()) {
        systems.get<tmt::NavigationSystem>().enable();
    }
    if (systems.try_get<tmt::SteeringSystem>()) {
        systems.get<tmt::SteeringSystem>().enable();
    }
    if (systems.try_get<tmt::RigModelManager>()) {
        systems.get<tmt::RigModelManager>().enable();
    }
    if (systems.try_get<tmt::Physics>()) {
        systems.get<tmt::Physics>().enable();
    }

    // unpause any running sounds
    tmt::engine.audio.resume_game_audio();
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

void MenuController::handle_saving(const EndRun& event) const {
    // this run
    float multiplier = 1.0f;
    if (event.player_dead) {
        multiplier = penalty_percentage;
    }
    save_resources_on_run(multiplier);
}
void MenuController::enable_end_of_game_menu(const EndRun& event) const {
    auto& player = tmt::engine.ecs.get_component<Player>(player_entity);
    player.set_hud_enabled(player.player_hud, false);
    player.set_hud_enabled(player.barge_hud, false);

    if (pause_menu_entity != entt::null) {
        tmt::engine.ecs.disable(pause_menu_entity);
    }
    if (upgrade_menu_entity != entt::null) {
        tmt::engine.ecs.disable(upgrade_menu_entity);
    }
    if (inventory_menu_entity != entt::null) {
        tmt::engine.ecs.disable(inventory_menu_entity);
    }

    // Open end of game menu
    if (event.player_dead) {
        // Ore gets reduced here
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

void MenuController::lock_mouse() const {
    tmt::engine.input.lock_mouse(true);
    tmt::engine.input.set_mouse_relative_to_window(true);

    // TODO this will get removed when proper game state are implemented
    // tmt::engine.ecs.get_component<Player>(player_entity).set_state(PlayerState::FREEMOVING);
}

void MenuController::unlock_mouse() const {
    tmt::engine.input.lock_mouse(false);
    tmt::engine.input.set_mouse_relative_to_window(false);

    // TODO this will get removed when proper game state are implemented
    // tmt::engine.ecs.get_component<Player>(player_entity).set_state(PlayerState::PAUSED);
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
