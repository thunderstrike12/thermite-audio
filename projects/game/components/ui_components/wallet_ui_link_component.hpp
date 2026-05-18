#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/text_renderer.hpp"
#include "projects/game/data_headers/wallet.hpp"

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

enum class DisplayType : uint8_t { CURRENT, LIMIT };

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

   private:
    void change_text() const;
    void update_value();
    bool wallet_entity_check() const;
    int current_value = 0;
};

}  // namespace game
TMT_OBJECT(game::WalletUiLink, (entities_with_wallet, resource_to_display, display_type));
