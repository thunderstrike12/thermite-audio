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

    tmt::Entity upgrade_target = entt::null;
    UpgradeType type = UpgradeType::MAX_HEALTH;
    float upgrade_to = 1.0f;
    uint64_t dollar_cost = 1.0f;

    std::vector<std::tuple<OreProperties::OreResources, uint64_t>> new_upgrade_costs = { { OreProperties::OreResources::NONE, 1.0f } };  // An initial cost
    bool apply_upgrade();
    void modify_upgrade_entities() const;
    void apply_entity_enable_disable() const;
    void button_apply(tmt::Button::Context context);
    std::vector<tmt::Entity> required_upgrade_entities = {};

    std::vector<tmt::Entity> entities_to_disable = { entt::null };
    std::vector<tmt::Entity> entities_to_enable = { entt::null };

   private:
    static void disable(tmt::Entity ett_to_dis) {
        if (tmt::engine.ecs.valid(ett_to_dis)) {
            tmt::engine.ecs.disable(ett_to_dis);
        }
    }

    static void enable(tmt::Entity ett_to_en) {
        if (tmt::engine.ecs.valid(ett_to_en)) {
            tmt::engine.ecs.enable(ett_to_en);
        }
    }
};

}  // namespace game
TMT_GAME_COMPONENT(game::Upgrade, (upgrade_target, type, upgrade_to, dollar_cost, new_upgrade_costs, required_upgrade_entities, entities_to_disable, entities_to_enable));
