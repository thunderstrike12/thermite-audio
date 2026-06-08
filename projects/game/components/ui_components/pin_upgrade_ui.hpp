#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/data_headers/wallet.hpp"
#include "projects/game/components/gameplay_functionality_components/upgrade.hpp"
#include "engine/core/components/text_renderer.hpp"
#include "pin_upgrade_button.hpp"

namespace game {

struct IconTextures {
    tmt::ResourceRef<tmt::Texture2D> dollar_icon_texture;
    tmt::ResourceRef<tmt::Texture2D> scrap_icon_texture;
    tmt::ResourceRef<tmt::Texture2D> copper_icon_texture;
    tmt::ResourceRef<tmt::Texture2D> thermite_icon_texture;
    tmt::ResourceRef<tmt::Texture2D> titanium_icon_texture;
};

class UpgradePinUiComponent : public tmt::GameComponent<UpgradePinUiComponent> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Upgrade Pin UI"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    IconTextures icon_textures {};

    tmt::Entity pin_text_entity_1 = entt::null;
    tmt::Entity pin_text_entity_2 = entt::null;
    tmt::Entity pin_text_entity_3 = entt::null;

    tmt::Entity pin_cost_template = entt::null;

   private:
    void spawn_cost_entity(tmt::Entity parent, int& cost_count, int cost, const tmt::ResourceRef<tmt::Texture2D>& icon_texture_ref, float vertical_padding, float y_pos_diff) const;
};

}  // namespace game
TMT_OBJECT(game::IconTextures, (dollar_icon_texture, scrap_icon_texture, copper_icon_texture, thermite_icon_texture, titanium_icon_texture))
TMT_GAME_COMPONENT(game::UpgradePinUiComponent, (icon_textures, pin_text_entity_1, pin_text_entity_2, pin_text_entity_3, pin_cost_template));
