#pragma once
#include "player.hpp"
#include "wallet.hpp"
namespace game {

enum class UpgradeType : uint8_t { MAX_HEALTH = 0u, MAX_BATTERY = 1u, MAX_SPEED = 2u, ACCELERATION = 3u, PRIMARY_ATK_SPEED = 4u, SECONDARY_ATK_SPEED = 5u, GUN_DMG = 6u, BARGE_FUEL = 7u };

enum class UpgradeResource : uint8_t { DOLLARS = 0u, GOLD = 1u, SILVER = 2u, COPPER = 3u, IRON = 4u, ENEMY_CORES = 5u };

class Upgrade : public tmt::GameComponent<Upgrade> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Upgrade"; }

    // added {} to prevent warnings about missing function bodies
    void start() override;
    void update(const tmt::FrameData& time) override {}
    void end() override;

    tmt::Entity upgrade_target = entt::null;
    UpgradeType type = UpgradeType::MAX_HEALTH;
    float upgrade_to = 1.0f;
    tmt::Entity player_entity = entt::null;
    std::vector<std::tuple<UpgradeResource, float>> upgrade_costs = { { UpgradeResource::DOLLARS, 1.0f } };  // An initial cost
    bool apply_upgrade();
    void button_apply();

   private:
};

}  // namespace game
TMT_OBJECT(game::Upgrade, (upgrade_target, type, upgrade_to, player_entity, upgrade_costs));
