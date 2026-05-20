#pragma once
#include "engine/systems/gameplay/game_component.hpp"

#include "projects/game/data_headers/events.hpp"
namespace game {

class OreCollector : public tmt::GameComponent<OreCollector> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "OreCollector"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    // Shoots rays
    float radius = 1.0f;
    // debugging settings
    tmt::Entity wallet_entity { entt::null };

   private:
    uint64_t ore_count = 0u;
    void on_collision_trigger(const TriggerCollisionEvent& trigger);
};

}  // namespace game
TMT_GAME_COMPONENT(game::OreCollector, (radius, wallet_entity));
