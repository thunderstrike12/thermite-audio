#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/button.hpp"
#include "projects/game/data_headers/wallet.hpp"
#include "projects/game/components/gameplay_functionality_components/upgrade.hpp"
#include "engine/core/components/text_renderer.hpp"

namespace game {

struct PinnedUpgradeData {
    std::string shown_name = "None";
    int dollar_cost = 0;
    int scrap_cost = 0;
    int copper_cost = 0;
    int thermite_cost = 0;
    int titanium_cost = 0;
    tmt::Entity upgrade_pin_button_entity = entt::null;
    bool active = false;
};
struct SavedUpgradePins {
    std::array<PinnedUpgradeData, 3> pinned_upgrades;
};
class UpgradePinButtonComponent : public tmt::GameComponent<UpgradePinButtonComponent> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Upgrade Pin Button"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    tmt::Entity upgrade_entity = entt::null;
    bool is_pinned = false;
    std::string shown_name = "Needs setup.";

   private:
    void add_pin_to_player_data();
    void remove_pin_from_player_data();
    void handle_button_press(tmt::Button::Context context);
    PinnedUpgradeData pinned_upgrade_data;
};

}  // namespace game
TMT_OBJECT(game::PinnedUpgradeData, (shown_name, dollar_cost, scrap_cost, copper_cost, thermite_cost, titanium_cost, upgrade_pin_button_entity, active));
TMT_OBJECT(game::SavedUpgradePins, (pinned_upgrades))
TMT_GAME_COMPONENT(game::UpgradePinButtonComponent, (upgrade_entity, shown_name, is_pinned));
