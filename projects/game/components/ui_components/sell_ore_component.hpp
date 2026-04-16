#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/text_renderer.hpp"
#include "projects/game/data_headers/wallet.hpp"

namespace game {

class SellOreComponent : public tmt::GameComponent<SellOreComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view name() { return "Sell Ore Button"; }
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    tmt::Entity entity_with_wallet = entt::null;
    tmt::Entity ore_property_entity = entt::null;
    tmt::Material::Type material_to_sell = tmt::Material::Type::NONE;

    float initial_hold_cooldown = 0.3f;
    float speed_up_factor = 1.2f;

   private:
    void sell_ore();
    void down_toggle();
    void up_toggle();
    bool entity_check() const;
    float timer = 0.0f;
    float current_cooldown = initial_hold_cooldown;
    bool is_held = false;
};

}  // namespace game
TMT_OBJECT(game::SellOreComponent, (entity_with_wallet, ore_property_entity, material_to_sell, initial_hold_cooldown, speed_up_factor));
