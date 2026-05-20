#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/button.hpp"
#include "projects/game/data_headers/wallet.hpp"

namespace game {

class ResourceLimitChecker : public tmt::GameComponent<ResourceLimitChecker> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Resource Limit Checker (Button)"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    tmt::Entity wallet_entity = entt::null;
    tmt::Entity flashing_ui_entity = entt::null;
    float flash_time = 0.1f;      // length of individual flash
    float flashing_time = 0.5f;   // length of flashing
    float check_interval = 1.0f;  // amount of time to pass to check in update for resource limit

   private:
    bool resource_check();
    void button_click(tmt::Button::Context context);
    void flash_ui();
    void stop_flash_ui();
    float interval_counter = 0.0f;
    float flash_counter = flash_time;
    float flashing_counter = 0.0f;
    bool is_flashing = false;
};

}  // namespace game

TMT_GAME_COMPONENT(game::ResourceLimitChecker, (wallet_entity, flashing_ui_entity, flash_time, flashing_time, check_interval));
