#pragma once
#include "player.hpp"
#include "projects/game/data_headers/wallet.hpp"
#include "engine/core/components/button.hpp"

namespace game {

enum class UpgradeType : uint8_t {
    // Player
    MAX_HEALTH,
    MAX_BATTERY,
    MAX_SPEED,
    ACCELERATION,
    MAX_BOOST_SPEED,
    HEALTH_RESTORE_SPEED,
    BATTERY_RESTORE_SPEED,
    EMERGENCY_BATTERY_TIME,
    STORAGE_LIMIT,
    // Barge
    BARGE_MAX_FUEL,
    BARGE_SPEED,
    BARGE_RECHARGE_DISTANCE,
    // Mining Tool
    DRILL_RADIUS,
    DRILL_DISTANCE,
    DRILL_CONSISTENCY,
    DRILL_SPEED,
    // Gravity Tool
    MAX_GRAB_SIZE,
    // Rifle
    RIFLE_IMPACT_RADIUS,
    RIFLE_EXPLOSION_POWER,
};
class Upgrade : public tmt::GameComponent<Upgrade> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Upgrade"; }

    // added {} to prevent warnings about missing function bodies
    void start() override;
    void update(const tmt::FrameData& time) override {}
    void end() override;

    UpgradeType type = UpgradeType::MAX_HEALTH;
    float upgrade_to = 1.0f;
    uint64_t dollar_cost = 1.0f;

    std::vector<std::tuple<OreProperties::OreResources, uint64_t>> new_upgrade_costs = { { OreProperties::OreResources::NONE, 1.0f } };  // An initial cost
    bool apply_upgrade();
    void modify_upgrade_entities() const;
    void button_apply(tmt::Button::Context context);

   private:
};

}  // namespace game
TMT_OBJECT(game::Upgrade, (type, upgrade_to, dollar_cost, new_upgrade_costs));
