#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/camera.hpp"

#include "projects/game/data_headers/events.hpp"

namespace game {

class MenuController : public tmt::GameComponent<MenuController> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Menu Controller"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    entt::entity pause_menu_entity = entt::null;
    entt::entity inventory_menu_entity = entt::null;
    entt::entity upgrade_menu_entity = entt::null;
    entt::entity end_run_menu_entity = entt::null;
    entt::entity death_menu_entity = entt::null;
    entt::entity player_entity = entt::null;
    entt::entity barge_entity = entt::null;
    entt::entity settings_menu_entity = entt::null;
    entt::entity close_settings_menu_entity = entt::null;
    float penalty_percentage = 0.6f;

    void enable_pause_menu() const;
    void disable_pause_menu() const;


    void enable_inventory_menu() const;
    void disable_inventory_menu() const;

    void enable_upgrade_menu() const;
    void disable_upgrade_menu() const;
    void handle_saving(const EndRun& event) const;

    void enable_end_of_game_menu(const EndRun& event) const;

   private:
    void lock_mouse() const;
    void unlock_mouse() const;
    bool check_for_open_menus() const;

    void enable_systems() const;
};

}  // namespace game
TMT_GAME_COMPONENT(
    game::MenuController, (pause_menu_entity, inventory_menu_entity, upgrade_menu_entity, end_run_menu_entity, death_menu_entity, player_entity, penalty_percentage, barge_entity,
                           settings_menu_entity, close_settings_menu_entity)
);
