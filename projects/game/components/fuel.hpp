#pragma once
#include "events.hpp"
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class FuelComponent : public tmt::GameComponent<FuelComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Fuel Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override {};
    void end() override;
    void on_trigger_movement(const MovementUpdateEvent& event);

    tmt::Entity mover_entity = entt::null;
    float max_fuel = 100.0f;
    float decrease_multiplier = 1.0f;

    float current_fuel = max_fuel;

   private:
};

}  // namespace game
TMT_OBJECT(game::FuelComponent, (mover_entity, max_fuel, decrease_multiplier, current_fuel));
