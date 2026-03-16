#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/camera.hpp"

namespace game {

class MenuController : public tmt::GameComponent<MenuController> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Menu Controller"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    entt::entity pause_menu_entity;
    entt::entity inventory_menu_entity;
    entt::entity upgrade_menu_entity;

    void enable_pause_menu() const;
    void disable_pause_menu() const;

    void enable_inventory_menu() const;
    void disable_inventory_menu() const;

    void enable_upgrade_menu() const;
    void disable_upgrade_menu() const;

   private:
    static void lock_mouse();
    static void unlock_mouse();
    bool check_for_open_menus() const;
};

}  // namespace game
TMT_OBJECT(game::MenuController, (pause_menu_entity, inventory_menu_entity, upgrade_menu_entity));
