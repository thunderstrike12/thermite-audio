#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/text_renderer.hpp"
#include "projects/game/data_headers/wallet.hpp"
#include "engine/tools/types/bezier_curve.hpp"
namespace game {

enum class DisplayTextType : uint8_t {
    NONE = static_cast<uint8_t>(OreProperties::OreResources::NONE),
    SCRAP = static_cast<uint8_t>(OreProperties::OreResources::SCRAP),
    COPPER = static_cast<uint8_t>(OreProperties::OreResources::COPPER),
    THERMITE = static_cast<uint8_t>(OreProperties::OreResources::THERMITE),
    TITANIUM = static_cast<uint8_t>(OreProperties::OreResources::TITANIUM),
    DOLLARS,
    ALL_RESOURCES
};

enum class DisplayType : uint8_t { CURRENT, LIMIT, PENALTY, TOTAL };

class WalletUiLink : public tmt::GameComponent<WalletUiLink> {
   public:
    using GameComponent::GameComponent;
    static std::string_view name() { return "Wallet Ui Link"; }
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    std::vector<tmt::Entity> entities_with_wallet = { entt::null };
    DisplayTextType resource_to_display = DisplayTextType::DOLLARS;
    DisplayType display_type = DisplayType::CURRENT;
    tmt::BezierCurve ramp_up_curve;
    bool use_ramp_up_curve = true;
    float ramp_duration = 0.75f;

   private:
    void change_text() const;
    void update_value();
    bool wallet_entity_check() const;
    bool component_check() const;
    int previous_value = -1;
    int target_value = 0;
    float ramp_start_value = 0.0f;
    float ramp_target_value = 0.0f;
    float ramp_elapsed = 0.0f;
    bool is_ramping = false;
};

}  // namespace game
TMT_GAME_COMPONENT(game::WalletUiLink, (entities_with_wallet, resource_to_display, display_type, ramp_up_curve, use_ramp_up_curve, ramp_duration));
