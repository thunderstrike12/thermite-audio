#pragma once

#include "engine/core/system.hpp"
#include "engine/core/ecs.hpp"

#include "engine/systems/ai/steering/components/steering_agent.hpp"

namespace game {

/**
 * SensorsSystem
 * Updates world state facts for AI agents.
 *
 * Examples:
 *  - player_in_range
 *  - at_target
 * This system is called every fixed update.
 */
class SensorsSystem : public tmt::ISystem {
   public:
    // Inherited via ISystem
    std::string get_name() override { return "Sensor System"; }
    void on_start() override;
    void on_update(const tmt::FrameData& /*time*/) override;
    void on_fixed_update(const tmt::FrameData& /*time*/) override {}
    void on_end() override;

   private:
    tmt::Entity player_entity = entt::null;
};

}  // namespace game
